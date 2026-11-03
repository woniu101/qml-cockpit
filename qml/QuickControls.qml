import QtQuick
Item {
    id: quick
    required property var page
    function state(index) {
        return index===0?(Vehicle.locked?"已锁车":"已解锁"):index===1?(Vehicle.hoodBusy?"正在开合":Vehicle.hoodOpen?"已打开":"已关闭"):index===2?(Vehicle.hatchBusy?"正在开合":Vehicle.hatchOpen?"已打开":"已关闭"):index===3?(Vehicle.mirrorsFolded?"已折叠":"已展开"):index===4?(Vehicle.portOpen?"已打开":"已关闭"):index===5?(Vehicle.windows.every(v=>v===0)?"全部已关闭":"部分车窗开启"):index===6?"开度 15%":"P / D 与车速"
    }
    function act(index) {
        if(index===0)Vehicle.toggleLock()
        else if(index===1){page.selectedPart="hood";page.focusSelected();Vehicle.toggleHood()}
        else if(index===2){page.selectedPart="hatch";page.focusSelected();Vehicle.toggleHatch()}
        else if(index===3)Vehicle.toggleMirrors()
        else if(index===4){page.selectedPart="port";page.focusSelected();if(Vehicle.chargeControlAvailable)Vehicle.togglePort();else page.shell.inspectPart("port")}
        else if(index===5)Vehicle.closeWindows()
        else if(index===6)Vehicle.ventilateWindows()
        else page.shell.openPopup("driving")
    }
    Label { x: 24; y: 22; text: "快捷控制"; font.pixelSize: 26; font.weight: Font.Medium }
    Label { x: 24; y: 64; text: "常用操作集中在这里"; font.pixelSize: 17; color: Theme.muted }
    Repeater {
        model: [{key:"lock",title:"车门锁",icon:"lock"},{key:"hood",title:"前备厢",icon:"trunk"},{key:"hatch",title:"后备厢",icon:"trunk"},{key:"mirrors",title:"后视镜",icon:"mirror"},{key:"port",title:"充电口",icon:"charge"},{key:"closeWindows",title:"全部关窗",icon:"arrow-up"},{key:"vent",title:"全车通风",icon:"window"},{key:"driving",title:"驾驶演示",icon:"car"}]
        UiButton {
            required property var modelData; required property int index
            objectName: "quick_"+modelData.key
            x: 24+(index%2)*180; y: 106+Math.floor(index/2)*108; width: 167; height: 94
            enabled: index===1?!Vehicle.hoodBusy:index===2?!Vehicle.hatchBusy:true
            background: Rectangle { radius: 17; color: Theme.secondary; border.width: 1; border.color: Theme.line }
            contentItem: Item {
                Icon { x: 3; y: 1; name: index===0&&!Vehicle.locked?"unlock":modelData.icon; width: 25; height: 25; tint: Theme.green }
                Label { x: 40; y: 0; text: modelData.title; font.pixelSize: 19 }
                Label { x: 3; y: 42; text: quick.state(index); font.pixelSize: 15; color: Theme.muted }
            }
            onClicked: quick.act(index)
        }
    }
    Label { x: 24; y: 554; width: 347; text: "车窗百分比请进入「车门车窗」"; font.pixelSize: 16; color: Theme.muted }
}
