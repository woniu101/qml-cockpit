import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    id: panel
    spacing: 16
    property bool showJson: false
    readonly property var acts: ["REJECT","CLARIFY","CANCEL","QUERY","EXECUTE"]
    readonly property var intentKeys: act.currentIndex === 3 ? ["WINDOW_QUERY","BATTERY_QUERY"]
        : act.currentIndex === 4 ? ["WINDOW_SET","TEMPERATURE_SET","AC_SET","FAN_SET","MEDIA_CONTROL","RADIO_TUNE","CONTEXT_ADJUSTMENT"] : ["NONE"]
    readonly property var intentNames: act.currentIndex === 3 ? ["车窗查询","电量查询"]
        : act.currentIndex === 4 ? ["车窗开度","空调温度","空调开关","风量","媒体控制","电台点播","连续调整"] : ["无动作"]
    property bool validParameters: true
    function modelSummary(result) {
        if (!result) return AssistantNlu.busy ? "正在解析…" : "本次未调用该模型"
        if (result.error) return result.error
        return [result.model, result.speech_act, result.intent,
                result.elapsed_ms !== undefined ? result.elapsed_ms + " ms" : ""].filter(Boolean).join(" · ")
    }
    FileDialog {
        id: replay
        title: "选择测试音频"
        nameFilters: ["音频 (*.wav *.mp3)"]
        onAccepted: AssistantSpeech.openWave(selectedFile)
    }
    Label { text: "开发调试"; font.pixelSize: 28 }
    Label { text: "回放指令、查看解析与执行结果，记录可复现的纠错样本。"; font.pixelSize: 16; color: Theme.muted }
    AssistantCard {
        Layout.fillWidth: true
        RowLayout {
            Layout.fillWidth: true
            AssistantTextField {
                id: testInput
                Layout.fillWidth: true
                placeholderText: "输入测试指令"
                enabled: !AssistantNlu.busy && !Assistant.pending
                onAccepted: run.clicked()
            }
            UiButton { id: run; text: "测试"; primary: true; Layout.preferredWidth: 95; textSize: 17; enabled: testInput.enabled && testInput.text.trim() !== ""; onClicked: AssistantNlu.submit(testInput.text) }
            UiButton { text: "回放音频"; iconName: "folder"; textSize: 17; Layout.preferredWidth: 168; enabled: AssistantSpeech.configured && !AssistantSpeech.recognizing && !AssistantSpeech.recording && !AssistantNlu.busy; onClicked: replay.open() }
            SoftSwitch { text: "记录样本"; checked: AssistantNlu.recordingSamples; onToggled: AssistantNlu.recordingSamples = checked }
        }
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 16; Layout.alignment: Qt.AlignTop
        AssistantCard {
            Layout.fillWidth: true; Layout.preferredWidth: 580; Layout.alignment: Qt.AlignTop
            Label { text: "本次结果"; font.pixelSize: 22 }
            Label { Layout.fillWidth: true; text: AssistantNlu.review.input === "asr" ? "语音转写" : "文字输入"; color: Theme.muted; font.pixelSize: 15 }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.PlainText; text: AssistantNlu.review.text || "等待指令"; font.pixelSize: 19 }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
            Label { text: "现用模型"; font.pixelSize: 15; color: Theme.muted }
            Label { Layout.fillWidth: true; text: panel.modelSummary(AssistantNlu.review.modelResult); wrapMode: Text.WordWrap; textFormat: Text.PlainText; font.pixelSize: 16 }
            Label { text: "对照模型"; font.pixelSize: 15; color: Theme.muted }
            Label { Layout.fillWidth: true; text: panel.modelSummary(AssistantNlu.review.candidateResult); wrapMode: Text.WordWrap; textFormat: Text.PlainText; font.pixelSize: 16 }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
            Label { text: "准入与执行"; font.pixelSize: 15; color: Theme.muted }
            Label { Layout.fillWidth: true; text: AssistantNlu.diagnostic; wrapMode: Text.WordWrap; textFormat: Text.PlainText; font.pixelSize: 16; color: Theme.green }
            Label { Layout.fillWidth: true; text: Assistant.message || AssistantSpeech.message || "等待结果"; wrapMode: Text.WordWrap; textFormat: Text.PlainText; font.pixelSize: 18 }
            RowLayout {
                UiButton { text: panel.showJson ? "收起原始数据" : "展开原始数据"; textSize: 16; quiet: true; onClicked: panel.showJson = !panel.showJson }
                UiButton { visible: panel.showJson; text: "复制"; textSize: 16; quiet: true; onClicked: { raw.selectAll(); raw.copy(); raw.deselect() } }
            }
            AssistantTextArea {
                id: raw
                visible: panel.showJson
                Layout.fillWidth: true; Layout.preferredHeight: 185
                readOnly: true; font.pixelSize: 14
                text: JSON.stringify(AssistantNlu.review, null, 2)
            }
        }
        AssistantCard {
            Layout.fillWidth: true; Layout.preferredWidth: 480; Layout.alignment: Qt.AlignTop
            Label { text: "人工纠错"; font.pixelSize: 22 }
            Label { text: "原句"; font.pixelSize: 15; color: Theme.muted }
            AssistantTextField { id: original; Layout.fillWidth: true; placeholderText: "转写有误时，填写实际说的话" }
            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 6
                Label { text: "正确类别"; font.pixelSize: 15; color: Theme.muted }
                Label { text: "正确意图"; font.pixelSize: 15; color: Theme.muted }
                SoftComboBox { id: act; Layout.fillWidth: true; Layout.preferredHeight: 46; model: ["拒绝／非指令","需要追问","取消","查询","执行"]; onActivated: intent.currentIndex = 0 }
                SoftComboBox { id: intent; Layout.fillWidth: true; Layout.preferredHeight: 46; model: panel.intentNames }
            }
            Label { text: "正确参数"; font.pixelSize: 15; color: Theme.muted }
            AssistantTextArea {
                id: parameters
                Layout.fillWidth: true; Layout.preferredHeight: 82
                placeholderText: 'JSON 对象，例如 {"value":23,"zone":"driver","unit":"celsius"}'
                onTextChanged: {
                    try {
                        const value = JSON.parse(text.trim() || "{}")
                        panel.validParameters = value !== null && !Array.isArray(value) && typeof value === "object"
                    } catch (error) { panel.validParameters = false }
                }
            }
            Label { visible: !panel.validParameters; text: "请输入有效的 JSON 对象"; color: "#a65c49"; font.pixelSize: 15 }
            Label { text: "训练标注（可选）"; font.pixelSize: 15; color: Theme.muted }
            AssistantTextArea { id: annotation; Layout.fillWidth: true; Layout.preferredHeight: 68; placeholderText: "例如 把[TARGET:右后]窗开到[VALUE:30%]"; font.pixelSize: 15 }
            UiButton {
                Layout.fillWidth: true; primary: true; text: "保存纠错样本"; textSize: 17
                enabled: AssistantNlu.canCorrect && panel.validParameters
                onClicked: if (AssistantNlu.correct(original.text, panel.acts[act.currentIndex], panel.intentKeys[intent.currentIndex], parameters.text, annotation.text)) {
                    original.clear(); parameters.clear(); annotation.clear()
                }
            }
        }
    }
}
