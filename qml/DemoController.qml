import QtQuick
Item {
    id: demo
    required property var shell
    property bool running: false
    property bool paused: false
    property int step: 0
    property real rate: 1
    property var saved: ({})
    property var savedMedia: ({})
    property var savedPlayback: ({})
    property int savedPage: 0
    function start(){
        if(Vehicle.sourceMode!=="internal"){shell.notify("自动演示请切换到内置数据源","warning");return}
        if(running)return
        saved=Vehicle.snapshot();savedMedia=MediaState.snapshot();savedPlayback=Media.snapshot();savedPage=shell.page
        Vehicle.setDemoActive(true);MediaState.setDemoActive(true);Media.stop();Media.useMusic()
        Vehicle.resetRuntime();MediaState.resetRuntime();step=0;paused=false;running=true;shell.popup="";shell.ignoreDemoInputUntil=Date.now()+400
        perform()
    }
    function stop(){
        if(!running)return
        running=false;paused=false;Vehicle.restoreSnapshot(saved);MediaState.restoreSnapshot(savedMedia);Media.restoreSnapshot(savedPlayback);Vehicle.setDemoActive(false);MediaState.setDemoActive(false);shell.page=savedPage;shell.popup="";shell.notify("演示已结束，已恢复进入前的车辆状态","info")
    }
    function next(){if(!running){start();paused=true;return}paused=true;step++;if(step>=12)stop();else perform()}
    function perform(){
        switch(step){
        case 0:shell.page=AppRoutes.Home;break
        case 1:shell.page=AppRoutes.MediaCenter;MediaState.playing=true;break
        case 2:shell.navigateTo("公司");break
        case 3:shell.enterDriving();Vehicle.advanceSimulation(180);break
        case 4:Vehicle.parkVehicle();shell.page=AppRoutes.Vehicle;break
        case 5:shell.inspectPart("window0");Vehicle.setWindow(0,35);break
        case 6:Vehicle.closeWindows();shell.inspectPart("door1");Vehicle.setDoor(1,true);break
        case 7:Vehicle.closeDoors();shell.inspectPart("hatch");Vehicle.toggleHatch();break
        case 8:if(Vehicle.hatchOpen)Vehicle.toggleHatch();Vehicle.toggleMirrors();shell.inspectPart("mirrors");break
        case 9:shell.inspectPart("port");if(!Vehicle.portOpen)Vehicle.togglePort();Vehicle.togglePlug();Vehicle.startCharging();break
        case 10:Vehicle.advanceSimulation(300);break
        case 11:Vehicle.stopCharging();if(Vehicle.plugged)Vehicle.togglePlug();if(Vehicle.portOpen)Vehicle.togglePort();shell.page=AppRoutes.Home;break
        }
    }
    Timer { interval: Math.round(3500/demo.rate); repeat: true; running: demo.running&&!demo.paused; onTriggered: {demo.step++;if(demo.step>=12)demo.stop();else demo.perform()} }
}
