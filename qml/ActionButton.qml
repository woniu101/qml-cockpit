import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool active: false
    property string caption: ""
    implicitWidth: 120
    implicitHeight: 62
    hoverEnabled: true
    background: Rectangle {
        radius: 12
        color: control.active ? "#193d3b" : (control.hovered ? "#25313c" : "#19232e")
        border.color: control.active ? "#71dac6" : "#2c3945"
        Behavior on color { ColorAnimation { duration: 140 } }
    }
    contentItem: Column {
        spacing: 3
        anchors.verticalCenter: parent.verticalCenter
        Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: control.text; color: control.active ? "#a4f5e3" : "#e0e8ed"; font.pixelSize: 15; font.weight: Font.Medium }
        Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: control.caption; color: "#8698a7"; font.pixelSize: 11; visible: text.length > 0 }
    }
}
