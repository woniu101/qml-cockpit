import QtQuick
import QtQuick.Controls
import QtQuick.Window
ApplicationWindow {
    id: shell
    width: Math.max(1120,Math.min(1600,Screen.desktopAvailableWidth-32))
    height: Math.max(630,Math.min(900,Screen.desktopAvailableHeight-64))
    minimumWidth: 1120; minimumHeight: 630
    visible: true; color: Theme.background
    title: "Car Terminal · 智能座舱演示"
    font.family: "Microsoft YaHei UI"
    property int page: 0
    property string popup: ""
    property string toastMessage: ""
    property string toastKind: "info"
    property string lastNotification: ""
    property var feedbackQueue: []
    property string clockText: "16:28"
    property string dateText: "11月2日 周一"
    property string arrivalText: "16:46"
    property string greetingText: "下午好，林先生"
    property bool testMode: false
    property double ignoreDemoInputUntil: 0
    readonly property bool demoRunning: demo.running
    readonly property bool demoPaused: demo.paused
    readonly property int demoStep: demo.step
    function formatTime(ms){const sec=Math.max(0,Math.floor(ms/1000));return Math.floor(sec/60).toString().padStart(2,"0")+":"+(sec%60).toString().padStart(2,"0")}
    function notify(message,kind="info") {
        if(message==="" || (toastMessage===message&&toastKind===kind))return
        if(toastMessage!==""&&toastKind==="warning"&&kind!=="warning"){
            if(!feedbackQueue.some(v=>v.message===message))feedbackQueue=feedbackQueue.concat([{message:message,kind:kind}]).slice(-3)
            return
        }
        toastKind=kind;toastMessage=message;lastNotification=message
        toastTimer.interval=kind==="warning"?4200:kind==="success"?1800:2200;toastTimer.restart()
    }
    function showPage(index){page=index;popup=""}
    function openPopup(name){popup=name}
    function inspectPart(part){showPage(2);vehiclePage.inspectPart(part)}
    function navigateTo(destination){if(Vehicle.startNavigation(destination)){page=1;popup=""}}
    function enterDriving(){if(Vehicle.gear!=="D"||Vehicle.speed<=0)Vehicle.startDriving();if(Vehicle.gear==="D"&&Vehicle.speed>0)showPage(4)}
    function toggleFullScreen(){visibility=visibility===Window.FullScreen?Window.Windowed:Window.FullScreen}
    function toggleDemo(){if(demo.running)demo.stop();else demo.start()}
    function demoPause(){demo.paused=!demo.paused}
    function demoNext(){demo.next()}
    function demoSpeed(value){demo.rate=value}
    function updateClock(){
        if(testMode)return
        const now=new Date(),hour=now.getHours()
        clockText=Qt.formatTime(now,"hh:mm")
        dateText=Qt.formatDate(now,"M月d日")+" "+["周日","周一","周二","周三","周四","周五","周六"][now.getDay()]
        greetingText=hour<6?"夜深了，林先生":hour<12?"上午好，林先生":hour<18?"下午好，林先生":"晚上好，林先生"
        arrivalText=Qt.formatTime(new Date(now.getTime()+Vehicle.remainingMinutes*60000),"hh:mm")
    }
    Component.onCompleted: updateClock()
    Shortcut { sequence: "F11"; onActivated: shell.toggleFullScreen() }
    Shortcut { sequence: "Escape"; onActivated: {if(shell.popup!=="")shell.popup="";else if(demo.running)demo.stop();else if(shell.visibility===Window.FullScreen)shell.visibility=Window.Windowed} }
    Timer { interval: 1000; repeat: true; running: true; onTriggered: {shell.updateClock();if(!shell.testMode&&!(demo.running&&demo.paused))Vehicle.advanceSimulation(1)} }
    Timer { id: toastTimer; onTriggered: {shell.toastMessage="";if(shell.feedbackQueue.length){const next=shell.feedbackQueue[0];shell.feedbackQueue=shell.feedbackQueue.slice(1);shell.notify(next.message,next.kind)}} }
    DemoController { id: demo; shell: shell }
    Connections {
        target: Vehicle
        function onToast(message,kind){shell.notify(message,kind)}
        function onVisualReset(){shell.toastMessage="";shell.feedbackQueue=[];toastTimer.stop()}
    }
    Item {
        id: canvas; objectName: "designCanvas"
        width: 1600; height: 900; anchors.centerIn: parent; scale: Math.min(shell.width/1600,shell.height/900)
        Rectangle { anchors.fill: parent; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop {position:0;color:Theme.top} GradientStop {position:1;color:Theme.bottom} } }
        HomePage { shell: shell; visible: shell.page===0 }
        NavigationPage { shell: shell; visible: shell.page===1 }
        VehiclePage { id: vehiclePage; objectName: "vehiclePage"; shell: shell; visible: shell.page===2 }
        MusicPage { shell: shell; visible: shell.page===3 }
        DrivingPage { shell: shell; visible: shell.page===4 }
        StatusBar { shell: shell; z: 5 }
        BottomBar { y: 790; shell: shell; z: 10 }
        Overlays { shell: shell; z: 20 }
        FeedbackBanner {
            objectName: "feedbackBanner"; z: 30; x: (1600-width)/2; y: 83
            message: shell.toastMessage!==""?shell.toastMessage:Vehicle.globalProgress
            kind: shell.toastMessage!==""?shell.toastKind:"progress"
        }
        Surface {
            visible: demo.running; z: 16; x: 568; y: 10; width: 522; height: 44; radius: 17
            Label { x: 17; y: 12; text: "演示 "+(demo.step+1)+"/12 · "+(demo.paused?"已暂停":"进行中"); font.pixelSize: 18 }
            UiButton { objectName: "demoPauseInline"; x: 212; y: 1; width: 82; height: 42; text: demo.paused?"继续":"暂停"; textSize: 17; quiet: true; onClicked: {shell.ignoreDemoInputUntil=Date.now()+300;demo.paused=!demo.paused} }
            UiButton { objectName: "demoNextInline"; x: 300; y: 1; width: 92; height: 42; text: "下一步"; textSize: 17; quiet: true; onClicked: {shell.ignoreDemoInputUntil=Date.now()+300;demo.next()} }
            UiButton { objectName: "demoStopInline"; x: 398; y: 1; width: 108; height: 42; text: "退出演示"; textSize: 17; quiet: true; onClicked: demo.stop() }
        }
        TapHandler { onTapped: {if(demo.running&&!demo.paused&&Date.now()>shell.ignoreDemoInputUntil&&shell.popup!=="demo"){demo.paused=true;shell.notify("已暂停自动演示，可使用顶部按钮继续","info")}} }
    }
}
