import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    required property var shell
    readonly property bool compact: !Vehicle.parked || shell.page === AppRoutes.Driving
    readonly property bool working: AssistantSpeech.recording || AssistantSpeech.recognizing || AssistantNlu.busy || Assistant.pending
    readonly property bool speaking: AssistantVoice.playing || AssistantVoice.synthesizing
    readonly property bool candidates: Assistant.choices.length > 0
    readonly property bool followingUp: AssistantNlu.followUpOptions.length > 0
    property string transcript: ""
    anchors.fill: parent
    visible: shell.popup === "assistant"
    onVisibleChanged: {
        if (!visible) {
            AssistantSpeech.cancel()
            if (followingUp) AssistantVoice.stop()
            AssistantNlu.endConversation()
        }
        else transcript = ""
    }
    function record() {
        if (AssistantSpeech.recording) AssistantSpeech.finish()
        else if (!working && AssistantSpeech.configured) AssistantSpeech.start()
    }
    Connections {
        target: AssistantSpeech
        function onRecognized(text) { panel.transcript = text }
    }
    FileDialog {
        id: replay
        title: "选择指令录音"
        nameFilters: ["WAV 录音 (*.wav)"]
        onAccepted: if (panel.visible && !panel.compact) AssistantSpeech.openWave(selectedFile)
    }
    Surface {
        id: card
        width: panel.compact ? 455 : 580
        x: panel.compact ? panel.width - width - 49 : (panel.width - width) / 2
        y: panel.compact ? 83 : panel.height - 130 - height
        height: panel.compact ? 75 : content.implicitHeight + 36
        radius: panel.compact ? 20 : 26
        border.width: 1
        border.color: Theme.line
        MouseArea { anchors.fill: parent }
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
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        color: Theme.green
                        text: AssistantSpeech.recording ? "正在听" : AssistantSpeech.recognizing ? "正在识别"
                            : AssistantNlu.busy ? "正在理解" : Assistant.pending ? "正在执行"
                            : panel.followingUp ? "等待补全" : panel.candidates ? "等你选择" : AssistantVoice.playing ? "正在播报"
                            : AssistantVoice.synthesizing ? "正在生成语音" : "座舱助手"
                    }
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: panel.compact ? 18 : 23
                        font.weight: Font.Medium
                        elide: panel.compact ? Text.ElideRight : Text.ElideNone
                        maximumLineCount: panel.compact ? 1 : 2
                        wrapMode: panel.compact ? Text.NoWrap : Text.WordWrap
                        text: panel.candidates && !panel.compact ? "你想播放哪一项？"
                            : AssistantSpeech.recording ? "说完再按一下"
                            : AssistantSpeech.recognizing || AssistantNlu.busy ? "请稍候…"
                            : Assistant.message || (AssistantSpeech.message && !panel.transcript ? AssistantSpeech.message : "")
                              || (panel.compact ? "按麦克风说话" : "我能帮你做什么？")
                    }
                }
                UiButton {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    visible: panel.compact && !panel.speaking
                    iconName: AssistantSpeech.recording ? "stop" : "mic"
                    caption: AssistantSpeech.recording ? "结束录音" : "按键说话"
                    enabled: AssistantSpeech.recording || (!panel.working && AssistantSpeech.configured)
                    onClicked: panel.record()
                }
                UiButton {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 44
                    iconName: panel.speaking ? "stop" : "close"
                    caption: panel.speaking ? "停止播报" : "关闭助手"
                    onClicked: {
                        if (panel.speaking) AssistantVoice.stop()
                        else panel.shell.popup = ""
                    }
                }
            }
            Label {
                visible: !panel.compact && panel.transcript !== ""
                Layout.fillWidth: true; elide: Text.ElideRight
                text: "识别内容  ·  " + panel.transcript
                color: Theme.muted; font.pixelSize: 15
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
                    onClicked: Assistant.choose(index)
                }
            }
            Label {
                visible: !panel.compact && panel.candidates
                Layout.fillWidth: true
                text: "点选一项，或说「选第二个」"; color: Theme.muted; font.pixelSize: 15
            }
            GridLayout {
                visible: !panel.compact && panel.followingUp
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
            UiButton {
                visible: !panel.compact && panel.followingUp
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 92
                Layout.preferredHeight: 36
                text: "取消"; textSize: 16; quiet: true
                onClicked: {
                    AssistantVoice.stop()
                    AssistantNlu.cancelFollowUp()
                }
            }
            RowLayout {
                visible: !panel.compact
                Layout.fillWidth: true; spacing: 7
                UiButton {
                    objectName: "assistantRecord"
                    Layout.preferredWidth: 48; Layout.preferredHeight: 45
                    iconName: AssistantSpeech.recording ? "stop" : "mic"
                    caption: AssistantSpeech.recording ? "结束录音" : "按键说话"
                    selected: AssistantSpeech.recording
                    enabled: AssistantSpeech.recording || (!panel.working && AssistantSpeech.configured)
                    onClicked: panel.record()
                }
                Controls.TextField {
                    id: request
                    objectName: "assistantText"
                    Layout.fillWidth: true; Layout.preferredHeight: 45
                    placeholderText: panel.followingUp ? "输入位置，或说「取消」" : "输入指令"
                    color: Theme.ink; font.pixelSize: 17
                    enabled: !panel.working
                    selectByMouse: true
                    background: Rectangle { radius: 12; color: Theme.secondary }
                    onAccepted: send.clicked()
                }
                UiButton {
                    id: send
                    Layout.preferredWidth: 70; Layout.preferredHeight: 45
                    text: "发送"; textSize: 16; primary: true
                    enabled: request.enabled && request.text.trim().length > 0
                    onClicked: {
                        if (!enabled) return
                        panel.transcript = ""
                        AssistantNlu.submit(request.text)
                        request.clear()
                    }
                }
                UiButton {
                    Layout.preferredWidth: 42; Layout.preferredHeight: 45
                    text: "···"; textSize: 22; caption: "更多"
                    onClicked: more.open()
                    Controls.Menu {
                        id: more
                        width: 216
                        padding: 7
                        x: parent.width - width
                        y: -height - 8
                        background: Rectangle { radius: 15; color: Theme.surface; border.color: Theme.line }
                        delegate: Controls.MenuItem {
                            id: menuItem
                            height: 43
                            contentItem: Label {
                                text: menuItem.text; font.pixelSize: 17
                                verticalAlignment: Text.AlignVCenter
                                opacity: menuItem.enabled ? 1 : 0.5
                            }
                            background: Rectangle { radius: 10; color: menuItem.highlighted ? Theme.active : "transparent" }
                        }
                        Controls.Action { text: "导入指令 WAV"; enabled: !panel.working && AssistantSpeech.configured; onTriggered: replay.open() }
                        Controls.Action { text: "座舱助手设置"; onTriggered: panel.shell.openPopup("assistantSettings") }
                    }
                }
            }
        }
    }
}
