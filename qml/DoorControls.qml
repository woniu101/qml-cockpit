import QtQuick
Item {
    id: control
    required property var page
    readonly property int index: page.selectedWindow
    readonly property bool moving: Vehicle.doorBusy[index]
    readonly property bool open: Vehicle.doors[index]
    Label { x: 24; y: 22; text: "门窗"; font.pixelSize: 25; font.weight: Font.Medium }
    UiButton { objectName: "windowMode"; x: 154; y: 18; width: 98; height: 43; text: "车窗"; textSize: 18; onClicked: control.page.choosePart(control.index,false) }
    UiButton { objectName: "doorMode"; x: 263; y: 18; width: 98; height: 43; text: "车门"; textSize: 18; selected: true; onClicked: control.page.choosePart(control.index,true) }
    CornerSelector { x: 24; y: 72; width: parent.width-48; selectedIndex: control.index; doorMode: true; onChosen: index=>control.page.choosePart(index,true) }
    Label { x: 24; y: 247; text: control.page.windowNames[control.index]+"车门"; font.pixelSize: 26; font.weight: Font.Medium }
    Label { x: 24; y: 289; text: control.moving?(control.open?"正在打开…":"正在关闭…"):control.open?"当前已打开":"当前已关闭"; font.pixelSize: 21; color: control.moving?Theme.green:Theme.muted }
    UiButton { objectName: "toggleSelectedDoor"; x: 24; y: 344; width: 347; height: 64; primary: !control.open; enabled: Vehicle.parked && !control.moving; text: (control.open?"关闭":"打开")+control.page.windowNames[control.index]+"车门"; iconName: "door"; onClicked: Vehicle.setDoor(control.index,!control.open) }
    Label { x: 24; y: 429; width: 347; text: !Vehicle.parked?"请先挂 P 挡，再操作车门。":"开门时自动解锁，关门完成后可以锁车。"; font.pixelSize: 17; color: Theme.muted; wrapMode: Text.WordWrap }
    UiButton { objectName: "vehicleLock"; x: 24; y: 486; width: 347; height: 47; text: Vehicle.locked?"解锁车辆":"锁定车辆"; iconName: Vehicle.locked?"lock":"unlock"; onClicked: Vehicle.toggleLock() }
    UiButton { objectName: "closeAllDoors"; x: 24; y: 546; width: 347; height: 43; text: "关闭全部车门"; textSize: 18; enabled: Vehicle.doors.some(v=>v); onClicked: Vehicle.closeDoors() }
}
