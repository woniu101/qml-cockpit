import QtQuick
Item {
    id: selector
    property int selectedIndex: 0
    property bool doorMode: false
    signal chosen(int index)
    height: 170
    Label { x: 0; y: 0; width: parent.width; text: "车头 ↑"; horizontalAlignment: Text.AlignHCenter; color: Theme.muted; font.pixelSize: 14 }
    Repeater {
        model: 4
        UiButton {
            required property int index
            readonly property int opening: Math.round(Vehicle.windowActual[index])
            readonly property bool moving: selector.doorMode?Vehicle.doorBusy[index]:Vehicle.windowBusy[index]
            objectName: "windowSelect"+index
            x: index%2*(selector.width+16)/2; y: 27+Math.floor(index/2)*73
            width: (selector.width-16)/2; height: 63
            Accessible.name: ["左前","右前","左后","右后"][index]+(selector.doorMode?"车门":"车窗")
            background: Rectangle { radius: 15; color: index===selector.selectedIndex?Theme.surface:Theme.secondary; border.width: index===selector.selectedIndex?2:1; border.color: index===selector.selectedIndex?Theme.green:Theme.line }
            contentItem: Item {
                Icon { x: 3; y: 9; width: 22; height: 22; name: selector.doorMode?"door":"window"; tint: index===selector.selectedIndex?Theme.green:Theme.muted }
                Label { x: 36; y: -1; text: ["左前","右前","左后","右后"][index]; font.pixelSize: 20 }
                Label { x: 36; y: 27; font.pixelSize: 14; color: Theme.muted; text: selector.doorMode?(moving?"开合中":Vehicle.doors[index]?"已打开":"已关闭"):(moving?"调整中 "+opening+"%":opening===0?"全关 0%":opening===100?"全开 100%":"开度 "+opening+"%") }
            }
            onClicked: selector.chosen(index)
        }
    }
}
