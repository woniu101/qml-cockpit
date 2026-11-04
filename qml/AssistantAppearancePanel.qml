import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: panel
    readonly property bool editing: AssistantAppearance.preview.toString().length > 0
    function updateCrop() { AssistantAppearance.crop(zoom.value, horizontal.value, vertical.value) }
    Timer { id: cropTimer; interval: 45; onTriggered: panel.updateCrop() }
    Component.onDestruction: if (AssistantAppearance) AssistantAppearance.discard()
    FileDialog {
        id: picker
        title: "选择助手头像"
        nameFilters: ["图片 (*.png *.jpg *.jpeg)"]
        onAccepted: if (AssistantAppearance.load(selectedFile)) {
            zoom.value = 1
            horizontal.value = 50
            vertical.value = 50
        }
    }
    ColumnLayout {
        anchors.left: parent.left; anchors.top: parent.top
        width: parent.width - 360
        spacing: 15
        Label { text: "形象设置"; font.pixelSize: 32 }
        Label { text: "选择本地图片，调整圆形头像的构图。"; color: Theme.muted; font.pixelSize: 17 }
        RowLayout {
            UiButton { text: "导入 PNG / JPG"; iconName: "folder"; Layout.preferredWidth: 190; onClicked: picker.open() }
            UiButton { text: "恢复默认形象"; Layout.preferredWidth: 170; enabled: AssistantAppearance.custom; onClicked: AssistantAppearance.restoreDefault() }
        }
        ColumnLayout {
            visible: panel.editing; Layout.fillWidth: true
            Label { text: "缩放"; font.pixelSize: 16 }
            SoftSlider { id: zoom; Layout.fillWidth: true; from: 1; to: 3; stepSize: 0.01; value: 1; onMoved: cropTimer.restart() }
            Label { text: "左右位置"; font.pixelSize: 16 }
            SoftSlider { id: horizontal; Layout.fillWidth: true; from: 0; to: 100; value: 50; onMoved: cropTimer.restart() }
            Label { text: "上下位置"; font.pixelSize: 16 }
            SoftSlider { id: vertical; Layout.fillWidth: true; from: 0; to: 100; value: 50; onMoved: cropTimer.restart() }
            RowLayout {
                UiButton { text: "保存并使用"; primary: true; Layout.preferredWidth: 150; onClicked: { cropTimer.stop(); panel.updateCrop(); AssistantAppearance.save() } }
                UiButton { text: "取消裁切"; quiet: true; onClicked: { cropTimer.stop(); AssistantAppearance.discard() } }
            }
        }
        UiButton {
            text: AssistantAppearance.shown ? "显示助手形象 · 开" : "显示助手形象 · 关"
            selected: AssistantAppearance.shown; Layout.preferredWidth: 232
            onClicked: AssistantAppearance.shown = !AssistantAppearance.shown
        }
        UiButton {
            text: AssistantAppearance.reducedMotion ? "减少动效 · 开" : "减少动效 · 关"
            selected: AssistantAppearance.reducedMotion; Layout.preferredWidth: 232
            onClicked: AssistantAppearance.reducedMotion = !AssistantAppearance.reducedMotion
        }
        Label { text: AssistantAppearance.message; color: Theme.muted; font.pixelSize: 16; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    }
    Surface {
        anchors.right: parent.right; anchors.top: parent.top
        width: 315; height: 430; radius: 22; color: Theme.secondary
        Column {
            anchors.centerIn: parent; spacing: 21
            Label { text: "圆形裁切预览"; font.pixelSize: 18; anchors.horizontalCenter: parent.horizontalCenter }
            AssistantAvatar {
                width: 184; height: width; forceShown: true
                picture: panel.editing ? AssistantAppearance.preview : AssistantAppearance.avatar
                custom: panel.editing || AssistantAppearance.custom
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter; spacing: 22
                AssistantAvatar { width: 34; height: width; forceShown: true; picture: panel.editing ? AssistantAppearance.preview : AssistantAppearance.avatar; custom: panel.editing || AssistantAppearance.custom }
                AssistantAvatar { width: 58; height: width; forceShown: true; picture: panel.editing ? AssistantAppearance.preview : AssistantAppearance.avatar; custom: panel.editing || AssistantAppearance.custom }
            }
        }
    }
}
