import QtQuick
import QtQuick.Controls
import "." as App
Item {
    id: control
    required property var page
    readonly property int index: page.selectedWindow
    readonly property real actual: Vehicle.windowActual[index]
    readonly property int target: Vehicle.windowTargets[index]
    readonly property bool moving: Vehicle.windowBusy[index]
    App.Label { x: 24; y: 22; text: "门窗"; font.pixelSize: 25; font.weight: Font.Medium }
    UiButton { objectName: "windowMode"; x: 154; y: 18; width: 98; height: 43; text: "车窗"; textSize: 18; selected: true; onClicked: control.page.choosePart(control.index,false) }
    UiButton { objectName: "doorMode"; x: 263; y: 18; width: 98; height: 43; text: "车门"; textSize: 18; onClicked: control.page.choosePart(control.index,true) }
    CornerSelector { x: 24; y: 72; width: parent.width-48; selectedIndex: control.index; onChosen: index=>control.page.choosePart(index,false) }
    App.Label { x: 24; y: 244; text: control.page.windowNames[control.index]+"车窗"; font.pixelSize: 24; font.weight: Font.Medium }
    App.Label { x: 24; y: 288; text: Math.round(control.actual)+"%"; font.pixelSize: 43 }
    App.Label { x: 133; y: 307; text: control.actual<0.1?"全关":control.actual>99.9?"全开":"部分开启"; font.pixelSize: 17; color: Theme.muted }
    App.Label { x: 266; y: 258; text: "设置开度 %"; font.pixelSize: 14; color: Theme.muted }
    SpinBox {
        id: percentInput
        objectName: "windowPercentInput"; x: 262; y: 286; width: 111; height: 47
        from: 0; to: 100; stepSize: 1; editable: true; value: control.target
        font.family: "Microsoft YaHei UI"; font.pixelSize: 19
        onValueModified: Vehicle.setWindow(control.index,value)
        background: Rectangle { radius: 12; color: Theme.secondary; border.color: percentInput.activeFocus?Theme.green:Theme.line }
        contentItem: TextInput {
            objectName: "windowPercentText"
            text: percentInput.textFromValue(percentInput.value,percentInput.locale)
            font: percentInput.font; color: Theme.ink; selectionColor: Theme.green; selectedTextColor: Theme.surface
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            readOnly: !percentInput.editable; validator: percentInput.validator
            inputMethodHints: Qt.ImhDigitsOnly
        }
        down.indicator: Rectangle {
            x: 0; height: parent.height; width: 32; color: "transparent"
            App.Label { anchors.centerIn: parent; text: "−"; font.pixelSize: 23; color: percentInput.down.enabled?Theme.ink:Theme.muted }
        }
        up.indicator: Rectangle {
            x: parent.width-width; height: parent.height; width: 32; color: "transparent"
            App.Label { anchors.centerIn: parent; text: "+"; font.pixelSize: 23; color: percentInput.up.enabled?Theme.ink:Theme.muted }
        }
        Accessible.name: "车窗目标开度百分比"
    }
    SoftSlider {
        id: openingSlider
        objectName: "windowOpeningSlider"; x: 14; y: 349; width: parent.width-28; height: 47
        from: 0; to: 100; stepSize: 1; value: pressed&&Vehicle.externalMode?control.target:control.actual
        property int dragIndex: 0
        onPressedChanged: {
            if(pressed){dragIndex=control.index;Vehicle.beginWindowDrag(dragIndex)}
            else Vehicle.endWindowDrag(dragIndex)
        }
        onMoved: { if(pressed)Vehicle.previewWindow(dragIndex,Math.round(value));else Vehicle.setWindow(control.index,Math.round(value)) }
        Accessible.name: "车窗开度，0%全关，100%全开"
    }
    App.Label { x: 24; y: 399; text: "0% 全关"; font.pixelSize: 15; color: Theme.muted }
    App.Label { x: 268; y: 399; width: 103; text: "100% 全开"; horizontalAlignment: Text.AlignRight; font.pixelSize: 15; color: Theme.muted }
    Repeater {
        model: [{value:0,title:"全关"},{value:15,title:"通风15%"},{value:50,title:"半开50%"},{value:100,title:"全开"}]
        UiButton {
            required property var modelData; required property int index
            objectName: "windowPreset"+modelData.value
            x: 24+index*89; y: 435; width: 80; height: 43; textSize: 16
            text: modelData.title; selected: !control.moving && Math.abs(control.actual-modelData.value)<0.1
            onClicked: Vehicle.setWindow(control.index,modelData.value)
        }
    }
    App.Label {
        objectName: "windowLocalFeedback"; x: 24; y: 493; width: control.moving?244:347
        text: control.moving?"当前 "+Math.round(control.actual)+"% → 目标 "+control.target+"%":Vehicle.windowMessages[control.index]
        color: Theme.green; font.pixelSize: 16; wrapMode: Text.WordWrap
    }
    UiButton { objectName: "stopWindow"; visible: control.moving; x: 285; y: 485; width: 86; height: 43; text: "停止"; textSize: 18; primary: true; onClicked: Vehicle.stopWindow(control.index) }
    UiButton { objectName: "closeAllWindows"; x: 24; y: 544; width: 167; height: 45; text: "全部关窗"; iconName: "arrow-up"; textSize: 17; enabled: Vehicle.windowActual.some(v=>v>0.01)||Vehicle.windowBusy.some(v=>v); onClicked: Vehicle.closeWindows() }
    UiButton { objectName: "ventilateWindows"; x: 204; y: 544; width: 167; height: 45; text: "全车通风"; iconName: "window"; textSize: 17; onClicked: Vehicle.ventilateWindows() }
}
