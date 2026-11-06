import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: field
    implicitWidth: 240
    implicitHeight: 100
    clip: true
    font.pixelSize: 17
    property alias text: editor.text
    property alias placeholderText: editor.placeholderText
    property alias readOnly: editor.readOnly
    readonly property bool editing: editor.activeFocus
    function clear() { editor.clear() }
    function selectAll() { editor.selectAll() }
    function copy() { editor.copy() }
    function deselect() { editor.deselect() }
    contentWidth: availableWidth
    contentHeight: editor.implicitHeight
    Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
    background: Rectangle {
        radius: 12; color: Theme.surface
        border.width: 1
        border.color: field.editing ? Theme.green : Theme.line
    }
    Controls.TextArea {
        id: editor
        width: field.availableWidth
        height: implicitHeight
        implicitHeight: Math.max(field.availableHeight, contentHeight + topPadding + bottomPadding)
        padding: 12
        font: field.font
        color: Theme.ink
        placeholderTextColor: Theme.muted
        selectByMouse: true
        wrapMode: TextEdit.Wrap
        textFormat: TextEdit.PlainText
        background: null
    }
}
