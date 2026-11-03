#include "MusicSource.h"
#include <QFile>
#include <QMediaPlayer>

qint64 musicPayloadOffset(const QString &path){
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return 0;const auto header=file.read(10);
    if(header.size()!=10||header.left(3)!="ID3"||quint8(header[3])<2||quint8(header[3])>4)return 0;
    qint64 length=0;for(int i=6;i<10;++i){const auto byte=quint8(header[i]);if(byte&0x80)return 0;length=(length<<7)|byte;}
    const qint64 offset=10+length+((header[3]==4&&(quint8(header[5])&0x10))?10:0);
    if(length>16*1024*1024||offset+16>file.size()||!file.seek(offset))return 0;
    const auto payload=file.read(16);quint32 boxSize=0;for(int i=0;i<4;++i)boxSize=(boxSize<<8)|quint8(payload[i]);
    return payload.mid(4,4)=="ftyp"&&boxSize>=16&&boxSize<=file.size()-offset?offset:0;
}
namespace {
class PayloadFile final:public QIODevice {
public:
    PayloadFile(const QString &path,qint64 offset):m_file(path),m_offset(offset){
        if(m_file.open(QIODevice::ReadOnly)&&m_file.seek(offset))open(QIODevice::ReadOnly|QIODevice::Unbuffered);
    }
    bool isSequential() const override{return false;}
    qint64 size() const override{return qMax(0LL,m_file.size()-m_offset);}
    bool seek(qint64 position) override{return position>=0&&position<=size()&&m_file.seek(m_offset+position)&&QIODevice::seek(position);}
protected:
    qint64 readData(char *data,qint64 size) override{return m_file.read(data,size);}
    qint64 writeData(const char*,qint64) override{return -1;}
private:
    QFile m_file;
    qint64 m_offset;
};
}
void setMusicSource(QMediaPlayer &player,const QUrl &url,std::unique_ptr<QIODevice> &owner){
    player.setSource(QUrl());owner.reset();
    const auto offset=url.isLocalFile()?musicPayloadOffset(url.toLocalFile()):0;
    if(offset){auto device=std::make_unique<PayloadFile>(url.toLocalFile(),offset);if(device->isOpen())owner=std::move(device);}
    if(owner)player.setSourceDevice(owner.get(),url);else player.setSource(url);
}
