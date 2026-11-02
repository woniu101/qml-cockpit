import QtQuick
import QtQuick.Controls
Slider {
    id: control
    implicitHeight: 40
    padding: 10
    background: Rectangle {
        x: control.leftPadding; y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth; height: 4; radius: 2; color: Theme.line
        Rectangle { width: control.visualPosition * parent.width; height: 4; radius: 2; color: Theme.green }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.pressed ? 24 : 20; height: width; radius: width / 2; color: Theme.green
    }
}
