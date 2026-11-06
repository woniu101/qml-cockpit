import QtQuick
import QtQuick.Controls
import QtQuick.Window
ApplicationWindow {
    id: shell
    width: Math.max(1120,Math.min(1600,Screen.desktopAvailableWidth-32))
    height: Math.max(630,Math.min(900,Screen.desktopAvailableHeight-64))
    minimumWidth: 1120; minimumHeight: 630
    visible: true; color: Theme.background
    title: "QML Cockpit · 智能座舱"
    font.family: "Microsoft YaHei UI"
    property int page: AppRoutes.Home
    property string popup: ""
    readonly property bool modalOpen: popup !== "" && popup !== "assistant"
    Binding { target: AssistantWake; property: "uiBlocked"; value: shell.modalOpen }
    property string assistantSettingsReturn: "settings"
    property string pendingAssistantExit: ""
    property bool assistantDraftPrompt: false
    property string toastMessage: ""
    property string toastKind: "info"
    property string lastNotification: ""
    property var feedbackQueue: []
    property string clockText: "16:28"
    property string dateText: "11月2日 周一"
    property string arrivalText: "16:46"
    readonly property string profileSurname: "周"
    readonly property string profileDisplayName: profileSurname + "先生"
    property string greetingText: "下午好，" + profileDisplayName
    property bool testMode: false
    property double ignoreDemoInputUntil: 0
    property bool pendingDrivingView: false
    property bool sourceWasReady: false
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
    function showPage(index){if(page===AppRoutes.MediaCenter&&index!==AppRoutes.MediaCenter)Media.videoPageLeft();page=index;popup=""}
    function showMedia(kind){showPage(AppRoutes.MediaCenter);mediaPage.openSource(kind)}
    function openPopup(name) {
        if (popup === "assistantSettings" && name !== "assistantSettings") {
            pendingAssistantExit = name
            if (AssistantVoice.dirty || AssistantAppearance.preview.toString() !== "") {
                AssistantVoice.cancelRecording()
                AssistantSpeech.cancel()
                assistantDraftPrompt = true
                return
            }
            finishAssistantExit(true)
            return
        }
        Media.fullscreen = false
        popup = name
    }
    function finishAssistantExit(keep) {
        AssistantVoice.cancelRecording()
        AssistantSpeech.cancel()
        AssistantVoice.stop()
        if (!keep) {
            AssistantVoice.discardDraft()
            AssistantAppearance.discard()
        }
        assistantDraftPrompt = false
        popup = pendingAssistantExit
    }
    function closeAssistant() {
        AssistantVoice.stop()
        popup = ""
    }
    function openAssistantSettings(from) {
        if (!Vehicle.parked) return
        assistantSettingsReturn = from
        openPopup("assistantSettings")
    }
    function returnFromAssistantSettings() {
        if (assistantSettingsReturn === "assistant") Assistant.clearFeedback()
        openPopup(assistantSettingsReturn)
    }
    Connections {
        target: AssistantWake
        function onActivated() { shell.openPopup("assistant") }
    }
    Connections {
        target: Assistant
        function onViewRequested(source) {
            shell.showMedia(source)
            shell.notify(Assistant.message)
        }
    }
    function inspectPart(part){showPage(AppRoutes.Vehicle);vehiclePage.inspectPart(part)}
    function navigateTo(destination){if(Vehicle.startNavigation(destination)){page=1;popup=""}}
    function enterDriving(){pendingDrivingView=true;if(Vehicle.gear!=="D"||Vehicle.speed<=0)Vehicle.startDriving();if(Vehicle.externalReady&&Vehicle.gear==="D"&&Vehicle.speed>0){pendingDrivingView=false;showPage(AppRoutes.Driving)}}
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
        const greeting = hour < 6 ? "夜深了" : hour < 12 ? "上午好" : hour < 18 ? "下午好" : "晚上好"
        greetingText = greeting + "，" + profileDisplayName
        arrivalText=Qt.formatTime(new Date(now.getTime()+Vehicle.remainingMinutes*60000),"hh:mm")
    }
    Component.onCompleted: updateClock()
    Shortcut { sequence: "F11"; onActivated: shell.toggleFullScreen() }
    Shortcut { sequence: "Escape"; onActivated: {if(Media.fullscreen)Media.fullscreen=false;else if(shell.assistantDraftPrompt)shell.assistantDraftPrompt=false;else if(shell.popup==="assistant")shell.closeAssistant();else if(shell.popup!=="")shell.openPopup("");else if(demo.running)demo.stop();else if(shell.visibility===Window.FullScreen)shell.visibility=Window.Windowed} }
    Timer { interval: 1000; repeat: true; running: true; onTriggered: {shell.updateClock();if(!shell.testMode&&!(demo.running&&demo.paused))Vehicle.advanceSimulation(1)} }
    Timer { id: toastTimer; onTriggered: {shell.toastMessage="";if(shell.feedbackQueue.length){const next=shell.feedbackQueue[0];shell.feedbackQueue=shell.feedbackQueue.slice(1);shell.notify(next.message,next.kind)}} }
    DemoController { id: demo; shell: shell }
    Connections {
        target: Vehicle
        function onToast(message,kind){if(kind==="warning")shell.pendingDrivingView=false;shell.notify(message,kind)}
        function onChanged(){
            if (!Vehicle.parked && shell.popup === "assistantSettings") {
                AssistantVoice.cancelRecording()
                AssistantSpeech.cancel()
                AssistantVoice.stop()
                shell.assistantDraftPrompt = false
                shell.popup = ""
            }
            if(shell.pendingDrivingView&&Vehicle.externalReady&&Vehicle.gear==="D"&&Vehicle.speed>0){shell.pendingDrivingView=false;shell.showPage(AppRoutes.Driving)}
        }
        function onVisualReset(){shell.toastMessage="";shell.feedbackQueue=[];toastTimer.stop()}
    }
    Connections { target: VehicleConnection; function onStatusChanged(){if(shell.sourceWasReady&&Vehicle.externalMode&&!VehicleConnection.ready){shell.pendingDrivingView=false;shell.notify("车辆数据连接中断，控制已暂停","warning")}shell.sourceWasReady=VehicleConnection.ready} }
    Item {
        id: canvas; objectName: "designCanvas"
        width: 1600; height: 900; anchors.centerIn: parent; scale: Math.min(shell.width/1600,shell.height/900)
        Rectangle { anchors.fill: parent; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop {position:0;color:Theme.top} GradientStop {position:1;color:Theme.bottom} } }
        HomePage { shell: shell; visible: shell.page===AppRoutes.Home }
        NavigationPage { shell: shell; visible: shell.page===AppRoutes.Navigation }
        VehiclePage { id: vehiclePage; objectName: "vehiclePage"; shell: shell; visible: shell.page===AppRoutes.Vehicle }
        MediaPage { id:mediaPage; shell:shell; visible:shell.page===AppRoutes.MediaCenter; z:Media.fullscreen?50:0 }
        DrivingPage { shell: shell; visible: shell.page===AppRoutes.Driving }
        StatusBar { shell: shell; z: 5 }
        BottomBar { y: 790; shell: shell; z: 10 }
        AssistantPanel { shell: shell; z: 15 }
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
