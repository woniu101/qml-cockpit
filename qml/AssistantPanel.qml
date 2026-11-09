import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    required property var shell
    readonly property bool compact: Vehicle.speed > 0 || (Vehicle.externalMode && !Vehicle.externalReady)
    readonly property bool working: AssistantWake.preparing || AssistantSpeech.recording || AssistantSpeech.recognizing || AssistantNlu.busy || Assistant.pending
    readonly property bool speaking: AssistantVoice.playing || AssistantVoice.synthesizing
    readonly property bool candidates: Assistant.choices.length > 0
    readonly property bool followingUp: AssistantNlu.followUpActive
    readonly property string speechError: !working && AssistantSpeech.message !== transcript ? AssistantSpeech.message : ""
    readonly property bool welcome: !working && !speaking && !followingUp && !candidates && !Assistant.message && !speechError
    readonly property bool confirming: AssistantNlu.followUpOptions.indexOf("确认调整") >= 0
                                       || AssistantNlu.followUpOptions.indexOf("确认执行全部") >= 0
    readonly property bool canRecord: AssistantSpeech.configured && AssistantSpeech.inputAvailable
    readonly property string voiceUnavailable: !AssistantSpeech.configured ? "语音输入未配置" : !AssistantSpeech.inputAvailable ? "未检测到麦克风" : ""
    property string transcript: ""
    property bool expandedReply: false
    anchors.fill: parent
    visible: shell.popup === "assistant"
    onVisibleChanged: {
        AssistantSpeech.setSessionOpen(visible)
        if (!visible) {
            AssistantSpeech.cancel()
            if (followingUp) AssistantVoice.stop()
            AssistantNlu.endConversation()
            request.clear()
            transcript = ""
            expandedReply = false
        }
    }
    onCompactChanged: if (compact) request.focus = false
    function record() {
        if (AssistantSpeech.recording) AssistantSpeech.finish()
        else if (!working && canRecord) {
            AssistantVoice.stop()
            if (!followingUp && !candidates) Assistant.clearFeedback()
            transcript = ""
            AssistantSpeech.start()
        }
    }
    Connections {
        target: AssistantSpeech
        function onRecognized(text) { panel.transcript = text }
    }
    FileDialog {
        id: replay
        title: "选择指令录音"
        nameFilters: ["音频 (*.wav *.mp3)"]
        onAccepted: if (panel.visible && !panel.compact) {
            AssistantVoice.stop()
            if (!panel.followingUp && !panel.candidates) Assistant.clearFeedback()
            panel.transcript = ""
            AssistantSpeech.openWave(selectedFile)
        }
    }
    Surface {
        id: card
        objectName: "assistantConversation"
        width: panel.compact ? 500 : 620
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 38
        anchors.bottomMargin: 126
        height: content.implicitHeight + 2 * content.anchors.margins
        radius: panel.compact ? 20 : 26
        border.width: 1
        border.color: Theme.line
        MouseArea { anchors.fill: parent; onWheel: wheel => { wheel.accepted = true } }
        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: panel.compact ? 12 : 18
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                spacing: 13
                AssistantAvatar {
                    Layout.preferredWidth: panel.compact ? 43 : 58
                    Layout.preferredHeight: width
                    active: panel.working || panel.speaking
                    expression: AssistantVoice.playing ? "speaking" : AssistantSpeech.recording ? "listening"
                               : panel.speechError ? "error" : panel.working ? "working" : "idle"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        color: Theme.green
                        text: AssistantSpeech.recording ? "正在听" : AssistantWake.preparing ? "语音唤醒" : AssistantSpeech.recognizing ? "正在识别"
                            : AssistantNlu.batchRunning || Assistant.pending ? "正在执行" : AssistantNlu.busy ? "正在理解"
                            : panel.speechError ? "语音输入" : panel.confirming ? "等待确认"
                            : panel.followingUp ? "等待补全" : panel.candidates ? "等你选择" : AssistantVoice.playing ? "正在播报"
                            : AssistantVoice.synthesizing ? "正在生成语音" : "座舱助手"
                    }
                    Label {
                        id: replyText
                        objectName: "assistantReply"
                        Layout.fillWidth: true
                        font.pixelSize: panel.compact ? 18 : 23
                        font.weight: Font.Normal
                        elide: Text.ElideRight
                        maximumLineCount: panel.compact ? 2 : panel.expandedReply ? 8 : panel.followingUp ? 4 : 2
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                        text: AssistantSpeech.recording ? (AssistantSpeech.automatic ? "停顿后自动提交" : "说完后点一下结束")
                            : AssistantWake.preparing ? "正在准备收音…"
                            : AssistantSpeech.recognizing || (AssistantNlu.busy && !AssistantNlu.batchRunning) ? "请稍候…"
                            : panel.speechError || (panel.candidates && !panel.compact ? "你想播放哪一项？" : Assistant.message)
                              || (panel.compact ? (panel.voiceUnavailable || "点麦克风说话")
                                                : (panel.canRecord ? "说出或输入指令" : "输入指令"))
                    }
                    Label {
                        visible: !panel.compact && panel.welcome && panel.voiceUnavailable !== ""
                        Layout.fillWidth: true
                        color: Theme.muted; font.pixelSize: 14
                        text: panel.voiceUnavailable + (AssistantSpeech.configured ? " · 可输入文字或导入音频" : " · 可输入文字")
                    }
                    Label {
                        visible: panel.compact && !panel.working && (panel.followingUp || panel.candidates)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: Theme.muted; font.pixelSize: 14
                        text: panel.candidates ? "点麦克风，说「选第几个」或「取消」"
                            : AssistantNlu.followUpHint.replace(/^输入/, "说")
                    }
                }
                UiButton {
                    visible: !panel.working && Assistant.message !== "" && !panel.speaking
                    Layout.preferredWidth: 40; Layout.preferredHeight: 44
                    iconName: "volume"; caption: "重播回复"
                    onClicked: AssistantVoice.speak(Assistant.message)
                }
                UiButton {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    visible: panel.compact
                    iconName: AssistantSpeech.recording ? "stop" : "mic"
                    caption: AssistantSpeech.recording ? "结束录音" : "按键说话"
                    enabled: AssistantSpeech.recording || (!panel.working && panel.canRecord)
                    onClicked: panel.record()
                }
                UiButton {
                    objectName: "assistantSettingsShortcut"
                    visible: !panel.compact
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    iconName: "settings"; caption: Vehicle.parked ? "助手设置" : "挂 P 挡后可打开助手设置"
                    enabled: Vehicle.parked && !panel.working
                    onClicked: {
                        AssistantVoice.stop()
                        panel.shell.openAssistantSettings("assistant")
                    }
                }
                UiButton {
                    visible: panel.speaking
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    iconName: "stop"; caption: "停止播报"
                    onClicked: AssistantVoice.stop()
                }
                UiButton {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    iconName: "close"; caption: "关闭助手"
                    onClicked: panel.shell.closeAssistant()
                }
            }
            RowLayout {
                visible: !panel.compact && (replyText.truncated || panel.expandedReply)
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                UiButton { text: panel.expandedReply ? "收起回复" : "展开回复"; textSize: 15; quiet: true; Layout.preferredHeight: 30; onClicked: panel.expandedReply = !panel.expandedReply }
            }
            Label {
                visible: AssistantVoice.failed
                Layout.fillWidth: true
                text: AssistantVoice.message
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                font.pixelSize: 15; color: "#a65c49"
            }
            Rectangle {
                visible: AssistantSpeech.recording
                Layout.fillWidth: true; height: 5; radius: 3; color: Theme.line
                Rectangle { height: parent.height; width: parent.width * AssistantSpeech.level; radius: 3; color: Theme.green }
            }
            RowLayout {
                visible: !panel.compact && panel.welcome
                Layout.fillWidth: true
                Repeater {
                    model: ["查询车辆电量", "主驾空调调到 23 度", "播放下一首"]
                    UiButton {
                        required property string modelData
                        Layout.fillWidth: true; Layout.preferredHeight: 40
                        text: modelData; textSize: 16; selected: true
                        onClicked: { request.text = modelData; request.forceActiveFocus() }
                    }
                }
            }
            Label {
                visible: !panel.compact && panel.transcript !== ""
                Layout.fillWidth: true; elide: Text.ElideRight
                textFormat: Text.PlainText
                text: "识别内容  ·  " + panel.transcript
                color: Theme.muted; font.pixelSize: 15
            }
            ColumnLayout {
                visible: !panel.compact && AssistantNlu.batchResults.length > 0
                Layout.fillWidth: true
                spacing: 4
                Repeater {
                    model: AssistantNlu.batchResults
                    Label {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        font.pixelSize: 15
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                        text: (index + 1) + ". " + modelData.title + " · " + modelData.status
                              + (modelData.detail ? " · " + modelData.detail : "")
                    }
                }
                UiButton {
                    visible: AssistantNlu.batchRunning
                    Layout.alignment: Qt.AlignRight; Layout.preferredWidth: 140; Layout.preferredHeight: 38
                    text: "取消后续"; textSize: 16; quiet: true
                    onClicked: AssistantNlu.cancelRemaining()
                }
            }
            ListView {
                visible: !panel.compact && panel.candidates
                Layout.fillWidth: true; Layout.preferredHeight: Math.min(150, contentHeight)
                model: Assistant.choices; clip: true; spacing: 5
                delegate: UiButton {
                    required property var modelData
                    required property int index
                    width: ListView.view.width; height: 46; textSize: 16
                    selected: true
                    iconName: modelData.kind === "video" ? "video" : "music"
                    text: (index + 1) + "   " + modelData.title + (modelData.artist ? " · " + modelData.artist : "")
                    onClicked: { AssistantVoice.stop(); Assistant.choose(index) }
                }
            }
            Label {
                visible: !panel.compact && panel.candidates
                Layout.fillWidth: true
                text: "点选一项，或说「选第二个」"; color: Theme.muted; font.pixelSize: 15
            }
            GridLayout {
                visible: !panel.compact && !panel.confirming && AssistantNlu.followUpOptions.length > 0
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 6
                Repeater {
                    model: AssistantNlu.followUpOptions
                    UiButton {
                        required property string modelData
                        required property int index
                        property int revision: AssistantNlu.followUpRevision
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        text: modelData
                        textSize: 17
                        selected: true
                        enabled: !panel.working
                        onClicked: {
                            AssistantVoice.stop()
                            AssistantNlu.chooseFollowUp(index, revision)
                        }
                    }
                }
            }
            RowLayout {
                visible: !panel.compact && (panel.followingUp || panel.candidates)
                Layout.fillWidth: true
                spacing: 12
                UiButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    text: "取消"; textSize: 18
                    selected: true
                    enabled: !panel.working
                    onClicked: { AssistantVoice.stop(); AssistantNlu.submit("取消") }
                }
                UiButton {
                    visible: panel.confirming
                    property int revision: AssistantNlu.followUpRevision
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    text: AssistantNlu.followUpOptions[0] || "确认执行"
                    textSize: 18; primary: true; enabled: !panel.working
                    onClicked: { AssistantVoice.stop(); AssistantNlu.chooseFollowUp(0, revision) }
                }
            }
            RowLayout {
                visible: !panel.compact
                Layout.fillWidth: true; spacing: 7
                UiButton {
                    objectName: "assistantRecord"
                    Layout.preferredWidth: 48; Layout.preferredHeight: 45
                    iconName: AssistantSpeech.recording ? "stop" : "mic"
                    caption: AssistantSpeech.recording ? "结束录音" : panel.voiceUnavailable || "按键说话"
                    selected: AssistantSpeech.recording
                    enabled: AssistantSpeech.recording || (!panel.working && panel.canRecord)
                    onClicked: panel.record()
                }
                AssistantTextField {
                    id: request
                    objectName: "assistantText"
                    Layout.fillWidth: true; Layout.preferredHeight: 45
                    placeholderText: panel.followingUp ? AssistantNlu.followUpHint : "输入指令"
                    color: Theme.ink; font.pixelSize: 17
                    enabled: !panel.working
                    selectByMouse: true
                    onAccepted: send.clicked()
                }
                UiButton {
                    id: send
                    objectName: "assistantSend"
                    Layout.preferredWidth: 70; Layout.preferredHeight: 45
                    text: "发送"; textSize: 16; primary: true
                    enabled: request.enabled && request.text.trim().length > 0
                    onClicked: {
                        if (!enabled) return
                        AssistantVoice.stop()
                        AssistantSpeech.cancel()
                        panel.transcript = ""
                        AssistantNlu.submit(request.text)
                        request.clear()
                    }
                }
                UiButton {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 45
                    iconName: "folder"; caption: "导入指令音频"
                    enabled: !panel.working && AssistantSpeech.configured
                    onClicked: replay.open()
                }
            }
        }
    }
}
