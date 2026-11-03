import QtQuick
import QtQuick.Controls as Controls
Item {
    id: panel
    required property var shell
    function select(mode){if(shell.demoRunning)shell.toggleDemo();Vehicle.sourceMode=mode;shell.popup="source"}
    Label { x: 30; y: 27; text: "车辆数据来源"; font.pixelSize: 28 }
    UiButton { objectName: "sourceInternal"; x: 30; y: 95; width: 164; height: 54; text: "内置演示"; textSize: 18; selected: Vehicle.sourceMode==="internal"; onClicked: panel.select("internal") }
    UiButton { objectName: "sourceSystem"; x: 208; y: 95; width: 164; height: 54; text: "电脑映射"; textSize: 18; selected: Vehicle.systemMode; onClicked: panel.select("system") }
    UiButton { objectName: "sourceExternal"; x: 386; y: 95; width: 164; height: 54; text: "外部模拟器"; textSize: 18; selected: Vehicle.externalMode; onClicked: panel.select("external") }
    Label { objectName: "sourceStatus"; x: 30; y: 171; width: 520; text: Vehicle.externalMode?VehicleConnection.statusText:Vehicle.energySourceText; font.pixelSize: 20; color: Vehicle.energyReady?Theme.green:Theme.muted; wrapMode: Text.WordWrap }
    Item { x: 30; y: 226; width: 520; height: 292; visible: Vehicle.externalMode
        Label { text: "本机服务名称"; font.pixelSize: 16; color: Theme.muted }
        Controls.TextField { objectName: "vehicleServerInput"; y: 29; width: 395; height: 48; text: VehicleConnection.serverName; maximumLength: 120; color: Theme.ink; font.pixelSize: 16; selectByMouse: true
            background: Rectangle { radius: 12; color: Theme.secondary }
            onEditingFinished: VehicleConnection.serverName=text
        }
        UiButton { objectName: "sourceReconnect"; x: 408; y: 29; width: 112; height: 48; text: "重连"; textSize: 18; enabled: Vehicle.externalMode; onClicked: VehicleConnection.reconnect() }
        Label { y: 94; width: 520; text: "待处理 "+VehicleConnection.pendingCount+" 条 · "+(VehicleConnection.lastCommand||"暂无控制指令"); font.pixelSize: 16; color: Theme.green; maximumLineCount: 2; wrapMode: Text.WordWrap; elide: Text.ElideRight }
        Rectangle { y: 150; width: 520; height: 142; radius: 12; color: Theme.secondary
            Flickable { anchors.fill: parent; anchors.margins: 12; contentHeight: eventRows.height; clip: true
                Column { id: eventRows; width: parent.width; spacing: 9
                    Repeater { model: VehicleConnection.events; Label { required property string modelData; width: eventRows.width; text: modelData; font.pixelSize: 13; wrapMode: Text.WrapAnywhere; color: Theme.muted } }
                }
            }
        }
    }
    Column { x: 30; y: 232; width: 520; spacing: 22; visible: !Vehicle.externalMode
        Label { width: 520; text: Vehicle.systemMode?Host.power.statusText:"单程序运行，车辆和能量使用内置模型。"; font.pixelSize: 21; color: Theme.green; wrapMode: Text.WordWrap }
        Label { width: 520; text: Vehicle.systemMode?(Vehicle.systemFallback?"当前无系统电池，车辆电量明确使用模拟值，模拟补能可操作。":"车辆 SOC 跟随电脑实际电量；供电与充电状态由系统决定。模拟启停、充电目标和充电快进不可用。电量读取失败时保留最后数据并停止相关动效。") :"门窗、行驶、充电可离线演示；网络信息继续读取当前系统。"; font.pixelSize: 17; color: Theme.muted; wrapMode: Text.WordWrap }
        Label { width: 520; text: "网络："+Host.network.summary; font.pixelSize: 17; color: Theme.muted; wrapMode: Text.WordWrap }
        Label { width: 520; text: "切换前请驻车；切回内置演示恢复本地车辆数据。本地音乐、空调和显示偏好保留。"; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
    }
    UiButton { objectName: "sourceSystemDetails"; x: 30; y: 532; width: 520; height: 44; text: "电脑电源与网络详情"; textSize: 18; onClicked: panel.shell.openPopup("system") }
    Label { x: 30; y: 586; width: 520; text: Vehicle.externalMode?"外部接管能量、行驶与车身；网络来自本机系统。":"功率与预计时间仅在数据来源提供时显示。"; font.pixelSize: 13; color: Theme.muted }
}
