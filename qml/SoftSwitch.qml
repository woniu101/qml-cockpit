import QtQuick
import QtQuick.Controls as Controls

Controls.Switch {
    id: control
    implicitHeight: 44
    leftPadding: 66; rightPadding: 4
    indicator: Rectangle {
        x: 0; y: (control.height - height) / 2
        width: 52; height: 30; radius: 15
        color: control.checked ? Theme.green : Theme.line
        opacity: control.enabled ? 1 : 0.5
        Rectangle {
            x: control.checked ? 25 : 3; y: 3
            width: 24; height: 24; radius: 12; color: "#ffffff"
            Behavior on x { NumberAnimation { duration: 130 } }
        }
    }
    contentItem: Label {
        text: control.text; font.pixelSize: 18
        verticalAlignment: Text.AlignVCenter
        color: control.enabled ? Theme.ink : Theme.muted
    }
}
