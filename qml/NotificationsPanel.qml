import QtQuick
import QtQuick.Controls
import "." as App

Item {
    id: panel
    required property var shell
    App.Label { x: 30; y: 27; text: "车辆通知"; font.pixelSize: 28 }
    Rectangle {
        x: 30; y: 100; width: 36; height: 36; radius: 18
        color: Vehicle.warningCount>0 ? (Vehicle.night?"#70592e":"#f8e4c1") : Theme.active
        App.Label { anchors.centerIn: parent; visible: Vehicle.warningCount>0; text: "!"; font.pixelSize: 23; font.bold: true; color: Vehicle.night?"#ffcf85":"#8c5d25" }
        Icon { anchors.centerIn: parent; visible: Vehicle.warningCount===0; name: Vehicle.busy?"refresh":Vehicle.notifications.length>0?"bell":"check"; width: 22; height: 22; tint: Theme.green }
    }
    App.Label { objectName: "notificationStatus"; x: 80; y: 99; width: 455; text: Vehicle.statusTitle; font.pixelSize: 24 }
    App.Label { objectName: "notificationEnergy"; x: 80; y: 141; text: (Vehicle.parked?"驻车":"行驶演示")+" · 电量 "+App.EnergyVisual.socText+(App.EnergyVisual.realSystem?" · 估算续航 ":" · 续航 ")+App.EnergyVisual.rangeText; font.pixelSize: 17; color: Theme.muted }
    Flickable {
        x: 30; y: 192; width: 520; height: 338; clip: true
        contentHeight: entries.height; boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            width: 4; policy: ScrollBar.AsNeeded
            contentItem: Rectangle { implicitWidth: 4; implicitHeight: 40; radius: 2; color: Theme.green; opacity: 0.55 }
        }
        Column {
            id: entries
            width: parent.width; spacing: 12
            Repeater {
                model: Vehicle.notifications
                Surface {
                    id: entry
                    required property var modelData
                    objectName: "notification_"+modelData.part
                    width: entries.width; height: 94; radius: 18
                    color: Theme.secondary
                    Icon { x: 15; y: 18; width: 23; height: 23; name: entry.modelData.icon; tint: entry.modelData.severity==="warning"?(Vehicle.night?"#e5bd7b":"#9b6c35"):Theme.green }
                    App.Label { x: 51; y: 13; width: 355; text: entry.modelData.title; font.pixelSize: 20; elide: Text.ElideRight }
                    App.Label { x: 51; y: 47; width: 355; text: entry.modelData.detail; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
                    UiButton { objectName: "inspectNotification_"+entry.modelData.part; x: 426; y: 27; width: 78; height: 40; text: "查看"; textSize: 17; quiet: true; foreground: Theme.green; onClicked: panel.shell.inspectPart(entry.modelData.part) }
                }
            }
        }
    }
    Column {
        visible: Vehicle.notifications.length===0
        x: 30; y: 258; width: 520; spacing: 17
        Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 42; height: 42; name: "check"; tint: Theme.green }
        App.Label { width: parent.width; text: "当前没有待处理提醒"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 22 }
        App.Label { width: parent.width; text: "车门、车窗、前后备厢与充电口均已关闭"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 17; color: Theme.muted }
    }
    App.Label { x: 30; y: 552; width: 520; text: "状态随部件操作实时更新 · 本地演示"; font.pixelSize: 15; color: Theme.muted }
}
