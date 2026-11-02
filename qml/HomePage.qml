import QtQuick
Item {
    id: home
    required property var shell
    width: 1600; height: 790
    Label { x: 56; y: 87; text: home.shell.greetingText; font.pixelSize: 34; font.weight: Font.Medium }
    Label { x: 57; y: 141; text: shell.dateText + " · " + (Vehicle.parked ? (Vehicle.navigationActive?"已规划路线 · 仍在驻车":"车辆已就绪") : "行驶演示中"); font.pixelSize: 19; color: Theme.muted }
    UiButton { objectName: "homeDriving"; x: 817; y: 150; width: 237; height: 44; text: "进入行驶演示"; textSize: 18; iconName: "car"; onClicked: home.shell.enterDriving() }
    Label { id: batteryValue; x: 59; y: 192; text: Math.round(Vehicle.batteryPercent); font.pixelSize: 50 }
    Label { x: 63+batteryValue.implicitWidth; y: 222; text: "%"; font.pixelSize: 24; color: Theme.muted }
    Label { x: 59; y: 259; text: "电池电量"; font.pixelSize: 18; color: Theme.muted }
    Rectangle { x: 196; y: 207; width: 1; height: 71; color: Theme.line }
    Label { x: 232; y: 192; text: Vehicle.rangeKm; font.pixelSize: 50 }
    Label { x: 340; y: 222; text: "km"; font.pixelSize: 23; color: Theme.muted }
    Label { x: 234; y: 259; text: "预计续航"; font.pixelSize: 18; color: Theme.muted }
    Icon { x: 458; y: 255; width: 21; height: 21; name: "lock"; tint: Theme.green }
    Label { x: 493; y: 254; text: Vehicle.locked ? "车辆已锁定" : "车辆已解锁"; font.pixelSize: 18; color: Theme.green }
    Repeater {
        model: [{ name: "lock", label: "车锁" }, { name: "trunk", label: "后备厢" }, { name: "charge", label: "充电口" }]
        Item {
            required property var modelData; required property int index
            x: 716 + index * 118; y: 204; width: 72; height: 100
            UiButton { objectName: "homeQuick" + index; width: 72; height: 62; enabled: index!==1 || !Vehicle.hatchBusy; iconName: modelData.name; selected: index === 0 ? !Vehicle.locked : index === 1 ? Vehicle.hatchOpen : Vehicle.portOpen; onClicked: { if (index === 0) Vehicle.toggleLock(); else if (index === 1) Vehicle.toggleHatch(); else Vehicle.togglePort() } }
            Label { y: 74; width: parent.width; text: index===1 && Vehicle.hatchBusy ? "执行中" : modelData.label; font.pixelSize: 17; color: Theme.muted; horizontalAlignment: Text.AlignHCenter }
        }
    }
    VehicleScene { activeView: home.visible; objectName: "homeVehicleScene"; x: 25; y: 300; width: 1050; height: 472
        onTapped: home.shell.showPage(2)
    }
    Label { x: 60; y: 748; text: "3D 实车 · 拖动旋转 / 滚轮缩放"; font.pixelSize: 16; color: Theme.muted }
    Rectangle {
        visible: Vehicle.doors.some(v => v) || Vehicle.hoodOpen || Vehicle.hatchOpen || Vehicle.portOpen || Vehicle.windows.some(v => v > 0)
        x: 58; y: 315; width: 780; height: 36; radius: 14; color: Theme.surface
        Label { x: 16; anchors.verticalCenter: parent.verticalCenter; width: parent.width-32; elide: Text.ElideRight; font.pixelSize: 17; color: Theme.green; text: Vehicle.busy ? "车辆部件正在调整…" : [Vehicle.doors.some(v => v) ? "车门已开启" : "", Vehicle.hoodOpen ? "前备厢已开启" : "", Vehicle.hatchOpen ? "后备厢已开启" : "", Vehicle.portOpen ? "充电口已开启" : "", Vehicle.windows.some(v => v > 0) ? "车窗已开启" : ""].filter(v => v !== "").join(" · ") }
    }
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
        UiButton { x: 15; y: 226; width: 405; height: 50; quiet: true; text: "附近充电站"; iconName: "charge"; onClicked: home.shell.openPopup("charging") }
    }
    MusicCard { x: 1110; y: 397; shell: home.shell }
    Surface {
        x: 1110; y: 652; width: 438; height: 110
        Label { x: 28; y: 32; text: "场景模式"; font.pixelSize: 19; color: Theme.muted }
        UiButton { objectName: "restScene"; x: 133; y: 24; width: 125; height: 52; text: "小憩"; iconName: "moon"; selected: Vehicle.scene === "rest"; onClicked: Vehicle.setScene(Vehicle.scene === "rest" ? "comfort" : "rest") }
        UiButton { objectName: "campScene"; x: 273; y: 24; width: 135; height: 52; text: "露营"; iconName: "tent"; selected: Vehicle.scene === "camp"; onClicked: Vehicle.setScene(Vehicle.scene === "camp" ? "comfort" : "camp") }
    }
}
