import QtQuick
import QtQuick.Layouts

Item {
    id: settings
    required property var shell
    property int section: 0
    Column {
        x: 32; y: 26; width: 218; spacing: 13
        UiButton { width: 116; height: 44; text: "返回"; iconName: "chevron-left"; quiet: true; onClicked: settings.shell.openPopup("settings") }
        Label { text: "座舱助手"; font.pixelSize: 30 }
        Repeater {
            model: [{name:"声音", icon:"volume"}, {name:"形象", icon:"user"}, {name:"唤醒", icon:"mic"}, {name:"开发调试", icon:"settings"}]
            UiButton {
                required property var modelData
                required property int index
                width: 200; height: 53; quiet: true
                text: modelData.name; iconName: modelData.icon
                selected: settings.section === index
                onClicked: settings.section = index
            }
        }
    }
    Rectangle { x: 255; y: 35; width: 1; height: parent.height - 70; color: Theme.line }
    Loader {
        x: 287; y: 44; width: parent.width - x - 36; height: parent.height - 78
        sourceComponent: [voice, appearance, wakeup, debug][settings.section]
    }
    Component { id: voice; AssistantVoicePanel {} }
    Component { id: appearance; AssistantAppearancePanel {} }
    Component { id: wakeup; AssistantWakePanel {} }
    Component { id: debug; AssistantDiagnosticsPanel {} }
}
