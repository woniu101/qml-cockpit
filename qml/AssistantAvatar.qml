import QtQuick
import QtQuick.Effects

Item {
    id: avatar
    property bool active: false
    property bool forceShown: false
    property url picture: AssistantAppearance.avatar
    property bool custom: AssistantAppearance.custom
    property string expression: "idle"
    property bool previewMotion: false
    property rect cropRect: Qt.rect(0, 0, 0, 0)
    implicitWidth: 44
    implicitHeight: width
    readonly property bool showPicture: forceShown || AssistantAppearance.shown
    readonly property bool stateActive: active || expression === "listening" || expression === "working" || expression === "speaking"
    readonly property color stateColor: expression === "error" ? "#a65c49" : Theme.green
    readonly property bool animateFace: visible && showPicture && (!forceShown || previewMotion) && !custom
                                       && width >= 43 && !AssistantAppearance.reducedMotion

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.active
        border.width: avatar.stateActive || avatar.expression === "success" || avatar.expression === "error" ? 2 : 1
        border.color: avatar.stateActive || avatar.expression === "success" || avatar.expression === "error" ? avatar.stateColor : Theme.line
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: width / 2
        color: Theme.active
        visible: avatar.showPicture
        layer.enabled: true
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: ShaderEffectSource {
                sourceItem: Rectangle {
                    width: avatar.width - 6
                    height: avatar.height - 6
                    radius: width / 2
                    color: "white"
                }
                hideSource: true
                visible: false
            }
        }
        Image {
            anchors.fill: parent
            source: avatar.picture
            sourceClipRect: avatar.custom ? avatar.cropRect : Qt.rect(156, 24, 714, 714)
            fillMode: Image.PreserveAspectCrop
            cache: !avatar.custom
            smooth: true
            mipmap: true
        }
        Item {
            id: face
            anchors.fill: parent
            visible: avatar.animateFace
            property bool blink: false
            Repeater {
                model: [0.345, 0.635]
                Rectangle {
                    required property real modelData
                    visible: face.blink
                    width: face.width * 0.16; height: face.height * 0.122
                    x: face.width * modelData - width / 2; y: face.height * 0.612 - height / 2
                    radius: width / 2
                    color: "#f5e1d2"
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 0.75; height: Math.max(1, parent.height * 0.16)
                        radius: height / 2; color: "#3c2c27"
                    }
                }
            }
            SequentialAnimation {
                running: face.visible && avatar.expression !== "speaking"
                loops: Animation.Infinite
                PauseAnimation { duration: avatar.expression === "listening" ? 3200 : 5200 }
                PropertyAction { target: face; property: "blink"; value: true }
                PauseAnimation { duration: 130 }
                PropertyAction { target: face; property: "blink"; value: false }
                onStopped: face.blink = false
            }
            Rectangle {
                id: mouth
                visible: avatar.expression === "speaking" || avatar.expression === "error"
                width: face.width * 0.058; height: face.height * 0.057
                x: face.width * 0.494 - width / 2; y: face.height * 0.731 - height / 2
                radius: width / 2; color: "#5a382d"
                scale: 0.65
                SequentialAnimation on scale {
                    running: mouth.visible && face.visible && avatar.expression === "speaking"
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.55; to: 1.0; duration: 125 }
                    NumberAnimation { from: 1.0; to: 0.55; duration: 155 }
                }
            }
        }
    }
    Icon {
        anchors.centerIn: parent
        width: parent.width * 0.58
        height: width
        name: "mic"
        tint: Theme.green
        visible: !avatar.showPicture
    }
    Rectangle {
        visible: avatar.stateActive && !AssistantAppearance.reducedMotion
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: avatar.stateColor
        border.width: 1
        SequentialAnimation on opacity {
            running: parent.visible
            loops: Animation.Infinite
            NumberAnimation { from: 0.9; to: 0.22; duration: 700 }
            NumberAnimation { from: 0.22; to: 0.9; duration: 700 }
        }
    }
}
