import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    implicitHeight: Math.max(main.implicitHeight, audition.implicitHeight)
    property bool creating: AssistantVoice.dirty
    property bool showReference: false
    property int step: AssistantVoice.verified ? 2 : AssistantVoice.transcript ? 1 : 0
    readonly property bool occupied: AssistantVoice.busy || AssistantVoice.referenceClip.busy || AssistantVoice.countdown > 0
    readonly property bool transcribing: AssistantSpeech.dictating && (AssistantSpeech.recording || AssistantSpeech.recognizing)
    readonly property var clip: AssistantVoice.referenceClip
    FileDialog {
        id: audioFile
        title: "选择音色参考音频"
        nameFilters: ["音频 (*.wav *.mp3)"]
        onAccepted: { panel.creating = true; panel.step = 0; AssistantVoice.importReference(selectedFile) }
    }
    Connections {
        target: AssistantVoice.referenceClip
        function onLoaded() { panel.creating = true; panel.step = 1 }
    }
    ColumnLayout {
        id: main
        width: parent.width - 352
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            Label { text: "声音"; font.pixelSize: 28; Layout.fillWidth: true }
            UiButton {
                visible: !panel.creating
                text: "创建音色"; iconName: "plus"; primary: true; textSize: 17
                Layout.preferredWidth: 148; Layout.preferredHeight: 46
                onClicked: { panel.creating = true; panel.step = 0 }
            }
        }
        AssistantCard {
            Layout.fillWidth: true
            visible: !panel.creating
            Label { text: "音频设备"; font.pixelSize: 20 }
            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 18; rowSpacing: 12
                Label { text: "收音"; font.pixelSize: 17 }
                Label { text: "播报"; font.pixelSize: 17 }
                SoftComboBox {
                    objectName: "assistantInputRoute"
                    Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.preferredHeight: 46
                    model: AssistantSpeech.route.names; currentIndex: AssistantSpeech.route.selectedIndex
                    Accessible.name: "助手收音设备"
                    onActivated: index => AssistantSpeech.route.select(index)
                }
                SoftComboBox {
                    objectName: "assistantOutputRoute"
                    Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.preferredHeight: 46
                    model: AssistantVoice.route.names; currentIndex: AssistantVoice.route.selectedIndex
                    Accessible.name: "助手播报设备"
                    onActivated: index => AssistantVoice.route.select(index)
                }
            }
        }
        AssistantCard {
            Layout.fillWidth: true
            visible: !panel.creating
            RowLayout {
                Layout.fillWidth: true
                Label { Layout.fillWidth: true; text: "当前音色  ·  " + (AssistantVoice.activeName || "未配置"); elide: Text.ElideRight; font.pixelSize: 19; textFormat: Text.PlainText }
                UiButton { text: panel.showReference ? "收起" : "原文"; textSize: 16; quiet: true; Layout.preferredWidth: 76; Layout.preferredHeight: 40; onClicked: panel.showReference = !panel.showReference }
                UiButton { text: "原声"; textSize: 16; quiet: true; Layout.preferredWidth: 76; Layout.preferredHeight: 40; enabled: !panel.occupied; onClicked: AssistantVoice.playReference(true) }
            }
            Label { visible: panel.showReference; Layout.fillWidth: true; text: AssistantVoice.activeTranscript; font.pixelSize: 16; wrapMode: Text.WordWrap; color: Theme.muted; textFormat: Text.PlainText }
            RowLayout {
                Label { text: "语速"; font.pixelSize: 17; Layout.fillWidth: true }
                Repeater {
                    model: [{v:85,t:"稍慢"}, {v:100,t:"标准"}, {v:115,t:"稍快"}]
                    UiButton {
                        required property var modelData
                        text: modelData.t; textSize: 16
                        Layout.preferredWidth: 95; Layout.preferredHeight: 42
                        selected: AssistantVoice.speed === modelData.v; enabled: AssistantVoice.rateAvailable
                        onClicked: AssistantVoice.speed = modelData.v
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "播报音量"; font.pixelSize: 17; Layout.fillWidth: true }
                SoftSlider { Layout.preferredWidth: 245; from: 0; to: 100; value: AssistantVoice.volume; onMoved: AssistantVoice.volume = Math.round(value) }
                Label { text: AssistantVoice.volume + "%"; font.pixelSize: 16; Layout.preferredWidth: 48 }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 18
                Label { text: "回复方式"; font.pixelSize: 17 }
                SoftComboBox {
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    model: ["语音＋文字", "仅语音输入时播报", "仅文字"]
                    currentIndex: ["all","voice","text"].indexOf(AssistantVoice.replyMode)
                    onActivated: index => AssistantVoice.replyMode = ["all","voice","text"][index]
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
            UiButton {
                objectName: "assistantResetPlayback"
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 216; Layout.preferredHeight: 44
                text: "恢复默认播报设置"; iconName: "refresh"; textSize: 16
                caption: "恢复语速、播报音量和回复方式的默认值"
                enabled: !panel.occupied && !panel.transcribing
                onClicked: AssistantVoice.resetPlaybackPreferences()
            }
        }
        AssistantCard {
            visible: panel.creating
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                Label { text: "创建音色"; font.pixelSize: 23; Layout.fillWidth: true }
                Label { text: (panel.step + 1) + " / 3"; font.pixelSize: 16; color: Theme.green }
            }
            RowLayout {
                Layout.fillWidth: true
                Repeater {
                    model: ["获取声音", "核对原文", "试听并启用"]
                    Label {
                        required property string modelData; required property int index
                        Layout.fillWidth: true; font.pixelSize: 16
                        text: (index + 1) + "  " + modelData
                        color: panel.step === index ? Theme.green : Theme.muted
                    }
                }
            }
            ColumnLayout {
                visible: panel.step === 0; Layout.fillWidth: true; spacing: 13
                Label { Layout.fillWidth: true; text: "导入一段单人清晰语音，或朗读下面的提示。"; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 17 }
                RowLayout {
                    UiButton { text: "导入 WAV / MP3"; iconName: "folder"; textSize: 17; Layout.preferredWidth: 206; enabled: !panel.occupied && !panel.transcribing; onClicked: audioFile.open() }
                    UiButton {
                        text: AssistantVoice.countdown > 0 ? "准备 " + AssistantVoice.countdown : panel.clip.recording ? "结束录制" : "朗读录制"
                        iconName: panel.clip.recording ? "stop" : "mic"; primary: true; textSize: 17
                        Layout.preferredWidth: 165
                        enabled: panel.clip.recording || (!panel.occupied && !panel.transcribing && AssistantSpeech.inputAvailable)
                        onClicked: panel.clip.recording ? panel.clip.finishRecording() : AssistantVoice.recordReference()
                    }
                    UiButton { visible: AssistantVoice.countdown > 0; text: "取消"; quiet: true; textSize: 17; onClicked: AssistantVoice.cancelRecording() }
                }
                Label { Layout.fillWidth: true; text: AssistantVoice.readingPrompt; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.pixelSize: 23 }
                Controls.ProgressBar {
                    id: recordMeter
                    visible: panel.clip.recording
                    Layout.fillWidth: true
                    value: panel.clip.level
                    background: Rectangle { implicitHeight: 6; color: Theme.line; radius: 3 }
                    contentItem: Item { Rectangle { width: recordMeter.visualPosition * parent.width; height: 6; radius: 3; color: Theme.green } }
                }
                Label { Layout.fillWidth: true; text: panel.clip.message || "录制最长 15 秒。请保持自然语速，避免背景音乐。"; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 16 }
            }
            ColumnLayout {
                visible: panel.step === 1; Layout.fillWidth: true; spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: AssistantVoice.referenceName; textFormat: Text.PlainText; elide: Text.ElideRight; font.pixelSize: 18 }
                    UiButton { text: "听原声"; iconName: "volume"; textSize: 16; enabled: !panel.occupied; onClicked: AssistantVoice.playReference() }
                }
                Label { visible: panel.transcribing; text: "正在识别参考原文…"; font.pixelSize: 16; color: Theme.green }
                Label { visible: AssistantSpeech.dictating && AssistantSpeech.failed; Layout.fillWidth: true; text: AssistantSpeech.message; wrapMode: Text.WordWrap; font.pixelSize: 16; color: "#a65c49" }
                Label { text: "确认下面是录音中实际说的话"; font.pixelSize: 17 }
                AssistantTextArea {
                    id: referenceText
                    Layout.fillWidth: true; Layout.preferredHeight: 100
                    text: AssistantVoice.transcript
                    enabled: !panel.occupied && !panel.transcribing
                    placeholderText: "可以输入原文，或点击口述"
                    onTextChanged: if (editing) AssistantVoice.setTranscript(text)
                }
                RowLayout {
                    UiButton { text: AssistantSpeech.recording && panel.transcribing ? "结束口述" : "口述原文"; iconName: "mic"; textSize: 16; enabled: AssistantSpeech.inputAvailable && !panel.occupied && !AssistantSpeech.recognizing; onClicked: AssistantSpeech.recording ? AssistantSpeech.finish() : AssistantSpeech.dictate("reference") }
                    UiButton { text: "重新转写"; textSize: 16; enabled: panel.clip.ready && !panel.occupied && !panel.transcribing; onClicked: AssistantSpeech.transcribe(panel.clip.wave, "reference") }
                }
                ColumnLayout {
                    visible: panel.clip.ready && panel.clip.duration > 15
                    Layout.fillWidth: true
                    Label { text: "选取参考片段"; font.pixelSize: 17 }
                    RowLayout {
                        Label { text: "起点 " + start.value.toFixed(1) + " 秒"; font.pixelSize: 15; Layout.preferredWidth: 120 }
                        SoftSlider { id: start; Layout.fillWidth: true; from: 0; to: Math.max(0, panel.clip.duration - length.value); value: panel.clip.start; stepSize: 0.1 }
                    }
                    RowLayout {
                        Label { text: "时长 " + length.value.toFixed(1) + " 秒"; font.pixelSize: 15; Layout.preferredWidth: 120 }
                        SoftSlider { id: length; Layout.fillWidth: true; from: 1; to: 15; value: panel.clip.length; stepSize: 0.1 }
                    }
                    UiButton { text: "更新片段并转写"; textSize: 16; enabled: !panel.transcribing && !panel.occupied; onClicked: AssistantVoice.trimReference(start.value, length.value) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    UiButton { text: "重新获取"; textSize: 16; quiet: true; enabled: !panel.occupied && !panel.transcribing; onClicked: panel.step = 0 }
                    Item { Layout.fillWidth: true }
                    UiButton { text: "原文正确，去试听"; primary: true; textSize: 17; Layout.preferredWidth: 198; enabled: AssistantVoice.transcript.trim().length > 0 && !panel.transcribing && !panel.occupied; onClicked: panel.step = 2 }
                }
            }
            ColumnLayout {
                visible: panel.step === 2; Layout.fillWidth: true; spacing: 12
                Label { Layout.fillWidth: true; text: "在右侧选择试听句，听过效果后保存启用。"; wrapMode: Text.WordWrap; font.pixelSize: 18 }
                Label { Layout.fillWidth: true; text: "参考原文  ·  " + AssistantVoice.transcript; wrapMode: Text.WordWrap; font.pixelSize: 16; color: Theme.muted; textFormat: Text.PlainText }
                RowLayout {
                    UiButton { text: "返回原文"; quiet: true; textSize: 17; enabled: !panel.occupied; onClicked: panel.step = 1 }
                    Item { Layout.fillWidth: true }
                    UiButton { text: "保存并启用"; primary: true; textSize: 17; Layout.preferredWidth: 174; enabled: AssistantVoice.verified && !panel.occupied; onClicked: if (AssistantVoice.save(AssistantVoice.transcript)) panel.creating = false }
                }
            }
            Label { Layout.fillWidth: true; text: AssistantVoice.message; visible: text.length > 0; wrapMode: Text.WordWrap; textFormat: Text.PlainText; font.pixelSize: 16; color: AssistantVoice.failed ? "#a65c49" : Theme.muted }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: panel.creating || AssistantVoice.dirty
            UiButton {
                visible: panel.creating
                text: "保留草稿，返回设置"
                textSize: 16; quiet: true; Layout.preferredWidth: 212
                enabled: !panel.occupied && !panel.transcribing
                onClicked: panel.creating = false
            }
            UiButton {
                visible: AssistantVoice.dirty
                text: panel.creating ? "放弃草稿" : "继续编辑音色"; textSize: 16
                enabled: !panel.occupied && !panel.transcribing
                onClicked: {
                    if (panel.creating) { AssistantSpeech.cancel(); AssistantVoice.discardDraft(); panel.creating = false }
                    else { panel.creating = true; panel.step = 1 }
                }
            }
        }
    }
    AssistantCard {
        id: audition
        x: parent.width - width; width: 332
        AssistantAvatar { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 94; Layout.preferredHeight: 94; expression: AssistantVoice.playing ? "speaking" : "idle"; active: AssistantVoice.playing }
        Label { text: AssistantVoice.dirty ? "草稿试听" : "当前音色试听"; font.pixelSize: 23 }
        Label { Layout.fillWidth: true; text: "选择一句示例，或输入想试听的内容。"; wrapMode: Text.WordWrap; font.pixelSize: 16; color: Theme.muted }
        SoftComboBox {
            Layout.fillWidth: true; Layout.preferredHeight: 46
            model: ["车窗操作", "出发问候", "播放音乐"]
            onActivated: index => AssistantVoice.previewText = ["右后车窗已调到百分之三十。", "早上好，祝你今天一路顺风。", "正在播放你喜欢的音乐。"][index]
        }
        AssistantTextArea {
            Layout.fillWidth: true; Layout.preferredHeight: 112
            text: AssistantVoice.previewText
            enabled: !panel.occupied && !panel.transcribing
            onTextChanged: if (editing) AssistantVoice.previewText = text
        }
        UiButton {
            text: AssistantSpeech.recording && AssistantSpeech.dictating ? "结束口述" : "口述试听句"
            iconName: "mic"; textSize: 16; Layout.fillWidth: true
            enabled: AssistantSpeech.inputAvailable && !panel.occupied && !AssistantSpeech.recognizing
            onClicked: AssistantSpeech.recording ? AssistantSpeech.finish() : AssistantSpeech.dictate("preview")
        }
        UiButton {
            objectName: "assistantPreview"
            Layout.fillWidth: true; primary: true; textSize: 18
            text: AssistantVoice.playing ? "停止试听" : AssistantVoice.busy ? "取消生成" : "试听音色"
            enabled: !panel.clip.busy && !panel.transcribing && AssistantVoice.countdown === 0 && AssistantVoice.transcript.trim().length > 0 && AssistantVoice.previewText.trim().length > 0
            onClicked: AssistantVoice.playing || AssistantVoice.busy ? AssistantVoice.stop() : AssistantVoice.preview()
        }
        Label { Layout.fillWidth: true; text: AssistantVoice.message; visible: text.length > 0; wrapMode: Text.WordWrap; font.pixelSize: 15; color: AssistantVoice.failed ? "#a65c49" : Theme.muted; textFormat: Text.PlainText }
    }
}
