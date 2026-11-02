import QtQuick
Item {
    required property var shell
    width: 1600; height: 64
    Item {
        objectName: "statusLeftGroup"; x: 40; width: 302; height: 64
        // Preserve the shared alignment box; padding shifts the glyphs 24px right and 2px down.
        Label { objectName: "statusClock"; width: 112; height: 64; leftPadding: 24; topPadding: 4; verticalAlignment: Text.AlignVCenter; text: shell.clockText; font.pixelSize: 26; font.weight: Font.Normal; font.features: {"tnum": 1}; color: Theme.muted }
        Rectangle { x: 128; y: 22; width: 1; height: 20; color: Theme.line }
        Icon { x: 150; y: 21; width: 22; height: 22; name: Vehicle.night?"moon":"sun"; tint: Theme.green }
        Label { x: 184; width: 46; height: 64; verticalAlignment: Text.AlignVCenter; text: "24°"; font.pixelSize: 20; color: Theme.muted }
        Label { x: 244; width: 58; height: 64; verticalAlignment: Text.AlignVCenter; text: "上海"; font.pixelSize: 20; color: Theme.muted }
    }
    UiButton {
        objectName: "simulatorButton"; x: 370; y: 10; width: 174; height: 44
        text: Vehicle.gear+" · "+(Vehicle.parked?"驻车":Vehicle.speed+" km/h"); selected: true; iconName: "car"; textSize: 17
        caption: "驾驶演示与车速"; onClicked: shell.openPopup("driving")
    }
    Label { objectName: "statusBattery"; x: 1156; width: 52; height: 64; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: Math.round(Vehicle.batteryPercent)+"%"; font.pixelSize: 19; color: Theme.muted }
    Rectangle { x: 1220; y: 24; width: 36; height: 16; radius: 4; color: "transparent"; border.color: Theme.muted
        Rectangle { x: 3; y: 3; width: 30*Vehicle.batteryPercent/100; height: 10; radius: 2; color: Theme.green }
        Rectangle { x: 39; y: 5; width: 3; height: 6; radius: 1; color: Theme.muted }
    }
    Repeater {
        model: ["bluetooth","wifi","signal"]
        Item { required property string modelData; required property int index; x: 1276+index*52; y: 10; width: 44; height: 44
            Icon { anchors.centerIn: parent; width: 24; height: 24; name: modelData; tint: Theme.muted }
        }
    }
    UiButton {
        objectName: "notificationsButton"; x: 1432; y: 10; width: 44; height: 44; quiet: true; iconName: "bell"; caption: "当前车辆提醒"; onClicked: shell.openPopup("notifications")
        Rectangle { visible: Vehicle.notifications.length>0; x: 29; y: -1; width: 22; height: 19; radius: 9.5; color: Vehicle.warningCount>0?"#ac7638":"#557a65"
            Label { anchors.centerIn: parent; text: Vehicle.notifications.length>9?"9+":Vehicle.notifications.length; font.pixelSize: 12; color: "white" }
        }
    }
    Rectangle { x: 1493; y: 22; width: 1; height: 20; color: Theme.line }
    UiButton { objectName: "settingsButton"; x: 1512; y: 10; width: 44; height: 44; selected: true; text: "林"; textSize: 20; caption: "显示与演示设置"; onClicked: shell.openPopup("settings") }
}
