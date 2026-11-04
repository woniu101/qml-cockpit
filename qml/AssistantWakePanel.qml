import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs

Column {
    spacing: 16
    FileDialog {
        id: sample
        title: "选择唤醒词试测 WAV"
        nameFilters: ["WAV 录音 (*.wav)"]
        onAccepted: AssistantWake.replay(selectedFile)
    }

    Label { text: "唤醒方式"; font.pixelSize: 32 }
    Controls.Switch {
        objectName: "assistantWakeToggle"
        text: "允许唤醒词"
        font.pixelSize: 17; palette.highlight: Theme.green
        checked: AssistantWake.enabled
        enabled: AssistantWake.configured || AssistantWake.enabled
        onToggled: AssistantWake.enabled = checked
    }
    Label {
        width: parent.width; wrapMode: Text.WordWrap
        text: AssistantWake.configured ? AssistantWake.message : "离线唤醒模型未配置，仍可用按键说话"
        font.pixelSize: 19; color: Theme.green
    }
    Label {
        width: parent.width; wrapMode: Text.WordWrap
        text: "使用 4–8 个汉字作为唤醒词。启用后，等助手显示「正在听」再说指令。"
        font.pixelSize: 17; color: Theme.muted
    }
    Row {
        spacing: 12
        Controls.TextField {
            id: keyword
            objectName: "assistantWakeKeyword"
            width: 285; height: 46
            color: Theme.ink; font.pixelSize: 18; placeholderText: "输入唤醒词"
            text: AssistantWake.keyword; maximumLength: 8; selectByMouse: true
            background: Rectangle { radius: 12; color: Theme.secondary }
        }
        UiButton {
            objectName: "assistantSaveWake"
            width: 130; height: 46; text: "保存唤醒词"; textSize: 17; selected: true
            enabled: AssistantWake.configured && !AssistantWake.testing && keyword.text.trim() !== AssistantWake.keyword
            onClicked: AssistantWake.saveKeyword(keyword.text)
        }
        UiButton {
            width: 128; height: 46; text: "恢复默认"; textSize: 17
            enabled: AssistantWake.configured && !AssistantWake.testing
            onClicked: {
                keyword.text = "你好小林"
                if (AssistantWake.keyword !== keyword.text) AssistantWake.saveKeyword(keyword.text)
            }
        }
    }
    UiButton {
        width: 170; height: 46; text: "用 WAV 试测"; textSize: 17; iconName: "folder"
        enabled: AssistantWake.configured && !AssistantWake.testing
        onClicked: sample.open()
    }
    Label {
        width: parent.width; wrapMode: Text.WordWrap; textFormat: Text.PlainText
        text: AssistantWake.testResult || "试测已保存的唤醒词，可在关闭麦克风监听时使用。"
        font.pixelSize: 17; color: Theme.muted
    }
}
