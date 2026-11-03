#include <QtTest>
#include <QLocalServer>
#include <QUuid>
#include <memory>
#include "VehicleLink.h"
#include "protocol/VehicleProtocol.h"

class VehicleLinkTests : public QObject {
    Q_OBJECT
    static QJsonObject state() {
        return {{"soc",63.5},{"gear","P"},{"speed",0},{"tripKm",0},{"forwardKm",0},
            {"windows",QJsonArray{0,15,50,100}},{"targets",QJsonArray{0,15,50,100}},
            {"windowMoving",QJsonArray{false,false,false,false}},{"doors",QJsonArray{false,false,false,false}},
            {"doorMoving",QJsonArray{false,false,false,false}},{"locked",true},{"hood",false},{"hatch",false},
            {"hoodMoving",false},{"hatchMoving",false},{"mirrors",false},{"port",false},{"plugged",false},
            {"charging",false},{"chargeTarget",90},{"chargePower",0}};
    }
    static QJsonObject snapshot(int sequence, QJsonObject values=state(), QString session="session-a") {
        return {{"type","snapshot"},{"session",session},{"sequence",sequence},{"state",values}};
    }
    static void send(QLocalSocket *peer,const QJsonObject &message) { peer->write(VehicleProtocol::encode(message));peer->flush(); }
    static QString name() { return "qml-cockpit-unit-"+QUuid::createUuid().toString(QUuid::WithoutBraces); }
private slots:
    void handshakeSequenceAndValidation() {
        QLocalServer server;QVERIFY(server.listen(name()));VehicleLink link;link.setServerName(server.serverName());link.setEnabled(true);
        QTRY_VERIFY(server.hasPendingConnections());auto *peer=server.nextPendingConnection();QSignalSpy snapshots(&link,&VehicleLink::snapshotReceived);
        send(peer,{{"type","hello"},{"session","session-a"}});QTest::qWait(30);QVERIFY(!link.ready());
        send(peer,snapshot(1,state(),"other-session"));QTest::qWait(30);QVERIFY(!link.ready());
        send(peer,snapshot(2));QTRY_VERIFY(link.ready());QCOMPARE(snapshots.size(),1);
        send(peer,snapshot(1));send(peer,snapshot(2));QTest::qWait(40);QCOMPARE(snapshots.size(),1);
        auto invalid=state();invalid["soc"]=101;send(peer,snapshot(3,invalid));QTRY_VERIFY(!link.ready());QCOMPARE(snapshots.size(),1);
        link.setEnabled(false);
    }
    void receiptsAndSourceCancellation() {
        QLocalServer server;QVERIFY(server.listen(name()));VehicleLink link;link.setServerName(server.serverName());link.setEnabled(true);
        QTRY_VERIFY(server.hasPendingConnections());auto *peer=server.nextPendingConnection();send(peer,{{"type","hello"},{"session","session-a"}});send(peer,snapshot(1));QTRY_VERIFY(link.ready());
        QSignalSpy results(&link,&VehicleLink::commandResult);const auto id=link.request("setWindow",{{"index",0},{"target",50}},"window");QVERIFY(!id.isEmpty());QCOMPARE(link.pendingCount(),1);
        send(peer,{{"type","result"},{"session","old-session"},{"id",id},{"status","completed"}});QTest::qWait(30);QCOMPARE(link.pendingCount(),1);
        send(peer,{{"type","result"},{"session","session-a"},{"id",id},{"status","accepted"}});QTRY_COMPARE(results.size(),1);QCOMPARE(link.pendingCount(),1);
        send(peer,{{"type","result"},{"session","session-a"},{"id",id},{"status","completed"}});QTRY_COMPARE(link.pendingCount(),0);QCOMPARE(results.last()[2].toString(),QString("completed"));
        link.request("setSpeed",{{"value",20}},"speed");link.setEnabled(false);QCOMPARE(link.pendingCount(),0);QCOMPARE(results.last()[2].toString(),QString("failed"));QVERIFY(!link.ready());
    }
    void protocolVersionAndMalformedFrames() {
        QByteArray buffer=VehicleProtocol::encode({{"type","hello"}});buffer.replace("\"protocol\":1","\"protocol\":2");QList<QJsonObject> messages;QString error;
        QVERIFY(!VehicleProtocol::decode(buffer,messages,error));QVERIFY(messages.isEmpty());
        buffer=QByteArray::fromHex("00000002")+"[]";QVERIFY(!VehicleProtocol::decode(buffer,messages,error));
        buffer=QByteArray::fromHex("00000000");QVERIFY(!VehicleProtocol::decode(buffer,messages,error));
        auto invalid=state();invalid["windows"]=QJsonArray{0,1,2};QVERIFY(!VehicleProtocol::validState(invalid));
        invalid=state();invalid["speed"]=40;QVERIFY(!VehicleProtocol::validState(invalid));
    }
    void shutdownWithPendingRequest() {
        QLocalServer server;QVERIFY(server.listen(name()));auto link=std::make_unique<VehicleLink>();link->setServerName(server.serverName());link->setEnabled(true);
        QTRY_VERIFY(server.hasPendingConnections());auto *peer=server.nextPendingConnection();send(peer,{{"type","hello"},{"session","session-a"}});send(peer,snapshot(1));QTRY_VERIFY(link->ready());
        link->request("setWindow",{{"index",0},{"target",100}},"window");QCOMPARE(link->pendingCount(),1);
        link.reset();QTRY_COMPARE(peer->state(),QLocalSocket::UnconnectedState);
    }
};
QTEST_GUILESS_MAIN(VehicleLinkTests)
#include "VehicleLinkTests.moc"
