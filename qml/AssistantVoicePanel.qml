import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    FileDialog {
        id: referenceDialog
        title: "选择助手音色参考 WAV"
        nameFilters: ["WAV 音频 (*.wav)"]
        onAccepted: {
            AssistantVoice.importReference(selectedFile)
            transcript.text = AssistantVoice.transcript
            editReference = true
        }
    }
    property bool editReference: false
    readonly property var rates: [{value:85, label:"稍慢 0.85×"}, {value:100, label:"标准 1.00×"}, {value:115, label:"稍快 1.15×"}]
    Component.onDestruction: if (AssistantVoice && AssistantVoice.previewActive) AssistantVoice.stop()
    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width - 350
        spacing: 14
        Label { text: "声音设置"; font.pixelSize: 32 }
        Label { text: "选择助手的音色、语速和回复方式。"; color: Theme.muted; font.pixelSize: 17 }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: preferences.implicitHeight + 30
            color: Theme.secondary; radius: 21
            ColumnLayout {
                id: preferences
                x: 18; y: 15; width: parent.width - 36
                spacing: 7
                RowLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: "当前音色   " + (AssistantVoice.activeName || "未配置"); elide: Text.ElideRight; font.pixelSize: 19 }
                    UiButton { text: "导入参考 WAV"; iconName: "folder"; textSize: 16; Layout.preferredWidth: 175; enabled: AssistantVoice.configured && !AssistantVoice.busy; onClicked: referenceDialog.open() }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
                RowLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: "语速"; font.pixelSize: 19 }
                    Repeater {
                        model: panel.rates
                        UiButton {
                            required property var modelData
                            Layout.preferredWidth: 113; Layout.preferredHeight: 43; textSize: 15
                            text: modelData.label; selected: AssistantVoice.speed === modelData.value
                            enabled: AssistantVoice.rateAvailable
                            onClicked: AssistantVoice.speed = modelData.value
                        }
                    }
                }
                Label { visible: !AssistantVoice.rateAvailable; text: "当前音频后端使用标准语速"; color: Theme.muted; font.pixelSize: 15 }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
                RowLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: "播报音量"; font.pixelSize: 19 }
                    SoftSlider { Layout.preferredWidth: 238; from: 0; to: 100; stepSize: 1; value: AssistantVoice.volume; onMoved: AssistantVoice.volume = value }
                    Label { Layout.preferredWidth: 52; text: AssistantVoice.volume + "%"; font.pixelSize: 16 }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
                RowLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: "回复方式"; font.pixelSize: 19 }
                    UiButton { text: "语音＋文字"; textSize: 16; selected: AssistantVoice.repliesEnabled; onClicked: AssistantVoice.repliesEnabled = true }
                    UiButton { text: "仅文字"; textSize: 16; selected: !AssistantVoice.repliesEnabled; onClicked: AssistantVoice.repliesEnabled = false }
                }
            }
        }
        RowLayout {
            UiButton { text: "恢复默认播放设置"; iconName: "refresh"; textSize: 16; Layout.preferredWidth: 212; onClicked: AssistantVoice.resetPlaybackPreferences() }
            UiButton { text: panel.editReference ? "收起参考原文" : "查看参考原文"; textSize: 16; Layout.preferredWidth: 152; onClicked: panel.editReference = !panel.editReference }
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: panel.editReference
            Label { text: "参考音频原文  ·  " + (AssistantVoice.referenceName || "未选择"); font.pixelSize: 17 }
            Controls.TextArea {
                id: transcript
                text: AssistantVoice.transcript
                Layout.fillWidth: true; Layout.preferredHeight: 70
                color: Theme.ink; font.pixelSize: 16; wrapMode: TextEdit.Wrap; selectByMouse: true
                placeholderText: "填入 1–15 秒录音中实际说的话"
                enabled: !AssistantVoice.busy
                background: Rectangle { color: Theme.secondary; radius: 12 }
            }
            UiButton { text: "保存并使用"; Layout.preferredWidth: 152; height: 45; primary: true; enabled: AssistantVoice.verified && !AssistantVoice.busy; onClicked: AssistantVoice.save(transcript.text) }
        }
    }
    Surface {
        anchors.right: parent.right; anchors.top: parent.top
        width: 315; height: 520; radius: 22; color: Theme.secondary
        ColumnLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: 17; spacing: 13
            Label { text: "试听与形象"; color: Theme.green; font.pixelSize: 16 }
            Image { Layout.fillWidth: true; Layout.preferredHeight: 290; source: AssistantAppearance.avatar; fillMode: Image.PreserveAspectFit; visible: AssistantAppearance.shown }
            AssistantAvatar { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 108; Layout.preferredHeight: 108; visible: !AssistantAppearance.shown }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: "「右后车窗已调到百分之三十。」"; color: Theme.muted; font.pixelSize: 16 }
            UiButton {
                Layout.fillWidth: true; primary: true; textSize: 17
                text: AssistantVoice.busy ? "正在合成…" : AssistantVoice.playing ? "停止试听" : "试听当前设置"
                enabled: AssistantVoice.configured && !AssistantVoice.busy && transcript.text.trim().length > 0
                onClicked: AssistantVoice.playing ? AssistantVoice.stop() : AssistantVoice.preview(transcript.text)
            }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: AssistantVoice.message; color: Theme.muted; font.pixelSize: 15 }
        }
    }
}
