import QtQuick
Image {
    property string name: "home"
    property color tint: Theme.ink
    width: 28; height: 28
    source: name.length > 0 ? "image://icons/" + name + "/" + tint.toString().replace("#", "") : ""
    sourceSize: Qt.size(width * 2, height * 2)
    fillMode: Image.PreserveAspectFit
    smooth: true
}
