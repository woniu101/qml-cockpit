import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs

Column {
    id: panel
    spacing: 12
    function modelResult(result) {
        if (!result) return "等待结果或未配置"
        if (result.error) return result.error
        return [result.model, result.speech_act, result.intent].filter(Boolean).join(" · ")
                + (result.proposal ? "  " + JSON.stringify(result.proposal) : "")
    }
    FileDialog {
        id: replay
        title: "选择测试指令 WAV"
        nameFilters: ["WAV 录音 (*.wav)"]
        onAccepted: AssistantSpeech.openWave(selectedFile)
    }
    Label { text: "开发调试"; font.pixelSize: 32 }
    Label { width: parent.width; wrapMode: Text.WordWrap; text: AssistantNlu.diagnostic; color: Theme.green; font.pixelSize: 18 }
    Label { width: parent.width; elide: Text.ElideRight; textFormat: Text.PlainText; text: "输入  " + (AssistantNlu.review.text || "等待指令"); font.pixelSize: 17 }
    Label { width: parent.width; elide: Text.ElideRight; textFormat: Text.PlainText; text: "现用  " + panel.modelResult(AssistantNlu.review.modelResult); color: Theme.muted; font.pixelSize: 16 }
    Label { width: parent.width; elide: Text.ElideRight; textFormat: Text.PlainText; text: "候选  " + panel.modelResult(AssistantNlu.review.candidateResult); color: Theme.muted; font.pixelSize: 16 }
    Row {
        spacing: 10
        UiButton { text: "回放指令 WAV"; iconName: "folder"; width: 186; enabled: AssistantSpeech.configured && !AssistantSpeech.recognizing && !AssistantNlu.busy; onClicked: replay.open() }
        Controls.CheckBox { text: "记录样本"; checked: AssistantNlu.recordingSamples; onToggled: AssistantNlu.recordingSamples = checked }
    }
    Label { width: parent.width; elide: Text.ElideRight; text: Assistant.message || AssistantSpeech.message; font.pixelSize: 17 }
    Row {
        spacing: 12
        Controls.TextField {
            id: original
            width: 260; height: 46
            placeholderText: "原句（ASR 转写有误时填写）"; color: Theme.ink
            selectByMouse: true
            background: Rectangle { radius: 10; color: Theme.secondary }
        }
        Controls.ComboBox {
            id: act
            width: 145; height: 46
            model: ["REJECT", "CLARIFY", "CANCEL", "QUERY", "EXECUTE"]
            Accessible.name: "正确语义类别"
        }
        Controls.ComboBox {
            id: intent
            width: 205; height: 46
            model: act.currentText === "QUERY" ? ["WINDOW_QUERY", "BATTERY_QUERY"]
                   : act.currentText === "EXECUTE" ? ["WINDOW_SET", "TEMPERATURE_SET", "AC_SET", "FAN_SET",
                                                       "MEDIA_CONTROL", "RADIO_TUNE", "CONTEXT_ADJUSTMENT"]
                   : ["NONE"]
            Accessible.name: "正确意图"
        }
        Controls.TextField {
            id: slots
            width: 260; height: 46; color: Theme.ink; selectByMouse: true
            placeholderText: "正确参数 JSON（如 {\"value\":30}）"
            background: Rectangle { radius: 10; color: Theme.secondary }
        }
        UiButton {
            text: "记录纠正"; width: 128; height: 46
            enabled: AssistantNlu.canCorrect
            onClicked: if (AssistantNlu.correct(original.text, act.currentText, intent.currentText, slots.text, trainingText.text)) {
                original.clear()
                slots.clear()
                trainingText.clear()
            }
        }
    }
    Controls.TextField {
        id: trainingText
        width: Math.min(660, parent.width); height: 46; color: Theme.ink
        placeholderText: "训练标注（选填），例如把[TARGET:右后]窗开到[VALUE:30%]"
        selectByMouse: true
        background: Rectangle { radius: 10; color: Theme.secondary }
    }
}
