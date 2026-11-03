#pragma once
// Original silent media and artwork for deterministic playback/metadata tests.
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QBuffer>
#include <QImage>
inline bool writeMusicFixture(const QString &path){
    QDir().mkpath(QFileInfo(path).absolutePath());QByteArray frames;
    auto frame=[&](const char *name,const QByteArray &payload){frames.append(name,4);QByteArray header;QDataStream size(&header,QIODevice::WriteOnly);size.setByteOrder(QDataStream::BigEndian);size<<quint32(payload.size())<<quint16(0);frames+=header;frames+=payload;};
    frame("TIT2",QByteArray(1,'\0')+"Local Music Test");frame("TPE1",QByteArray(1,'\0')+"Cockpit Studio");frame("TALB",QByteArray(1,'\0')+"Offline Sessions");
    QImage image(100,100,QImage::Format_RGB32);image.fill(QColor("#537d68"));for(int y=20;y<80;++y)for(int x=20;x<80;++x)if((x-50)*(x-50)+(y-50)*(y-50)<600)image.setPixelColor(x,y,QColor("#e1cf9c"));
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");
    QByteArray picture(1,'\0');picture+="image/png";picture.append('\0');picture.append(char(3));picture.append('\0');picture+=png;frame("APIC",picture);
    QByteArray id3("ID3\x03\x00\x00",6);for(int shift:{21,14,7,0})id3.append(char((frames.size()>>shift)&0x7f));id3+=frames;
    QFile file(path);if(!file.open(QIODevice::WriteOnly))return false;QDataStream stream(&file);stream.setByteOrder(QDataStream::LittleEndian);QByteArray silence(22050*2*15,'\0');
    stream.writeRawData("RIFF",4);stream<<quint32(36+silence.size()+8+id3.size()+(id3.size()%2));stream.writeRawData("WAVEfmt ",8);stream<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(22050)<<quint32(44100)<<quint16(2)<<quint16(16);stream.writeRawData("data",4);stream<<quint32(silence.size());stream.writeRawData(silence.data(),silence.size());stream.writeRawData("id3 ",4);stream<<quint32(id3.size());stream.writeRawData(id3.data(),id3.size());if(id3.size()%2)stream.writeRawData("\0",1);return true;
}
