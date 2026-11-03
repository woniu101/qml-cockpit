#include <QtTest>
#include "BluetoothAudio.h"
#include <atomic>
#include <chrono>
#include <thread>

struct FakeReceiverState {
    std::atomic_bool link{false},holdOpen{false},lateSuccess{false},success{true},supported{true},empty{false};
    std::atomic_int opens{0},closes{0};
};
class FakeReceiver:public ReceiverBackend {
    std::shared_ptr<FakeReceiverState> s;
public:
    explicit FakeReceiver(std::shared_ptr<FakeReceiverState> state):s(state){}
    ReceiverScan scan(const Cancelled&) override{return {s->supported,s->empty?QVariantList{}:QVariantList{QVariantMap{{"token","phone"},{"name","test phone"}}},{}};}
    ReceiverOpen open(const QString&,const Cancelled &cancel) override{
        ++s->opens;while(s->holdOpen&&!cancel())std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if(cancel()){if(s->lateSuccess){s->link=true;return {true,{}};}return {};}
        s->link=s->success.load();return {s->link,s->link?QString():QStringLiteral("timeout fixture")};
    }
    bool connected() override{return s->link;}
    void close() noexcept override{s->link=false;++s->closes;}
};
class BluetoothAudioTests:public QObject {
    Q_OBJECT
    static BluetoothAudio::Factory factory(const std::shared_ptr<FakeReceiverState> &s){return [s]{return std::make_unique<FakeReceiver>(s);};}
private slots:
    void explicitConnectAndNoAutoReconnect(){
        auto s=std::make_shared<FakeReceiverState>();BluetoothAudio receiver(nullptr,factory(s));
        QTRY_VERIFY(receiver.supported()&&!receiver.scanning());QCOMPARE(receiver.devices().size(),1);QCOMPARE(s->opens,0);
        QVERIFY(!receiver.connectDevice("missing"));QVERIFY(receiver.connectDevice("phone"));QVERIFY(receiver.active());QVERIFY(!receiver.connectDevice("phone"));
        QTRY_COMPARE(receiver.phase(),"connected");QVERIFY(!receiver.text().contains(QStringLiteral("正在播放")));
        s->link=false;QTRY_COMPARE(receiver.phase(),"error");QVERIFY(!receiver.active());QTest::qWait(650);QCOMPARE(s->opens,1);
        QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(receiver.phase(),"connected");receiver.disconnectDevice();QTRY_COMPARE(receiver.phase(),"idle");QVERIFY(!s->link);
    }
    void cancelDiscardsLateSuccess(){
        auto s=std::make_shared<FakeReceiverState>();s->holdOpen=true;s->lateSuccess=true;BluetoothAudio receiver(nullptr,factory(s));
        QTRY_VERIFY(receiver.supported()&&!receiver.scanning());QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(s->opens,1);
        receiver.disconnectDevice();QTRY_COMPARE(receiver.phase(),"idle");s->holdOpen=false;QTest::qWait(200);QVERIFY(!receiver.active());QVERIFY(!s->link);
        QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(receiver.phase(),"connected");
    }
    void failureOutputLossAndRecovery(){
        auto s=std::make_shared<FakeReceiverState>();s->success=false;BluetoothAudio receiver(nullptr,factory(s));
        QTRY_VERIFY(receiver.supported()&&!receiver.scanning());QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(receiver.phase(),"error");QVERIFY(!receiver.error().isEmpty());
        s->success=true;receiver.setOutputAvailable(false);QVERIFY(!receiver.connectDevice("phone"));receiver.setOutputAvailable(true);QCOMPARE(s->opens,1);
        QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(receiver.phase(),"connected");receiver.setOutputAvailable(false);QTRY_COMPARE(receiver.phase(),"idle");QVERIFY(!s->link);
        receiver.setOutputAvailable(true);QTest::qWait(200);QCOMPARE(s->opens,2);QVERIFY(!receiver.active());
    }
    void removedCandidateCannotConnect(){
        auto s=std::make_shared<FakeReceiverState>();BluetoothAudio receiver(nullptr,factory(s));
        QTRY_VERIFY(receiver.supported()&&!receiver.scanning());QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(receiver.phase(),"connected");
        s->empty=true;receiver.refresh();QTRY_VERIFY(!receiver.scanning());QTRY_COMPARE(receiver.phase(),"error");QVERIFY(!s->link);QVERIFY(receiver.devices().isEmpty());QVERIFY(!receiver.connectDevice("phone"));
    }
    void unsupportedAndShutdownDuringConnect(){
        auto s=std::make_shared<FakeReceiverState>();s->supported=false;
        {BluetoothAudio receiver(nullptr,factory(s));QTRY_COMPARE(receiver.phase(),"unsupported");QVERIFY(!receiver.connectDevice("phone"));}
        s->supported=true;s->holdOpen=true;
        {BluetoothAudio receiver(nullptr,factory(s));QTRY_VERIFY(receiver.supported()&&!receiver.scanning());QVERIFY(receiver.connectDevice("phone"));QTRY_COMPARE(s->opens,1);}
        QVERIFY(!s->link);QVERIFY(s->closes>0);
    }
};
QTEST_GUILESS_MAIN(BluetoothAudioTests)
#include "BluetoothAudioTests.moc"
