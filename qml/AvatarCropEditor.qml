import QtQuick

Item {
    id: editor
    required property url source
    required property size imageSize
    property real zoom: 1
    property real horizontal: 50
    property real vertical: 50
    readonly property real side: Math.min(imageSize.width, imageSize.height) / zoom
    readonly property rect cropRect: Qt.rect((imageSize.width - side) * horizontal / 100,
                                           (imageSize.height - side) * vertical / 100, side, side)
    readonly property real imageScale: width / Math.max(1, side)
    readonly property real overflowX: Math.max(0, imageSize.width * imageScale - width)
    readonly property real overflowY: Math.max(0, imageSize.height * imageScale - height)
    signal committed()
    function resizeCrop(nextZoom) {
        const centerX = cropRect.x + cropRect.width / 2
        const centerY = cropRect.y + cropRect.height / 2
        zoom = Math.max(1, Math.min(3, nextZoom))
        horizontal = imageSize.width > side ? Math.max(0, Math.min(100, (centerX - side / 2) / (imageSize.width - side) * 100)) : 50
        vertical = imageSize.height > side ? Math.max(0, Math.min(100, (centerY - side / 2) / (imageSize.height - side) * 100)) : 50
    }
    clip: true
    Image {
        source: editor.source
        x: -editor.overflowX * editor.horizontal / 100
        y: -editor.overflowY * editor.vertical / 100
        width: editor.imageSize.width * editor.imageScale
        height: editor.imageSize.height * editor.imageScale
        smooth: true; mipmap: true
    }
    Rectangle {
        anchors.fill: parent; radius: width / 2
        color: "transparent"; border.width: 2; border.color: Theme.green
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property point origin
        property real initialHorizontal
        property real initialVertical
        onPressed: mouse => {
            origin = Qt.point(mouse.x, mouse.y)
            initialHorizontal = editor.horizontal
            initialVertical = editor.vertical
        }
        onPositionChanged: mouse => {
            if (!pressed) return
            editor.horizontal = editor.overflowX > 0 ? Math.max(0, Math.min(100, initialHorizontal - (mouse.x - origin.x) / editor.overflowX * 100)) : 50
            editor.vertical = editor.overflowY > 0 ? Math.max(0, Math.min(100, initialVertical - (mouse.y - origin.y) / editor.overflowY * 100)) : 50
        }
        onReleased: editor.committed()
        onCanceled: editor.committed()
        onWheel: wheel => {
            editor.resizeCrop(editor.zoom * Math.pow(1.12, wheel.angleDelta.y / 120))
            editor.committed()
            wheel.accepted = true
        }
    }
    PinchHandler {
        target: null
        minimumPointCount: 2
        property real initialZoom
        onActiveChanged: {
            if (active) initialZoom = editor.zoom
            else editor.committed()
        }
        onActiveScaleChanged: if (active) editor.resizeCrop(initialZoom * activeScale)
    }
}
