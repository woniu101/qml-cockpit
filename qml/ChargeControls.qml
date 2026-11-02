import QtQuick
Item {
    id: charge
    required property var shell
    Label { x: 24; y: 22; text: "充电与补能"; font.pixelSize: 26; font.weight: Font.Medium }
    Label { x: 24; y: 68; text: Vehicle.chargeStatus; font.pixelSize: 20; color: Theme.green }
    Label { x: 24; y: 105; text: Vehicle.batteryPercent.toFixed(1)+"%"; font.pixelSize: 43 }
    Label { x: 208; y: 127; text: "续航 "+Vehicle.rangeKm+" km"; font.pixelSize: 19; color: Theme.muted }
    Rectangle { x: 24; y: 168; width: 347; height: 9; radius: 4.5; color: Theme.line
        Rectangle { width: parent.width*Vehicle.batteryPercent/100; height: parent.height; radius: 4.5; color: Theme.green }
    }
    Label { x: 24; y: 201; text: "目标电量 "+Vehicle.chargeTarget+"%"; font.pixelSize: 21 }
    SoftSlider { objectName: "chargeTargetSlider"; x: 14; y: 237; width: 367; from: 50; to: 100; stepSize: 1; value: Vehicle.chargeTarget; onMoved: Vehicle.chargeTarget=Math.round(value) }
    Label { x: 24; y: 286; width: 347; text: Vehicle.plugged?(Vehicle.chargePower.toFixed(0)+" kW · 预计 "+Vehicle.chargeMinutes+" 分钟达到目标"):"驻车后打开充电口，再连接充电枪"; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
    UiButton { objectName: "chargePort"; x: 24; y: 332; width: 347; height: 47; text: Vehicle.portOpen?"关闭充电口":"打开充电口"; iconName: "charge"; enabled: !Vehicle.plugged; onClicked: Vehicle.togglePort() }
    UiButton { objectName: "chargePlug"; x: 24; y: 392; width: 347; height: 47; text: Vehicle.plugged?"断开充电枪":"连接充电枪"; enabled: Vehicle.portOpen&&!Vehicle.charging; onClicked: Vehicle.togglePlug() }
    UiButton { objectName: "chargeStart"; x: 24; y: 452; width: 347; height: 52; text: Vehicle.charging?"暂停充电":"开始充电"; primary: !Vehicle.charging; enabled: Vehicle.plugged; onClicked: Vehicle.charging?Vehicle.stopCharging():Vehicle.startCharging() }
    UiButton { objectName: "chargeFastForward"; x: 24; y: 519; width: 167; height: 43; text: "快进 5 分钟"; textSize: 17; enabled: Vehicle.charging; onClicked: Vehicle.advanceSimulation(300) }
    UiButton { x: 204; y: 519; width: 167; height: 43; text: "附近充电站"; textSize: 17; onClicked: charge.shell.openPopup("charging") }
    Label { x: 24; y: 574; text: "离线补能演示 · 55 kW / 60 kWh"; font.pixelSize: 13; color: Theme.muted }
}
