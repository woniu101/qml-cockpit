#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2bth.h>
#endif
#include "AvrcpTransportProbe.h"
#include <QJsonArray>
#include <QScopeGuard>
QJsonObject probeLocalAvrcpTransport(){
    QJsonObject report{{"remoteConnectionsAttempted",false},{"driverChanges",false},{"purpose","Test the local provider only; socket creation is not a working AVRCP channel"}};
#ifdef Q_OS_WIN
    WSADATA data{};const auto error=WSAStartup(MAKEWORD(2,2),&data);report["startupError"]=error;if(error)return report;auto cleanup=qScopeGuard([]{WSACleanup();});QJsonArray rows;
    for(const auto protocol:{BTHPROTO_RFCOMM,BTHPROTO_L2CAP}){
        QJsonObject row{{"requestedProtocol",protocol}};const auto handle=socket(AF_BTH,SOCK_STREAM,protocol);row["socketCreated"]=handle!=INVALID_SOCKET;
        if(handle==INVALID_SOCKET){row["socketError"]=WSAGetLastError();rows.append(row);continue;}auto close=qScopeGuard([&]{closesocket(handle);});WSAPROTOCOL_INFOW info{};int length=sizeof(info);
        if(getsockopt(handle,SOL_SOCKET,SO_PROTOCOL_INFOW,reinterpret_cast<char*>(&info),&length)==0){row["provider"]=QString::fromWCharArray(info.szProtocol);row["actualProtocol"]=info.iProtocol;}
        SOCKADDR_BTH address{};address.addressFamily=AF_BTH;address.port=BT_PORT_ANY;const auto result=bind(handle,reinterpret_cast<const sockaddr*>(&address),sizeof(address));row["bindSucceeded"]=result==0;row["bindError"]=result==0?0:WSAGetLastError();rows.append(row);
    }
    report["socketChecks"]=rows;
#else
    report["error"]="Windows only";
#endif
    return report;
}
