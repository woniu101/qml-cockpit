import QtQuick

Rectangle {
    id: banner
    property string message: ""
    property string kind: "info"
    property string displayedMessage: ""
    property string displayedKind: "info"
    property real spinAngle: 0
    readonly property bool warning: displayedKind === "warning"
    onMessageChanged: if(message!==""){displayedMessage=message;displayedKind=kind}
    onKindChanged: if(message!=="")displayedKind=kind
    width: Math.max(320,Math.min(560,messageLabel.implicitWidth+102))
    height: messageLabel.contentHeight>26?64:52
    radius: 18
    color: warning ? (Vehicle.night?"#493b27":"#fff0d9") : Theme.surface
    border.width: 1
    border.color: warning ? (Vehicle.night?"#80663c":"#e9cf9f") : Theme.line
    opacity: message!=="" ? 1 : 0
    visible: opacity>0
    Behavior on opacity { NumberAnimation { duration: 170 } }
    Accessible.role: Accessible.AlertMessage
    Accessible.name: displayedMessage
    NumberAnimation { target: banner; property: "spinAngle"; from: 0; to: 360; duration: 1000; loops: Animation.Infinite; running: banner.visible && banner.displayedKind==="progress" }
    Rectangle {
        x: 15; anchors.verticalCenter: parent.verticalCenter; width: 28; height: 28; radius: 14
        color: banner.warning ? (Vehicle.night?"#70592e":"#f5dfb8") : Theme.active
        Label { visible: banner.warning; anchors.centerIn: parent; text: "!"; font.pixelSize: 19; font.bold: true; color: Vehicle.night?"#ffcf85":"#8c5d25" }
        Icon {
            id: statusIcon
            visible: !banner.warning; anchors.centerIn: parent; width: 18; height: 18
            name: banner.displayedKind==="progress"?"refresh":banner.displayedKind==="success"?"check":"bell"
            tint: Theme.green
            rotation: banner.displayedKind==="progress" ? banner.spinAngle : 0
        }
    }
    Label {
        id: messageLabel
        x: 56; anchors.verticalCenter: parent.verticalCenter
        width: parent.width-74; maximumLineCount: 2; wrapMode: Text.WordWrap; elide: Text.ElideRight
        font.pixelSize: 18; lineHeight: 1.05
        text: banner.displayedMessage
        color: banner.warning ? (Vehicle.night?"#ffcf85":"#805525") : Theme.ink
    }
}
