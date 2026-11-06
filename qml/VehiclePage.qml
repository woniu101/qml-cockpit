import QtQuick
Item {
    id: vehiclePageRoot
    required property var shell
    property int section: 1
    property int selectedWindow: 0
    property bool doorMode: false
    property string selectedPart: "window0"
    readonly property var windowNames: ["左前","右前","左后","右后"]
    readonly property var titles: ["快捷控制","车门与车窗","座椅与空调","外部灯光","后视镜","充电与补能"]
    width: 1600; height: 790
    function choosePart(index,mode,moveCamera=true) {
        selectedWindow=index;doorMode=mode;section=1;selectedPart=(mode?"door":"window")+index
        if(moveCamera && Vehicle.autoFocus)vehicleScene.focusPart(selectedPart)
    }
    function focusSelected() { vehicleScene.focusPart(selectedPart) }
    function lookAt(angle) {vehicleScene.interior=false;vehicleScene.moveView(angle)}
    function inspectPart(part) {
        if(part.indexOf("door")===0 || part.indexOf("window")===0){choosePart(Number(part.slice(-1)),part.indexOf("door")===0,false);focusSelected()}
        else if(part==="port"){section=5;selectedPart="port";focusSelected()}
        else if(part==="mirrors"){section=4;selectedPart="";lookAt(65)}
        else {section=0;selectedPart=part==="hood"||part==="hatch"?part:"";if(selectedPart!=="")focusSelected()}
    }
    function partTitle() {
        if(selectedPart.indexOf("window")===0){const i=Number(selectedPart.slice(-1));return windowNames[i]+"车窗 · 开度 "+Math.round(Vehicle.windowActual[i])+"%"}
        if(selectedPart.indexOf("door")===0){const i=Number(selectedPart.slice(-1));return windowNames[i]+"车门 · "+(Vehicle.doorBusy[i]?"开合中":Vehicle.doors[i]?"已打开":"已关闭")}
        return selectedPart==="hood"?"前备厢":selectedPart==="hatch"?"后备厢":selectedPart==="port"?"充电口 · 左后侧":""
    }
    Connections { target: Vehicle; function onVisualReset(){vehiclePageRoot.selectedWindow=0;vehiclePageRoot.doorMode=false;vehiclePageRoot.section=1;vehiclePageRoot.selectedPart="window0"} }
    Label { x: 43; y: 90; text: "车辆"; font.pixelSize: 34; font.weight: Font.Medium }
    Rectangle { x: 249; y: 166; width: 1; height: 579; color: Theme.line }
    Repeater {
        model: [{icon:"settings",title:"快捷控制"},{icon:"window",title:"车门车窗"},{icon:"seat",title:"座椅空调"},{icon:"light",title:"外部灯光"},{icon:"mirror",title:"后视镜"},{icon:"charge",title:"充电补能"}]
        UiButton {
            required property var modelData; required property int index
            objectName: "vehicleSection"+index
            x: 24; y: 171+index*77; width: 202; height: 62; selected: index===vehiclePageRoot.section; quiet: true
            contentItem: Item {
                Icon { x: 9; y: 8; width: 26; height: 26; name: modelData.icon; tint: index===vehiclePageRoot.section?Theme.green:Theme.muted }
                Label { x: 54; y: 5; text: modelData.title; font.pixelSize: 23; color: index===vehiclePageRoot.section?Theme.green:Theme.muted }
            }
            onClicked: {
                vehiclePageRoot.section=index
                vehiclePageRoot.selectedPart=index===1?(vehiclePageRoot.doorMode?"door":"window")+vehiclePageRoot.selectedWindow:index===5?"port":""
                if(Vehicle.autoFocus && vehiclePageRoot.selectedPart!=="")vehiclePageRoot.focusSelected()
                else if(Vehicle.autoFocus && index===4)vehiclePageRoot.lookAt(65)
            }
        }
    }
    Label { x: 290; y: 95; text: vehiclePageRoot.titles[vehiclePageRoot.section]; font.pixelSize: 30; font.weight: Font.Medium }
    Label { x: 290; y: 171; text: vehiclePageRoot.partTitle(); font.pixelSize: 19; color: Theme.green }
    Label { x: 738; y: 173; width: 388; text: "拖动旋转 · 双指或滚轮缩放"; font.pixelSize: 16; horizontalAlignment: Text.AlignRight; color: Theme.muted }
    VehicleScene {
        id: vehicleScene; objectName: "controlsVehicleScene"; x: 260; y: 213; width: 885; height: 460
        activeView: vehiclePageRoot.visible
        interactive: !vehiclePageRoot.shell.modalOpen
        energyDetail: vehiclePageRoot.section===5?"detailed":"ambient"
        selectedPart: vehiclePageRoot.selectedPart; selectedCaption: vehiclePageRoot.selectedPart==="port"?"充电口":vehiclePageRoot.partTitle()
        onPartTapped: part=>{
            const codes=["lf","rf","lr","rr"],index=codes.findIndex(p=>part.indexOf("door_"+p+"_")===0)
            if(index>=0)vehiclePageRoot.choosePart(index,part.indexOf("glass")<0,false)
            else if(part.indexOf("bonnet")>=0)vehiclePageRoot.inspectPart("hood")
            else if(part.indexOf("boot")>=0)vehiclePageRoot.inspectPart("hatch")
            else if(part.indexOf("charge")>=0)vehiclePageRoot.inspectPart("port")
        }
    }
    UiButton { objectName: "exteriorView"; x: 290; y: 706; width: 92; height: 45; text: "车外"; textSize: 18; selected: !vehicleScene.interior; onClicked: vehicleScene.interior=false }
    UiButton { objectName: "interiorView"; x: 392; y: 706; width: 92; height: 45; text: "车内"; textSize: 18; selected: vehicleScene.interior; onClicked: vehicleScene.interior=true }
    UiButton { objectName: "frontView"; x: 503; y: 706; width: 84; height: 45; text: "前视"; textSize: 17; quiet: true; onClicked: vehiclePageRoot.lookAt(0) }
    UiButton { objectName: "sideView"; x: 588; y: 706; width: 84; height: 45; text: "侧视"; textSize: 17; quiet: true; onClicked: vehiclePageRoot.lookAt(90) }
    UiButton { objectName: "rearView"; x: 673; y: 706; width: 84; height: 45; text: "后视"; textSize: 17; quiet: true; onClicked: vehiclePageRoot.lookAt(180) }
    UiButton { objectName: "resetVehicleView"; x: 770; y: 706; width: 50; height: 45; quiet: true; iconName: "refresh"; caption: "恢复默认视角"; onClicked: vehicleScene.resetView() }
    UiButton { objectName: "focusSelectedPart"; x: 836; y: 706; width: 128; height: 45; text: "定位部件"; textSize: 17; iconName: "locate"; enabled: vehiclePageRoot.selectedPart!==""; onClicked: vehiclePageRoot.focusSelected() }
    UiButton { objectName: "autoFocusButton"; x: 976; y: 706; width: 150; height: 45; text: Vehicle.autoFocus?"自动定位 开":"自动定位 关"; textSize: 16; selected: Vehicle.autoFocus; onClicked: Vehicle.autoFocus=!Vehicle.autoFocus }
    Surface {
        x: 1160; y: 165; width: 395; height: 600
        Loader {
            enabled: Vehicle.externalReady || vehiclePageRoot.section===2 || vehiclePageRoot.section===3
            anchors.fill: parent
            sourceComponent: vehiclePageRoot.section===0?quickComponent:vehiclePageRoot.section===1?(vehiclePageRoot.doorMode?doorComponent:windowComponent):vehiclePageRoot.section===5?chargeComponent:comfortComponent
        }
    }
    Component { id: quickComponent; QuickControls { page: vehiclePageRoot } }
    Component { id: windowComponent; WindowControls { page: vehiclePageRoot } }
    Component { id: doorComponent; DoorControls { page: vehiclePageRoot } }
    Component { id: chargeComponent; ChargeControls { shell: vehiclePageRoot.shell } }
    Component { id: comfortComponent; ComfortControls { page: vehiclePageRoot } }
}
