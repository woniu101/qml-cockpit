import QtQuick
Item {
    id: driving
    required property var shell
    enabled: Vehicle.externalReady
    Label { x: 30; y: 27; text: "驾驶演示"; font.pixelSize: 28 }
    Label { x: 30; y: 91; text: Vehicle.gear+" 挡"; font.pixelSize: 43 }
    Label { x: 240; y: 101; text: Vehicle.speed+" km/h"; font.pixelSize: 31 }
    Label { x: 30; y: 151; text: Vehicle.navigationActive?"导航至 "+Vehicle.destination:"自由行驶 · 无需规划路线"; font.pixelSize: 19; color: Theme.muted }
    Row { x: 30; y: 201; spacing: 12; Repeater { model: ["P","R","N","D"]; UiButton { required property string modelData; objectName: "gear"+modelData; width: 121; height: 60; text: modelData; textSize: 27; selected: Vehicle.gear===modelData; onClicked: Vehicle.gear=modelData } } }
    Label { x: 30; y: 290; text: "模拟车速"; font.pixelSize: 22 }
    SoftSlider { objectName: "drivingSpeed"; x: 20; y: 329; width: 540; from: 0; to: Vehicle.gear==="R"?15:100; stepSize: 1; enabled: Vehicle.gear==="D"||Vehicle.gear==="R"; value: Vehicle.speed; onMoved: Vehicle.speed=Math.round(value) }
    UiButton { objectName: "startDriving"; x: 30; y: 400; width: 253; height: 57; primary: true; text: "进入 D 挡行驶演示"; textSize: 19; onClicked: driving.shell.enterDriving() }
    UiButton { objectName: "parkDriving"; x: 297; y: 400; width: 253; height: 57; text: "挂 P 挡驻车"; textSize: 19; onClicked: Vehicle.parkVehicle() }
    UiButton { objectName: "driveFastForward"; x: 30; y: 480; width: 253; height: 48; text: "快进 1 分钟"; textSize: 18; onClicked: Vehicle.advanceSimulation(60) }
    UiButton { x: 297; y: 480; width: 253; height: 48; text: "查看行驶画面"; textSize: 18; onClicked: driving.shell.showPage(4) }
    Label { x: 30; y: 550; width: 520; text: "导航与挡位独立 · 仅为本地车辆信号演示"; font.pixelSize: 16; color: Theme.muted }
}
