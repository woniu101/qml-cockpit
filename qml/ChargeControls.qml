import QtQuick
Item {
    id: charge
    required property var shell
    readonly property bool canControl: Vehicle.chargeControlAvailable
    readonly property bool targetReached: canControl && Vehicle.batteryPercent>=Vehicle.chargeTarget
    Label { x: 24; y: 22; text: "充电与补能"; font.pixelSize: 26; font.weight: Font.Medium }
    Rectangle { x: 24; y: 75; width: 9; height: 9; radius: 4.5; color: EnergyVisual.stateColor
        SequentialAnimation on opacity { running: charge.visible && EnergyVisual.flow; loops: Animation.Infinite; NumberAnimation { to: 0.4; duration: 900 } NumberAnimation { to: 1; duration: 900 } }
    }
    Label { objectName: "chargeStateLabel"; x: 43; y: 68; text: EnergyVisual.stateText; font.pixelSize: 20; color: EnergyVisual.stateColor }
    Label { objectName: "chargeSocLabel"; x: 24; y: 105; text: EnergyVisual.socText; font.pixelSize: 43; color: EnergyVisual.readingColor }
    Label { x: 208; y: 127; text: (EnergyVisual.realSystem?"估算 ":"续航 ")+EnergyVisual.rangeText; font.pixelSize: 19; color: Theme.muted }
    Rectangle { x: 24; y: 168; width: 347; height: 9; radius: 4.5; color: Theme.line
        Rectangle { id: chargeFill; width: parent.width*EnergyVisual.soc/100; height: parent.height; radius: 4.5; color: EnergyVisual.levelColor; clip: true
            Rectangle { width: 46; height: parent.height; radius: 4.5; color: "#ffffff"; opacity: 0.25; visible: EnergyVisual.flow
                NumberAnimation on x { from: -46; to: chargeFill.width; duration: 2300; loops: Animation.Infinite; running: charge.visible && EnergyVisual.flow }
            }
        }
        Rectangle { visible: charge.canControl; x: Math.min(parent.width-2,parent.width*Vehicle.chargeTarget/100); y: -3; width: 2; height: 15; radius: 1; color: Theme.muted; opacity: 0.65 }
    }
    Item { anchors.fill: parent; visible: charge.canControl
        Label { x: 24; y: 201; text: "目标电量 "+Vehicle.chargeTarget+"%"; font.pixelSize: 21 }
        SoftSlider { objectName: "chargeTargetSlider"; x: 14; y: 237; width: 367; from: 50; to: 100; stepSize: 1; value: Vehicle.chargeTarget; onMoved: Vehicle.chargeTarget=Math.round(value) }
        Label { x: 24; y: 286; width: 347; text: !Vehicle.energyReady?"等待最新车辆数据，充电状态待确认":Vehicle.charging?(Vehicle.chargePower.toFixed(0)+" kW · 预计 "+Vehicle.chargeMinutes+" 分钟达到目标"):!Vehicle.parked?"挂 P 挡后开始补能":Vehicle.plugged?(charge.targetReached?"已达目标，可断枪或提高目标电量":"充电枪已连接，可以开始或继续充电"):"驻车后打开充电口，再连接充电枪"; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
        UiButton { objectName: "chargePort"; x: 24; y: 332; width: 347; height: 47; text: Vehicle.portOpen?"关闭充电口":"打开充电口"; iconName: "charge"; enabled: Vehicle.parked&&!Vehicle.plugged; onClicked: Vehicle.togglePort() }
        UiButton { objectName: "chargePlug"; x: 24; y: 392; width: 347; height: 47; text: Vehicle.plugged?"断开充电枪":"连接充电枪"; enabled: Vehicle.parked&&Vehicle.portOpen&&!Vehicle.charging; onClicked: Vehicle.togglePlug() }
        UiButton { objectName: "chargeStart"; x: 24; y: 452; width: 347; height: 52; text: Vehicle.charging?"暂停充电":EnergyVisual.mode==="full"?"电池已充满":Vehicle.plugged&&charge.targetReached?"已达到目标电量":"开始充电"; iconName: Vehicle.charging?"pause":"charge"; primary: Vehicle.charging||!charge.targetReached; enabled: Vehicle.parked&&Vehicle.plugged&&(Vehicle.charging||!charge.targetReached); onClicked: Vehicle.charging?Vehicle.stopCharging():Vehicle.startCharging() }
        UiButton { objectName: "chargeFastForward"; x: 24; y: 519; width: 167; height: 43; text: "快进 5 分钟"; textSize: 17; enabled: Vehicle.charging; onClicked: Vehicle.advanceSimulation(300) }
        UiButton { objectName: "chargeNearbyStations"; x: 204; y: 519; width: 167; height: 43; text: "附近充电站"; textSize: 17; onClicked: charge.shell.openPopup("charging") }
    }
    Item { objectName: "systemChargeReadOnly"; x: 24; y: 201; width: 347; height: 350; visible: !charge.canControl
        Label { text: "实际供电状态 · 只读"; font.pixelSize: 21 }
        Rectangle { y: 42; width: parent.width; height: 165; radius: 16; color: Theme.secondary
            Label { x: 16; y: 17; width: 315; text: Host.power.statusText; font.pixelSize: 18; color: Theme.green; wrapMode: Text.WordWrap }
            Label { x: 16; y: 68; text: "充电功率：未提供\n预计时间：未提供\n充电目标：由系统管理"; font.pixelSize: 16; color: Theme.muted; lineHeight: 1.4 }
        }
        Label { y: 224; width: parent.width; text: "插拔电脑电源后自动刷新。模拟充电、目标电量和快进请使用内置演示或外部模拟器。"; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
        UiButton { objectName: "chargeChangeSource"; y: 301; width: parent.width; height: 44; text: "选择车辆数据来源"; textSize: 18; onClicked: charge.shell.openPopup("source") }
    }
    Label { x: 24; y: 574; text: charge.canControl?(Vehicle.systemFallback?"无系统电池 · 模拟补能":Vehicle.externalMode?"外部车辆模型 · 55 kW / 60 kWh":"离线补能演示 · 55 kW / 60 kWh"):"Windows 实际电量 · 续航为车辆模型估算"; font.pixelSize: 13; color: Theme.muted }
}
