import QtQuick
import QtQuick.Controls as Controls

Controls.TextField {
    implicitHeight: 48
    leftPadding: 14; rightPadding: 14
    font.pixelSize: 17
    color: Theme.ink
    placeholderTextColor: Theme.muted
    selectByMouse: true
    background: Rectangle {
        radius: 12
        color: Theme.surface
        border.width: 1
        border.color: parent.activeFocus ? Theme.green : Theme.line
    }
}
