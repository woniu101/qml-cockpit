import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    id: panel
    spacing: 16
    FileDialog {
        id: sample
        title: "选择唤醒词测试录音"
        nameFilters: ["WAV 录音 (*.wav)"]
        onAccepted: AssistantWake.replay(selectedFile)
    }
    Connections {
        target: AssistantSpeech
        function onDictated(text, target) { if (target === "wake") keyword.text = text.replace(/[，。！？,.!? ]/g, "") }
    }
    Label { text: "唤醒与对话"; font.pixelSize: 28 }
    AssistantCard {
        Layout.fillWidth: true
        SoftSwitch {
            objectName: "assistantWakeToggle"
            text: "允许唤醒词"
            checked: AssistantWake.enabled
            enabled: AssistantWake.configured || AssistantWake.enabled
            onToggled: AssistantWake.enabled = checked
        }
        Label { Layout.fillWidth: true; text: AssistantWake.message; wrapMode: Text.WordWrap; color: Theme.green; font.pixelSize: 17 }
        Label { text: "唤醒词"; font.pixelSize: 17 }
        RowLayout {
            Layout.fillWidth: true
            AssistantTextField { id: keyword; objectName: "assistantWakeKeyword"; Layout.fillWidth: true; text: AssistantWake.keyword; maximumLength: 8; placeholderText: "4–8 个汉字" }
            UiButton {
                text: AssistantSpeech.recording && AssistantSpeech.dictating ? "结束口述" : "口述"
                iconName: "mic"; textSize: 16; Layout.preferredWidth: 112
                enabled: AssistantSpeech.inputAvailable && !AssistantSpeech.recognizing
                onClicked: AssistantSpeech.recording ? AssistantSpeech.finish() : AssistantSpeech.dictate("wake")
            }
            UiButton { objectName: "assistantSaveWake"; text: "保存"; primary: true; textSize: 17; Layout.preferredWidth: 100; enabled: AssistantWake.configured && !AssistantWake.testing && keyword.text.trim() !== AssistantWake.keyword; onClicked: AssistantWake.saveKeyword(keyword.text) }
            UiButton {
                text: "恢复默认"; textSize: 17; Layout.preferredWidth: 136
                enabled: AssistantWake.configured && !AssistantWake.testing
                onClicked: {
                    keyword.text = AssistantWake.defaultKeyword
                    AssistantWake.saveKeyword(AssistantWake.defaultKeyword)
                }
            }
        }
        Label { Layout.fillWidth: true; text: "唤醒后，等助手显示「正在听」再说指令。"; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 16 }
    }
    AssistantCard {
        Layout.fillWidth: true
        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: "唤醒提示"; font.pixelSize: 17 }
            SoftComboBox {
                objectName: "assistantWakeFeedback"
                Layout.preferredWidth: 180
                Layout.preferredHeight: 44
                model: ["提示音", "我在", "静音"]
                currentIndex: AssistantWake.feedback
                onActivated: index => AssistantWake.feedback = index
            }
        }
        Label {
            Layout.fillWidth: true
            text: "仅在语音唤醒时播放。「我在」使用当前音色，准备期间使用提示音。"
            wrapMode: Text.WordWrap
            color: Theme.muted
            font.pixelSize: 16
        }
    }
    AssistantCard {
        Layout.fillWidth: true
        SoftSwitch { text: "连续对话"; checked: AssistantSpeech.continuous; onToggled: AssistantSpeech.continuous = checked }
        Label { Layout.fillWidth: true; text: "停顿后自动提交，回复结束后收听下一句。说「结束对话」或关闭卡片即可结束收音。"; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 16 }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
        SoftSwitch { text: "用唤醒词打断播报"; enabled: AssistantWake.enabled; checked: AssistantWake.bargeIn; onToggled: AssistantWake.bargeIn = checked }
        Label { Layout.fillWidth: true; text: "命中唤醒词后停止播报，再收听你的下一句。"; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 16 }
    }
    AssistantCard {
        Layout.fillWidth: true
        RowLayout {
            Label { Layout.fillWidth: true; text: AssistantWake.testResult || "用录音测试已保存的唤醒词"; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.pixelSize: 17 }
            UiButton { text: "选择测试 WAV"; textSize: 17; iconName: "folder"; Layout.preferredWidth: 204; enabled: AssistantWake.configured && !AssistantWake.testing; onClicked: sample.open() }
        }
    }
}
