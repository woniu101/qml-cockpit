import QtQuick
import QtQuick.Effects

Item {
    id: avatar
    property bool active: false
    property bool forceShown: false
    property url picture: AssistantAppearance.avatar
    property bool custom: AssistantAppearance.custom
    implicitWidth: 44
    implicitHeight: width
    readonly property bool showPicture: forceShown || AssistantAppearance.shown

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.active
        border.width: avatar.active ? 2 : 1
        border.color: avatar.active ? Theme.green : Theme.line
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
            sourceClipRect: avatar.custom ? Qt.rect(0, 0, 0, 0) : Qt.rect(156, 24, 714, 714)
            fillMode: Image.PreserveAspectCrop
            cache: !avatar.custom
            smooth: true
            mipmap: true
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
        visible: avatar.active && !AssistantAppearance.reducedMotion
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: Theme.green
        border.width: 1
        SequentialAnimation on opacity {
            running: parent.visible
            loops: Animation.Infinite
            NumberAnimation { from: 0.9; to: 0.22; duration: 700 }
            NumberAnimation { from: 0.22; to: 0.9; duration: 700 }
        }
    }
}
