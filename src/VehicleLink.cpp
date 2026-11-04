#include "VehicleLink.h"
#include "protocol/VehicleProtocol.h"
#include <QUuid>
#include <QTime>

VehicleLink::VehicleLink(QObject *parent):QObject(parent){
    m_clock.start();m_timer.setInterval(100);connect(&m_timer,&QTimer::timeout,this,&VehicleLink::tick);m_timer.start();
    connect(&m_socket,&QLocalSocket::readyRead,this,&VehicleLink::read);
    connect(&m_socket,&QLocalSocket::connected,this,[this]{m_connectedAt=m_clock.elapsed();m_socket.write(VehicleProtocol::encode({{"type","hello"},{"role","cockpit"}}));m_status=QStringLiteral("已连接，等待车辆快照");emit statusChanged();});
    connect(&m_socket,&QLocalSocket::disconnected,this,[this]{if(m_enabled){invalidate(QStringLiteral("模拟器已断开，正在重连"));m_nextAttempt=m_clock.elapsed()+1000;}});
    connect(&m_socket,&QLocalSocket::errorOccurred,this,[this](QLocalSocket::LocalSocketError){if(m_enabled&&m_socket.state()==QLocalSocket::UnconnectedState){m_status=QStringLiteral("未连接模拟器，正在重试");emit statusChanged();m_nextAttempt=m_clock.elapsed()+1000;}});
}
VehicleLink::~VehicleLink(){
    // Socket teardown may emit disconnected; stop callbacks before member state is destroyed.
    m_timer.stop();m_socket.disconnect(this);m_socket.abort();
}
void VehicleLink::log(const QString &line){m_events.prepend(QTime::currentTime().toString("HH:mm:ss")+"  "+line);while(m_events.size()>60)m_events.removeLast();emit activityChanged();}
void VehicleLink::setEnabled(bool enabled){if(enabled==m_enabled)return;m_enabled=enabled;m_ready=false;m_session.clear();m_buffer.clear();m_sequence=0;failPending(QStringLiteral("数据源已切换"));m_socket.abort();m_status=enabled?QStringLiteral("正在连接模拟器"):QStringLiteral("内置演示");m_nextAttempt=0;emit statusChanged();if(enabled)reconnect();}
void VehicleLink::setServerName(const QString &name){const auto value=name.trimmed();if(value.isEmpty()||value.size()>120||value==m_serverName)return;m_serverName=value;if(m_enabled)reconnect();emit statusChanged();}
void VehicleLink::reconnect(){if(!m_enabled)return;invalidate(QStringLiteral("正在连接模拟器"));m_socket.abort();m_nextAttempt=m_clock.elapsed()+1000;m_socket.setReadBufferSize(VehicleProtocol::MaxFrame*4);m_socket.connectToServer(m_serverName);}
void VehicleLink::failPending(const QString &reason){const auto pending=m_pending;m_pending.clear();for(auto i=pending.cbegin();i!=pending.cend();++i){const auto &p=i.value();m_lastCommand=p.label+QStringLiteral(" · 未确认 · ")+reason;log(m_lastCommand);emit commandResult(p.kind,p.args,"failed",reason,i.key());}emit activityChanged();}
void VehicleLink::invalidate(const QString &status){const bool wasReady=m_ready;m_ready=false;m_session.clear();m_buffer.clear();m_sequence=0;m_status=status;failPending(status);if(wasReady)log(status);emit statusChanged();}
QString VehicleLink::request(const QString &kind,const QJsonObject &args,const QString &label){
    if(!ready()){emit commandResult(kind,args,"failed",QStringLiteral("模拟器数据不可用，请等待连接恢复"),{});return {};}
    if(m_pending.size()>=64){emit commandResult(kind,args,"failed",QStringLiteral("待处理指令过多，请稍候"),{});return {};}
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);m_pending.insert(id,{kind,label,args,m_clock.elapsed()+1800,false});
    m_socket.write(VehicleProtocol::encode({{"type","command"},{"session",m_session},{"id",id},{"kind",kind},{"args",args}}));m_lastCommand=label+QStringLiteral(" · 等待回执");log(m_lastCommand);return id;
}
void VehicleLink::read(){m_buffer+=m_socket.read(VehicleProtocol::MaxFrame*4);QList<QJsonObject> messages;QString error;if(!VehicleProtocol::decode(m_buffer,messages,error)){invalidate(error);m_socket.abort();return;}for(const auto &message:messages)this->message(message);}
void VehicleLink::message(const QJsonObject &m){
    const auto type=m["type"].toString();
    if(type=="hello"){
        const auto session=m["session"].toString();if(session.isEmpty()||session.size()>80){invalidate(QStringLiteral("模拟器会话无效"));m_socket.abort();return;}m_session=session;m_sequence=0;return;
    }
    if(m_session.isEmpty()||m["session"].toString()!=m_session)return;
    if(type=="snapshot"){
        if(!VehicleProtocol::number(m["sequence"],1,9e15,true)||!m["state"].isObject()||!VehicleProtocol::validState(m["state"].toObject())){invalidate(QStringLiteral("模拟器数据无效"));m_socket.abort();return;}
        const auto sequence=quint64(m["sequence"].toDouble());if(sequence<=m_sequence)return;m_sequence=sequence;m_lastSnapshot=m_clock.elapsed();const bool first=!m_ready;m_ready=true;m_status=QStringLiteral("外部联调 · 已连接");emit snapshotReceived(m["state"].toObject());if(first){log(QStringLiteral("已收到有效车辆快照"));emit statusChanged();}return;
    }
    if(type=="result"){
        const auto id=m["id"].toString(),status=m["status"].toString();if(!m_pending.contains(id))return;
        if(!QStringList{"accepted","completed","rejected","cancelled"}.contains(status))return;
        auto p=m_pending[id];const auto reason=m["reason"].toString().left(240);
        if(status=="accepted"){m_pending[id].accepted=true;m_pending[id].deadline=m_clock.elapsed()+6000;}else m_pending.remove(id);
        const QString text=status=="accepted"?QStringLiteral("执行中"):status=="completed"?QStringLiteral("已完成"):status=="rejected"?QStringLiteral("已拒绝"):QStringLiteral("已取消");
        m_lastCommand=p.label+" · "+text+(status=="rejected"||status=="cancelled"?" · "+reason:"");log(m_lastCommand);emit commandResult(p.kind,p.args,status,reason,id);
    }
}
void VehicleLink::tick(){
    if(!m_enabled)return;const auto now=m_clock.elapsed();
    if(m_socket.state()==QLocalSocket::UnconnectedState&&now>=m_nextAttempt){reconnect();return;}
    if(m_socket.state()==QLocalSocket::ConnectedState){
        if((m_ready&&now-m_lastSnapshot>3000)||(!m_ready&&now-m_connectedAt>5000)){invalidate(QStringLiteral("车辆数据已过期，正在重新连接"));m_socket.abort();m_nextAttempt=now+1000;return;}
    }
    QStringList expired;for(auto i=m_pending.cbegin();i!=m_pending.cend();++i)if(now>i->deadline)expired<<i.key();
    for(const auto &id:expired){const auto p=m_pending.take(id);m_lastCommand=p.label+QStringLiteral(" · 超时，实际状态以遥测为准");log(m_lastCommand);emit commandResult(p.kind,p.args,"timeout",QStringLiteral("指令超时，实际状态以遥测为准"),id);}
}
