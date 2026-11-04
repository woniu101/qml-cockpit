import QtQuick
Item {
    id: overlay
    required property var shell
    anchors.fill:parent;visible:shell.popup!=="" && shell.popup!=="assistant"
    Rectangle {anchors.fill:parent;color:"#23332f";opacity:0.18;MouseArea{anchors.fill:parent;onClicked:overlay.shell.popup=""}}
    Surface {
        id: panel
        x:overlay.shell.popup==="assistantSettings"?44:overlay.shell.popup==="climate"?410:510
        y:overlay.shell.popup==="assistantSettings"?80:overlay.shell.popup==="climate"?430:155
        width:overlay.shell.popup==="assistantSettings"?1512:overlay.shell.popup==="climate"?780:580
        height:overlay.shell.popup==="assistantSettings"?690:overlay.shell.popup==="climate"?333:610
        MouseArea {anchors.fill:parent}
        UiButton {objectName:"closePopup";x:parent.width-63;y:14;width:47;height:47;quiet:true;iconName:"close";z:10;onClicked:overlay.shell.popup=""}
        Loader {
            id: contents;anchors.fill:parent;active:overlay.visible
            sourceComponent: ({assistantSettings:assistantSettingsComponent,weather:weatherComponent,musicQueue:musicManagerComponent,musicFolders:musicManagerComponent,videoFolders:videoFoldersComponent,bluetooth:bluetoothComponent,audio:audioComponent,system:systemComponent,source:vehicleSourceComponent,climate:climateComponent,destination:destinationComponent,settings:settingsComponent,notifications:notificationsComponent,driving:drivingComponent,demo:demoComponent,charging:chargingComponent})[overlay.shell.popup]||null
        }
    }
    KeyboardPad {id:keyboard;x:460;y:534;width:680;height:235;z:11;visible:false;input:overlay.shell.popup==="destination"&&contents.item?contents.item.input:null}
    Connections {target:overlay.shell;function onPopupChanged(){keyboard.visible=false}}
    Component {id:destinationComponent;DestinationPanel {shell:overlay.shell;onKeyboardRequested:keyboard.visible=!keyboard.visible}}
    Component {id:settingsComponent;SettingsPanel {shell:overlay.shell}}
    Component {id:vehicleSourceComponent;SourcePanel {shell:overlay.shell}}
    Component {id:systemComponent;SystemPanel {shell:overlay.shell}}
    Component {id:bluetoothComponent;BluetoothPanel {shell:overlay.shell}}
    Component {id:audioComponent;AudioPanel {}}
    Component {id:weatherComponent;WeatherPanel {}}
    Component {id:assistantSettingsComponent;AssistantSettings {shell:overlay.shell}}
    Component {id:videoFoldersComponent;VideoFoldersPanel {shell:overlay.shell}}
    Component {id:musicManagerComponent;MusicManagerPanel {shell:overlay.shell}}
    Component {id:notificationsComponent;NotificationsPanel {shell:overlay.shell}}
    Component {id:drivingComponent;DrivingPanel {shell:overlay.shell}}
    Component {id:demoComponent;DemoPanel {shell:overlay.shell}}
    Component {
        id:climateComponent
        Item {
            Label {x:30;y:23;text:"空调";font.pixelSize:28;font.weight:Font.Medium}
            Label {x:31;y:76;text:"主驾 "+Vehicle.driverTemperature.toFixed(1)+"°";font.pixelSize:29}
            Label {x:453;y:76;text:"副驾 "+Vehicle.passengerTemperature.toFixed(1)+"°";font.pixelSize:29}
            SoftSlider {objectName:"driverTemperatureSlider";x:21;y:132;width:315;from:16;to:30;stepSize:0.5;value:Vehicle.driverTemperature;onMoved:Vehicle.driverTemperature=value}
            SoftSlider {x:443;y:132;width:315;from:16;to:30;stepSize:0.5;value:Vehicle.passengerTemperature;onMoved:Vehicle.passengerTemperature=value}
            Label {x:31;y:200;text:"风量 "+Vehicle.fan;font.pixelSize:21}
            SoftSlider {objectName:"fanSlider";x:119;y:188;width:320;from:0;to:7;stepSize:1;value:Vehicle.fan;onMoved:Vehicle.fan=value}
            UiButton {x:461;y:187;width:124;height:52;text:"AUTO";selected:Vehicle.automatic;onClicked:Vehicle.automatic=!Vehicle.automatic}
            UiButton {x:608;y:187;width:124;height:52;text:"A/C";selected:Vehicle.acOn;onClicked:Vehicle.acOn=!Vehicle.acOn}
            UiButton {x:30;y:263;width:158;height:45;text:"前挡除雾";iconName:"defrost";selected:Vehicle.defrost;textSize:18;onClicked:Vehicle.defrost=!Vehicle.defrost}
            UiButton {x:210;y:263;width:158;height:45;text:"座椅加热";iconName:"seat";selected:Vehicle.seatHeat;textSize:18;onClicked:Vehicle.seatHeat=!Vehicle.seatHeat}
            UiButton {objectName:"climateSync";x:390;y:263;width:158;height:45;text:"SYNC";selected:Vehicle.climateSync;textSize:18;onClicked:Vehicle.climateSync=!Vehicle.climateSync}
            Label {x:575;y:274;text:"除雾退出后恢复风量";font.pixelSize:14;color:Theme.muted}
        }
    }
    Component {
        id:chargingComponent
        Item {
            Label {x:30;y:27;text:"附近充电站";font.pixelSize:28}
            Label {objectName:"nearbyChargeSoc";x:30;y:99;text:EnergyVisual.socText;font.pixelSize:45;color:EnergyVisual.readingColor}
            Label {objectName:"nearbyChargeRange";x:260;y:123;width:290;text:(EnergyVisual.realSystem?"估算续航 ":"续航 ")+EnergyVisual.rangeText;font.pixelSize:21}
            Repeater {model:["滨江充电站","云杉快充站","望江公共充电站"]
                UiButton {required property string modelData;required property int index;x:30;y:220+index*79;width:520;height:62;text:modelData+" · "+Vehicle.routePreview(modelData).km+" km";textSize:21;iconName:"charge";onClicked:overlay.shell.navigateTo(modelData)}
            }
            UiButton {x:30;y:486;width:520;height:56;text:"打开车辆补能控制";onClicked:overlay.shell.inspectPart("port")}
            Label {x:30;y:569;text:EnergyVisual.realSystem?"站点为离线示意 · 电量来自 Windows":"站点与补能均为离线演示";font.pixelSize:15;color:Theme.muted}
        }
    }
}
