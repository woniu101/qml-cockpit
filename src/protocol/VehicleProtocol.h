#pragma once
// Vehicle protocol v1. Keep this wire-codec identical in both repositories.
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>
#include <cmath>

namespace VehicleProtocol {
constexpr int Version=1;
constexpr quint32 MaxFrame=65536;
inline QByteArray encode(QJsonObject object) {
    object["protocol"]=Version;
    const QByteArray payload=QJsonDocument(object).toJson(QJsonDocument::Compact);
    if(payload.size()>MaxFrame)return {};
    QByteArray frame(4,'\0');qToBigEndian<quint32>(quint32(payload.size()),frame.data());frame+=payload;return frame;
}
inline bool decode(QByteArray &buffer,QList<QJsonObject> &messages,QString &error) {
    if(buffer.size()>int(MaxFrame*4)){error=QStringLiteral("接收缓冲区超过限制");return false;}
    while(buffer.size()>=4){
        const auto size=qFromBigEndian<quint32>(buffer.constData());
        if(size==0||size>MaxFrame){error=QStringLiteral("消息长度无效");return false;}
        if(buffer.size()<qsizetype(size+4))break;
        QJsonParseError parse;const auto document=QJsonDocument::fromJson(buffer.mid(4,size),&parse);buffer.remove(0,size+4);
        if(parse.error!=QJsonParseError::NoError||!document.isObject()){error=QStringLiteral("消息必须是有效 JSON 对象");return false;}
        const auto object=document.object();
        if(object["protocol"].toInt(-1)!=Version){error=QStringLiteral("协议版本不兼容");return false;}
        if(!object["type"].isString()){error=QStringLiteral("消息缺少类型");return false;}
        messages.append(object);
    }
    return true;
}
inline bool number(const QJsonValue &v,double minimum,double maximum,bool integer=false) {
    return v.isDouble()&&std::isfinite(v.toDouble())&&v.toDouble()>=minimum&&v.toDouble()<=maximum&&(!integer||std::floor(v.toDouble())==v.toDouble());
}
inline bool validState(const QJsonObject &s) {
    if(!number(s["soc"],0,100)||!number(s["speed"],0,100,true)||!number(s["tripKm"],0,1e9)||!number(s["forwardKm"],0,1e9)||!number(s["chargeTarget"],50,100,true)||!number(s["chargePower"],0,350))return false;
    const auto gear=s["gear"].toString();if(!QStringList{"P","R","N","D"}.contains(gear))return false;
    if(((gear=="P"||gear=="N")&&s["speed"].toInt()!=0)||(gear=="R"&&s["speed"].toInt()>15))return false;
    for(const char *key:{"locked","hood","hatch","hoodMoving","hatchMoving","mirrors","port","plugged","charging"})if(!s[key].isBool())return false;
    for(const char *key:{"windows","targets","windowMoving","doors","doorMoving"}){
        if(!s[key].isArray()||s[key].toArray().size()!=4)return false;
        for(const auto &v:s[key].toArray())if(QByteArray(key)=="windows"||QByteArray(key)=="targets"){if(!number(v,0,100,QByteArray(key)=="targets"))return false;}else if(!v.isBool())return false;
    }
    return true;
}
}
