#include "MusicFiles.h"
#include "MusicSource.h"
#include "media/MediaDirectory.h"
#include <QFile>
#include <QStringDecoder>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QImage>
#include <QCryptographicHash>
#include <QScopeGuard>
#include <QThread>
#include <QDateTime>
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>
#include <chrono>
#endif
namespace {
QVariantMap prefixedId3Tags(const QString &path,const QString &cache){
    const auto offset=musicPayloadOffset(path);if(!offset)return {};QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};const auto bytes=file.read(offset);QVariantMap tags;
    if(bytes.size()<10||(quint8(bytes[5])&0xc0)||quint8(bytes[3])<3)return tags;const bool v4=bytes[3]==4;
    auto decode=[](const QByteArray &data){if(data.isEmpty())return QString();const auto text=data.mid(1);QString result;
        switch(quint8(data[0])){case 0:result=QString::fromLatin1(text);break;case 1:result=QStringDecoder(QStringConverter::Utf16).decode(text);break;case 2:result=QStringDecoder(QStringConverter::Utf16BE).decode(text);break;case 3:result=QString::fromUtf8(text);break;}
        return result.replace(QChar(0),' ').trimmed().left(1024);
    };
    for(qint64 at=10;at+10<=bytes.size();){
        const auto name=bytes.mid(at,4);quint32 size=0;bool valid=true;for(int i=4;i<8;++i){const auto byte=quint8(bytes[at+i]);if(v4&&(byte&0x80))valid=false;size=(size<<(v4?7:8))|byte;}
        if(!valid||!size||size>quint64(bytes.size()-at-10))break;const auto data=bytes.mid(at+10,size);const bool flagged=bytes[at+8]!=0||bytes[at+9]!=0;at+=10+size;if(flagged)continue;
        if(name=="TIT2")tags["title"]=decode(data);else if(name=="TPE1")tags["artist"]=decode(data);else if(name=="TALB")tags["album"]=decode(data);
        else if(name=="APIC"&&data.size()>4&&!cache.isEmpty()){
            const auto mimeEnd=data.indexOf('\0',1);if(mimeEnd<0||mimeEnd+2>=data.size())continue;qsizetype start=mimeEnd+2;
            const int unit=(quint8(data[0])==1||quint8(data[0])==2)?2:1;
            while(start+unit<=data.size()){bool end=true;for(int i=0;i<unit;++i)end&=data[start+i]==0;start+=unit;if(end)break;}
            if(start>=data.size()||data.size()-start>8*1024*1024)continue;QImage image;
            if(image.loadFromData(data.mid(start))&&image.width()*qint64(image.height())<=40000000){
                QDir().mkpath(cache);const auto key=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();const auto cover=cache+"/prefix-"+key+".jpg";
                if(QFileInfo::exists(cover)||image.scaled(512,512,Qt::KeepAspectRatio,Qt::SmoothTransformation).save(cover,"JPG"))tags["cover"]=QUrl::fromLocalFile(cover).toString();
            }
        }
    }return tags;
}
}
QString musicPathKey(const QString &path){return mediaPathKey(path);}
bool supportedMusicFile(const QString &path){return QStringList{"mp3","wav","flac","m4a","ogg","aac","opus","wma"}.contains(QFileInfo(path).suffix().toLower());}
QString sidecarCover(const QString &path){
    const QFileInfo file(path);const QString dir=file.absolutePath()+"/",base=dir+file.completeBaseName();
    for(const auto &candidate:QStringList{base+".jpg",base+".png",dir+"cover.jpg",dir+"cover.png",dir+"folder.jpg",dir+"folder.png"})if(QFileInfo(candidate).isFile())return QUrl::fromLocalFile(candidate).toString();
    return {};
}
QString musicFileStamp(const QString &path){
    const QFileInfo file(path);const QFileInfo cover(QUrl(sidecarCover(path)).toLocalFile());
    return "2:"+QString::number(file.size())+":"+QString::number(file.lastModified().toMSecsSinceEpoch())+":"+cover.absoluteFilePath()+":"+QString::number(cover.lastModified().toMSecsSinceEpoch());
}
QVariantMap readMusicFile(const QString &path,const QString &cacheDirectory){
    const QFileInfo info(path);QVariantMap row{{"url",QUrl::fromLocalFile(info.absoluteFilePath()).toString()},{"title",info.completeBaseName()},{"artist",QString()},{"album",QString()},{"folder",info.absolutePath()},{"available",info.isFile()},{"source",QStringLiteral("本地文件")},{"cover",sidecarCover(path)}};
    row["_stamp"]=musicFileStamp(path);if(!info.isFile())return row;const auto fallback=prefixedId3Tags(path,cacheDirectory);for(auto it=fallback.cbegin();it!=fallback.cend();++it)if(!it.value().toString().isEmpty())row[it.key()]=it.value();
#ifdef Q_OS_WIN
    using namespace winrt::Windows::Storage;using namespace winrt::Windows::Storage::FileProperties;using namespace winrt::Windows::Storage::Streams;using namespace std::chrono_literals;
    auto completed=[](const auto &operation){if(operation.wait_for(2s)!=winrt::Windows::Foundation::AsyncStatus::Completed){operation.Cancel();return false;}return true;};
    try{
        auto get=StorageFile::GetFileFromPathAsync(QDir::toNativeSeparators(info.absoluteFilePath()).toStdWString());if(!completed(get))return row;const auto file=get.GetResults();
        auto properties=file.Properties().GetMusicPropertiesAsync();if(completed(properties)){
            const auto tags=properties.GetResults();auto value=[](const winrt::hstring &text){return QString::fromWCharArray(text.c_str()).trimmed().left(1024);};
            if(!value(tags.Title()).isEmpty())row["title"]=value(tags.Title());if(!value(tags.Artist()).isEmpty())row["artist"]=value(tags.Artist());if(!value(tags.Album()).isEmpty())row["album"]=value(tags.Album());if(tags.Duration().count()>0)row["duration"]=tags.Duration().count()/10000;
        }
        if(cacheDirectory.isEmpty()||QThread::currentThread()->isInterruptionRequested())return row;
        const auto key=QCryptographicHash::hash((musicPathKey(path)+QString::number(info.size())+info.lastModified().toString(Qt::ISODateWithMs)).toUtf8(),QCryptographicHash::Sha256).toHex();
        const QString coverPath=cacheDirectory+"/"+key+".jpg";
        if(QFileInfo::exists(coverPath)){row["cover"]=QUrl::fromLocalFile(coverPath).toString();return row;}
        auto thumb=file.GetThumbnailAsync(ThumbnailMode::MusicView,512);if(!completed(thumb))return row;const auto stream=thumb.GetResults();
        if(stream&&stream.Type()==ThumbnailType::Image&&stream.Size()>0&&stream.Size()<=8*1024*1024){
            DataReader reader(stream.GetInputStreamAt(0));auto load=reader.LoadAsync(quint32(stream.Size()));if(!completed(load))return row;load.GetResults();
            QByteArray data(int(stream.Size()),Qt::Uninitialized);reader.ReadBytes(winrt::array_view<uint8_t>(reinterpret_cast<uint8_t*>(data.data()),reinterpret_cast<uint8_t*>(data.data())+data.size()));
            QImage image;if(image.loadFromData(data)&&image.width()*qint64(image.height())<=40000000){QDir().mkpath(cacheDirectory);if(image.scaled(512,512,Qt::KeepAspectRatio,Qt::SmoothTransformation).save(coverPath,"JPG"))row["cover"]=QUrl::fromLocalFile(coverPath).toString();}
        }
    }catch(const winrt::hresult_error&){}
#else
    Q_UNUSED(cacheDirectory);
#endif
    return row;
}
