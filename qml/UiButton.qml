import QtQuick
import QtQuick.Controls
Button {
    id: control
    property string iconName: ""
    property string caption: ""
    property bool selected: false
    property bool primary: false
    property bool quiet: false
    property bool vertical: false
    property int textSize: 21
    property color foreground: primary ? "#ffffff" : selected ? Theme.green : Theme.ink
    implicitWidth: 120; implicitHeight: 54
    padding: 10
    hoverEnabled: true
    opacity: enabled ? 1 : 0.62
    Accessible.name: text || caption || iconName
    background: Rectangle {
        radius: Math.min(22, height / 2)
        color: control.primary ? "#557a65" : control.selected ? Theme.active : control.quiet ? "transparent" : Theme.surface
        Rectangle { anchors.fill: parent; radius: parent.radius; color: control.down ? "#152e17" : "#638a68"; opacity: control.down ? 0.12 : control.hovered ? 0.06 : 0 }
    }
    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: 12
            visible: !control.vertical
            Icon { name: control.iconName; tint: control.foreground; width: 25; height: 25; visible: control.iconName !== ""; anchors.verticalCenter: parent.verticalCenter }
            Label { text: control.text; color: control.foreground; font.pixelSize: control.textSize; visible: text !== ""; anchors.verticalCenter: parent.verticalCenter }
        }
        Column {
            anchors.centerIn: parent; spacing: 7; visible: control.vertical
            Icon { name: control.iconName; tint: control.foreground; width: 28; height: 28; anchors.horizontalCenter: parent.horizontalCenter }
            Label { text: control.text; color: control.foreground; font.pixelSize: control.textSize; anchors.horizontalCenter: parent.horizontalCenter }
        }
    }
    ToolTip.visible: hovered && caption !== ""
    ToolTip.text: caption
    ToolTip.delay: 650
}
