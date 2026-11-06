import QtQuick
Item {
    id: drivingPage
    objectName: "drivingPage"
    required property var shell
    width: 1600; height: 790
    onVisibleChanged: if(visible)Qt.callLater(function(){driveScene.interior=false;driveScene.moveView(145,-18,710)})
    Label { x: 40; y: 95; text: "行驶演示"; font.pixelSize: 32; font.weight: Font.Medium }
    Label { x: 935; y: 105; width: 615; text: !Vehicle.energyReady?"数据待更新 · 行驶画面已暂停":Vehicle.speed>0?"车轮与路面随车速同步":"已停止 · 可旋转查看车辆"; horizontalAlignment: Text.AlignRight; font.pixelSize: 17; color: Vehicle.energyReady?Theme.muted:EnergyVisual.stateColor }
    Surface {
        x: 40; y: 165; width: 340; height: 440
        Label { x: 26; y: 24; text: !Vehicle.energyReady?"最后确认状态":Vehicle.speed>0?"正在行驶":Vehicle.parked?"车辆已驻车":"车辆静止"; font.pixelSize: 22; color: Theme.green }
        Label { objectName: "driveGearLabel"; x: 26; y: 78; text: Vehicle.gear; font.pixelSize: 58; font.weight: Font.Medium }
        Label { objectName: "driveSpeedLabel"; x: 105; y: 70; width: 200; text: Vehicle.speed; font.pixelSize: 78; font.weight: Font.Light; horizontalAlignment: Text.AlignRight }
        Label { x: 235; y: 162; text: "km/h"; font.pixelSize: 19; color: Theme.muted }
        Rectangle { x: 26; y: 205; width: 288; height: 1; color: Theme.line }
        Label { x: 26; y: 227; text: "本次行驶"; font.pixelSize: 18; color: Theme.muted }
        Label { objectName: "driveTripLabel"; x: 26; y: 257; text: Vehicle.tripKm.toFixed(2)+" km"; font.pixelSize: 29 }
        Label { x: 26; y: 315; text: "电量 "+EnergyVisual.socText; font.pixelSize: 21; color: EnergyVisual.readingColor }
        Label { x: 26; y: 353; text: (EnergyVisual.realSystem?"估算续航 ":"预计续航 ")+EnergyVisual.rangeText; font.pixelSize: 19; color: Theme.muted }
        Label { x: 26; y: 399; text: "挂 P 停止 · R 挡反向移动"; font.pixelSize: 16; color: Theme.muted }
    }
    Surface {
        x: 400; y: 165; width: 1150; height: 440; clip: true
        VehicleScene {
            id: driveScene; objectName: "drivingVehicleScene"; x: 14; y: 14; width: parent.width-28; height: 326; drivingMode: true; energyDetail: "off"; activeView: drivingPage.visible
            interactive: !drivingPage.shell.modalOpen
            animationPaused: drivingPage.shell.demoRunning&&drivingPage.shell.demoPaused
            yaw: 145; elevation: -18; distance: 710
        }
        UiButton { x: 938; y: 20; width: 84; height: 39; text: "前视"; textSize: 16; onClicked: driveScene.moveView(35,-18,710) }
        UiButton { x: 1030; y: 20; width: 98; height: 39; text: "跟车视角"; textSize: 16; onClicked: driveScene.moveView(145,-18,710) }
        Surface {
            x: 22; y: 350; width: 1106; height: 70; radius: 18
            Icon { x: 17; y: 22; name: Vehicle.navigationActive?"nav":"car"; width: 26; height: 26; tint: Theme.green }
            Label { objectName: "driveRouteLabel"; x: 58; y: 10; width: 720; text: Vehicle.navigationActive?"导航至 "+Vehicle.destination:"自由行驶 · 未设置目的地"; font.pixelSize: 21; elide: Text.ElideRight }
            Label { x: 58; y: 40; width: 720; text: Vehicle.navigationActive?Vehicle.remainingKm.toFixed(1)+" km · "+Vehicle.remainingMinutes+" 分钟 · "+Vehicle.laneHint:"可直接调整车速，路线与挡位分别控制"; font.pixelSize: 15; color: Theme.muted; elide: Text.ElideRight }
            UiButton { objectName: "driveNavigation"; x: 897; y: 12; width: 193; height: 46; text: Vehicle.navigationActive?"查看导航地图":"选择目的地"; textSize: 18; iconName: "nav"; onClicked: {if(Vehicle.navigationActive)drivingPage.shell.showPage(AppRoutes.Navigation);else drivingPage.shell.openPopup("destination")} }
        }
    }
    Surface {
        x: 40; y: 625; width: 1510; height: 140
        enabled: Vehicle.externalReady
        Label { x: 26; y: 18; text: "挡位"; font.pixelSize: 16; color: Theme.muted }
        Row { x: 22; y: 52; spacing: 8
            Repeater { model: ["P","R","N","D"]
                UiButton { required property string modelData; objectName: "driveGear"+modelData; width: 65; height: 58; text: modelData; textSize: 26; selected: Vehicle.gear===modelData; onClicked: Vehicle.gear=modelData }
            }
        }
        Label { x: 350; y: 18; text: "模拟车速 · "+Vehicle.speed+" km/h"; font.pixelSize: 17; color: Theme.muted }
        SoftSlider { objectName: "drivePageSpeed"; x: 340; y: 57; width: 320; height: 48; from: 0; to: Vehicle.gear==="R"?15:120; stepSize: 1; value: Vehicle.speed; enabled: Vehicle.gear==="D"||Vehicle.gear==="R"; onMoved: Vehicle.speed=Math.round(value) }
        UiButton { objectName: "driveResume"; x: 700; y: 50; width: 225; height: 60; text: Vehicle.speed>0?"恢复 40 km/h":"开始行驶 40 km/h"; textSize: 19; primary: true; onClicked: Vehicle.startDriving() }
        UiButton { objectName: "drivePark"; x: 945; y: 50; width: 205; height: 60; text: "挂 P 挡驻车"; textSize: 19; selected: Vehicle.parked; onClicked: Vehicle.parkVehicle() }
        UiButton { objectName: "drivePageFastForward"; x: 1170; y: 50; width: 182; height: 60; text: "快进 1 分钟"; textSize: 18; enabled: Vehicle.speed>0; onClicked: Vehicle.advanceSimulation(60) }
        UiButton { x: 1370; y: 50; width: 115; height: 60; text: "车辆控制"; textSize: 18; onClicked: drivingPage.shell.showPage(AppRoutes.Vehicle) }
    }
}
