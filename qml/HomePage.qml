import QtQuick
Item {
    id: home
    required property var shell
    width: 1600; height: 790
    property bool modelHintOffered: false
    property bool modelHintActive: false
    function offerModelHint() {
        if (!modelHintOffered && home.visible && homeScene.ready && shell.popup==="" && !shell.demoRunning) {
            modelHintOffered=true; modelHintActive=true; modelHintTimer.restart()
        }
    }
    function dismissModelHint() { modelHintActive=false; modelHintTimer.stop() }
    onVisibleChanged: { if (visible) offerModelHint(); else dismissModelHint() }
    Connections {
        target: home.shell
        function onPopupChanged() { if (home.shell.popup!=="") home.dismissModelHint(); else home.offerModelHint() }
        function onDemoRunningChanged() { if (home.shell.demoRunning) home.dismissModelHint(); else home.offerModelHint() }
    }
    Timer { id: modelHintTimer; interval: 4500; onTriggered: home.dismissModelHint() }
    Label { x: 56; y: 87; text: home.shell.greetingText; font.pixelSize: 34; font.weight: Font.Medium }
    Label { objectName: "homeDate"; x: 57; y: 141; text: shell.dateText; font.pixelSize: 19; color: Theme.muted }
    UiButton { objectName: "homeDriving"; x: 817; y: 150; width: 237; height: 44; text: "进入行驶演示"; textSize: 18; iconName: "car"; onClicked: home.shell.enterDriving() }
    Label { id: batteryValue; objectName: "homeBatteryValue"; x: 59; y: 192; text: EnergyVisual.wholeSocValue; font.pixelSize: 50; color: EnergyVisual.readingColor }
    Label { x: 63+batteryValue.implicitWidth; y: 222; text: "%"; visible: Vehicle.batteryKnown; font.pixelSize: 24; color: Theme.muted }
    Label { x: 59; y: 259; text: "电量 · "+EnergyVisual.levelText; font.pixelSize: 16; color: EnergyVisual.levelColor }
    Rectangle { x: 196; y: 207; width: 1; height: 71; color: Theme.line }
    Label { x: 232; y: 192; text: Vehicle.batteryKnown?Vehicle.rangeKm:"—"; font.pixelSize: 50 }
    Label { x: 340; y: 222; text: "km"; visible: Vehicle.batteryKnown; font.pixelSize: 23; color: Theme.muted }
    Label { x: 234; y: 259; text: EnergyVisual.realSystem?"估算续航":"预计续航"; font.pixelSize: 18; color: Theme.muted }
    Icon { x: 458; y: 255; width: 21; height: 21; name: "lock"; tint: Theme.green }
    Label { x: 493; y: 254; text: Vehicle.locked ? "车辆已锁定" : "车辆已解锁"; font.pixelSize: 18; color: Theme.green }
    Repeater {
        model: [{ name: "lock", label: "车锁" }, { name: "trunk", label: "后备厢" }, { name: "charge", label: "充电口" }]
        Item {
            required property var modelData; required property int index
            x: 716 + index * 118; y: 204; width: 72; height: 100
            UiButton { objectName: "homeQuick" + index; width: 72; height: 62; enabled: index!==1 || !Vehicle.hatchBusy; iconName: modelData.name; selected: index === 0 ? !Vehicle.locked : index === 1 ? Vehicle.hatchOpen : Vehicle.portOpen; onClicked: { if (index === 0) Vehicle.toggleLock(); else if (index === 1) Vehicle.toggleHatch(); else if(Vehicle.chargeControlAvailable)Vehicle.togglePort(); else home.shell.inspectPart("port") } }
            Label { y: 74; width: parent.width; text: index===1 && Vehicle.hatchBusy ? "执行中" : modelData.label; font.pixelSize: 17; color: Theme.muted; horizontalAlignment: Text.AlignHCenter }
        }
    }
    VehicleScene {
        id: homeScene
        activeView: home.visible; objectName: "homeVehicleScene"
        interactive: !home.shell.modalOpen
        x: 25; y: 300; width: 1050; height: 472; energyDetail: "detailed"
        framingOffset: Qt.point(40,-45)
        onReadyChanged: home.offerModelHint()
        onInteracted: home.dismissModelHint()
        onTapped: home.shell.showPage(AppRoutes.Vehicle)
    }
    Label {
        x: 60; y: 730; width: 995
        text: "拖动旋转 · 滚轮/双指缩放"; font.pixelSize: 15; color: Theme.muted
        horizontalAlignment: Text.AlignHCenter
        opacity: home.modelHintActive?1:0; visible: opacity>0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }
    HomeEnergyRow { objectName: "homeEnergyRow"; shell: home.shell; x: 59; y: 309; width: 995 }
    Surface {
        x: 1110; y: 92; width: 438; height: 285
        Icon { x: 28; y: 29; width: 27; height: 27; name: "search"; tint: Theme.green }
        Label { x: 71; y: 24; text: "想去哪里？"; font.pixelSize: 27; font.weight: Font.Medium }
        MouseArea { x: 20; y: 15; width: 395; height: 65; onClicked: home.shell.openPopup("destination") }
        Label { x: 28; y: 78; text: "常用目的地"; font.pixelSize: 17; color: Theme.muted }
        Repeater {
            model: [{ icon: "home", label: "回家", time: Vehicle.routePreview("回家").minutes+" 分钟" }, { icon: "briefcase", label: "公司", time: Vehicle.routePreview("公司").minutes+" 分钟" }]
            UiButton {
                required property var modelData; required property int index
                objectName: "destination" + index; x: 28 + index * 198; y: 117; width: 184; height: 80
                background: Rectangle { radius: 18; color: parent.down ? Theme.active : Theme.secondary }
                contentItem: Item {
                    Icon { x: 8; y: 8; name: modelData.icon; tint: Theme.green }
                    Label { x: 46; y: 0; text: modelData.label; font.pixelSize: 21; font.weight: Font.Medium }
                    Label { x: 46; y: 34; text: modelData.time; font.pixelSize: 17; color: Theme.muted }
                }
                onClicked: home.shell.navigateTo(modelData.label)
            }
        }
        Rectangle { x: 29; y: 220; width: 380; height: 1; color: Theme.line }
        UiButton { objectName: "homeNearbyStations"; x: 15; y: 226; width: 405; height: 50; quiet: true; text: "附近充电站"; iconName: "charge"; onClicked: home.shell.openPopup("charging") }
    }
    MediaCard { x: 1110; y: 397; shell: home.shell }
    Surface {
        x: 1110; y: 652; width: 438; height: 110
        Label { x: 28; y: 12; text: "场景模式"; font.pixelSize: 21; font.weight: Font.Medium }
        Row {
            x: 28; y: 44; spacing: 14
            Repeater {
                model: [{ key: "rest", label: "小憩", icon: "moon" }, { key: "camp", label: "露营", icon: "tent" }]
                UiButton {
                    id: sceneButton
                    required property var modelData
                    objectName: modelData.key + "Scene"
                    width: 184; height: 50; textSize: 19
                    text: modelData.label; iconName: modelData.icon
                    selected: Vehicle.scene === modelData.key
                    Accessible.name: selected ? text + "，已开启" : text
                    background: Rectangle {
                        radius: 16
                        color: sceneButton.selected || sceneButton.down ? Theme.active : Theme.secondary
                        border.width: sceneButton.selected || sceneButton.visualFocus ? 1 : 0
                        border.color: Theme.green
                    }
                    onClicked: Vehicle.setScene(selected ? "comfort" : modelData.key)
                }
            }
        }
    }
}
