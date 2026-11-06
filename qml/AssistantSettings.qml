import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: settings
    required property var shell
    property int section: 0
    UiButton {
        objectName: "assistantSettingsBack"
        x: 22; y: 20; width: 48; height: 48
        iconName: "chevron-left"; quiet: true
        caption: settings.shell.assistantSettingsReturn === "assistant" ? "返回会话" : "返回偏好"
        onClicked: settings.shell.returnFromAssistantSettings()
    }
    Label { x: 82; y: 28; text: "座舱助手"; font.pixelSize: 28 }
    Label { x: 249; y: 36; text: "声音、形象与对话偏好"; font.pixelSize: 16; color: Theme.muted }
    Rectangle { x: 24; y: 84; width: parent.width - 48; height: 1; color: Theme.line }
    Column {
        x: 24; y: 110; width: 190; spacing: 12
        Repeater {
            model: [{name:"声音", icon:"volume"}, {name:"形象", icon:"user"},
                    {name:"唤醒与对话", icon:"mic"}, {name:"开发调试", icon:"settings"}]
            UiButton {
                required property var modelData
                required property int index
                objectName: "assistantSection" + index
                width: 190; height: 54; quiet: true
                text: modelData.name; iconName: modelData.icon; textSize: 19
                selected: settings.section === index
                onClicked: {
                    if (settings.section === index) return
                    AssistantVoice.cancelRecording()
                    AssistantSpeech.cancel()
                    AssistantVoice.stop()
                    settings.section = index
                    sectionScroll.contentY = 0
                }
            }
        }
    }
    Rectangle { x: 232; y: 110; width: 1; height: parent.height - 138; color: Theme.line }
    Flickable {
        id: sectionScroll
        objectName: "assistantSettingsScroll"
        x: 259; y: 109; width: parent.width - x - 30; height: parent.height - y - 24
        clip: true; contentHeight: sectionContent.height
        boundsBehavior: Flickable.StopAtBounds
        Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }
        Loader {
            id: sectionContent
            width: sectionScroll.width - 15
            height: item ? item.implicitHeight : 0
            sourceComponent: [voice, appearance, wakeup, debug][settings.section]
        }
    }
    Component { id: voice; AssistantVoicePanel {} }
    Component { id: appearance; AssistantAppearancePanel {} }
    Component { id: wakeup; AssistantWakePanel {} }
    Component { id: debug; AssistantDiagnosticsPanel {} }
    Rectangle {
        anchors.fill: parent; radius: 28
        visible: !!settings.shell.assistantDraftPrompt
        color: Theme.surface; z: 20
        MouseArea { anchors.fill: parent; onWheel: wheel => { wheel.accepted = true } }
        Column {
            anchors.centerIn: parent; width: 540; spacing: 20
            Label { text: "还有未保存的修改"; font.pixelSize: 28 }
            Label { width: parent.width; text: "可以保留草稿稍后继续，也可以放弃本次修改。"; font.pixelSize: 18; color: Theme.muted; wrapMode: Text.WordWrap }
            Row {
                spacing: 12
                UiButton { width: 170; text: "继续编辑"; textSize: 18; onClicked: settings.shell.assistantDraftPrompt = false }
                UiButton { width: 170; text: "放弃修改"; textSize: 18; onClicked: settings.shell.finishAssistantExit(false) }
                UiButton { width: 170; text: "保留草稿"; textSize: 18; primary: true; onClicked: settings.shell.finishAssistantExit(true) }
            }
        }
    }
}
