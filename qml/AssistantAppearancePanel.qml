import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    implicitHeight: Math.max(editColumn.implicitHeight, preview.implicitHeight)
    readonly property bool editing: AssistantAppearance.original.toString() !== ""
    property string previewExpression: "idle"
    property string loadedSource: ""
    function commit() { if (editing) AssistantAppearance.crop(editor.zoom, editor.horizontal, editor.vertical) }
    Connections {
        target: AssistantAppearance
        function onChanged() {
            const source = AssistantAppearance.original.toString()
            if (source !== panel.loadedSource) {
                panel.loadedSource = source
                editor.zoom = AssistantAppearance.zoom
                editor.horizontal = AssistantAppearance.horizontal
                editor.vertical = AssistantAppearance.vertical
            }
        }
    }
    FileDialog {
        id: picker
        title: "选择助手形象"
        nameFilters: ["图片 (*.png *.jpg *.jpeg)"]
        onAccepted: AssistantAppearance.load(selectedFile)
    }
    ColumnLayout {
        id: editColumn
        width: parent.width - 352; spacing: 16
        Label { text: "形象"; font.pixelSize: 28 }
        AssistantCard {
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                Label { text: panel.editing ? "编辑头像" : "当前形象"; font.pixelSize: 21; Layout.fillWidth: true }
                UiButton { text: "选择图片"; iconName: "folder"; textSize: 17; Layout.preferredWidth: 160; onClicked: picker.open() }
            }
            Label {
                visible: !panel.editing
                Layout.fillWidth: true; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap
                text: "导入自己的图片，右侧可查看在界面里的实际效果。"
            }
            RowLayout {
                visible: panel.editing
                Layout.fillWidth: true; spacing: 24
                Rectangle {
                    Layout.preferredWidth: 270; Layout.preferredHeight: 270
                    radius: 18; color: Theme.surface
                    AvatarCropEditor {
                        id: editor
                        objectName: "assistantCropCanvas"
                        anchors.fill: parent; anchors.margins: 10
                        source: AssistantAppearance.original
                        imageSize: panel.editing ? AssistantAppearance.originalSize : Qt.size(384,384)
                        zoom: AssistantAppearance.zoom
                        horizontal: AssistantAppearance.horizontal
                        vertical: AssistantAppearance.vertical
                        onCommitted: panel.commit()
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 16
                    Label { Layout.fillWidth: true; text: "拖动调整位置，滚轮缩放。触屏可用双指缩放。"; font.pixelSize: 16; wrapMode: Text.WordWrap; color: Theme.muted }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "缩放"; font.pixelSize: 17; Layout.fillWidth: true }
                        Label { text: editor.zoom.toFixed(2) + "×"; font.pixelSize: 16 }
                    }
                    SoftSlider {
                        Layout.fillWidth: true; from: 1; to: 3; stepSize: 0.01; value: editor.zoom
                        onMoved: { editor.resizeCrop(value); panel.commit() }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        UiButton { text: "居中"; textSize: 16; Layout.fillWidth: true; onClicked: { editor.horizontal = 50; editor.vertical = 50; panel.commit() } }
                        UiButton { text: "重置"; textSize: 16; Layout.fillWidth: true; onClicked: { editor.zoom = 1; editor.horizontal = 50; editor.vertical = 50; panel.commit() } }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        UiButton { text: "取消修改"; textSize: 17; Layout.fillWidth: true; onClicked: AssistantAppearance.discard() }
                        UiButton { text: "保存并启用"; primary: true; textSize: 17; Layout.fillWidth: true; onClicked: { panel.commit(); AssistantAppearance.save() } }
                    }
                }
            }
            UiButton { visible: !panel.editing && AssistantAppearance.custom; text: "恢复默认羊角帽"; iconName: "refresh"; textSize: 17; onClicked: AssistantAppearance.restoreDefault() }
            Label { visible: AssistantAppearance.message !== ""; Layout.fillWidth: true; text: AssistantAppearance.message; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
        }
        AssistantCard {
            Layout.fillWidth: true
            SoftSwitch { text: "显示助手形象"; checked: AssistantAppearance.shown; onToggled: AssistantAppearance.shown = checked }
            SoftSwitch { text: "减少动效"; checked: AssistantAppearance.reducedMotion; onToggled: AssistantAppearance.reducedMotion = checked }
        }
    }
    AssistantCard {
        id: preview
        x: parent.width - width; width: 332
        Label { text: "使用效果"; font.pixelSize: 23 }
        AssistantAvatar {
            Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 160; Layout.preferredHeight: 160
            forceShown: true; previewMotion: true; expression: panel.previewExpression
            picture: panel.editing ? AssistantAppearance.original : AssistantAppearance.avatar
            custom: panel.editing || AssistantAppearance.custom
            cropRect: panel.editing ? editor.cropRect : Qt.rect(0,0,0,0)
        }
        Label { Layout.alignment: Qt.AlignHCenter; text: "形象预览"; font.pixelSize: 15; color: Theme.muted }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
        RowLayout {
            Layout.fillWidth: true
            Repeater {
                model: [{size:32,label:"底栏入口"}, {size:58,label:"会话头像"}]
                ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredHeight: 105; spacing: 10
                    Item {
                        Layout.fillWidth: true; Layout.preferredHeight: 64
                        AssistantAvatar {
                            anchors.centerIn: parent; width: modelData.size; height: width
                            forceShown: true
                            picture: panel.editing ? AssistantAppearance.original : AssistantAppearance.avatar
                            custom: panel.editing || AssistantAppearance.custom
                            cropRect: panel.editing ? editor.cropRect : Qt.rect(0,0,0,0)
                        }
                    }
                    Label { Layout.alignment: Qt.AlignHCenter; text: modelData.label; font.pixelSize: 16; color: Theme.muted }
                }
            }
        }
        Label { text: "状态预览"; font.pixelSize: 17 }
        GridLayout {
            Layout.fillWidth: true; columns: 3; rowSpacing: 6; columnSpacing: 6
            Repeater {
                model: [{v:"idle",t:"待机"},{v:"listening",t:"聆听"},{v:"working",t:"处理"},
                        {v:"speaking",t:"播报"},{v:"success",t:"完成"},{v:"error",t:"失败"}]
                UiButton {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredHeight: 38; textSize: 15
                    text: modelData.t; selected: panel.previewExpression === modelData.v
                    onClicked: panel.previewExpression = modelData.v
                }
            }
        }
        Label { Layout.fillWidth: true; text: AssistantAppearance.custom || panel.editing ? "自定义图片使用状态光圈，可随时减少动效。" : "默认形象提供眨眼与播报动效。"; font.pixelSize: 15; color: Theme.muted; wrapMode: Text.WordWrap }
    }
}
