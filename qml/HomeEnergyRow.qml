import QtQuick
import QtQuick.Layouts
Item {
    id: summary
    required property var shell
    readonly property string chargeMode: EnergyVisual.mode
    readonly property bool hasEnergyState: EnergyVisual.realSystem || Vehicle.plugged || Vehicle.portOpen || !Vehicle.energyReady
    readonly property var bodyAlerts: Vehicle.notifications.filter(entry=>entry.part!=="port" && entry.part!=="lock" && entry.part!=="mirrors")
    readonly property bool hasWarning: bodyAlerts.some(entry=>entry.severity==="warning")
    readonly property color stateColor: hasEnergyState?EnergyVisual.stateColor:hasWarning?(Vehicle.night?"#ebbf79":"#a16d27"):Theme.muted
    readonly property string stateText: hasEnergyState?(chargeMode==="target"?"已达到目标 "+Vehicle.chargeTarget+"%":chargeMode==="unplugged"?"充电口已打开":EnergyVisual.stateText):bodyAlerts.length?bodyAlerts[0].title:""
    readonly property string detailText: EnergyVisual.realSystem?EnergyVisual.detail:!hasEnergyState?(bodyAlerts.length?bodyAlerts[0].detail:""):chargeMode==="charging"?"约 "+Vehicle.chargeMinutes+" 分钟至目标 "+Vehicle.chargeTarget+"%":chargeMode==="connected"?"可在补能页开始或继续充电":chargeMode==="target"?"补能已停止，可断开充电枪":chargeMode==="full"?"充电已完成，可断开充电枪":chargeMode==="stale"?"等待连接恢复，充电状态待确认":"连接充电枪后开始补能"
    visible: hasEnergyState || bodyAlerts.length>0
    height: 44
    RowLayout {
        anchors.fill: parent; spacing: 14
        Icon { Layout.preferredWidth: 22; Layout.preferredHeight: 22; name: summary.hasEnergyState?(summary.chargeMode==="full"||summary.chargeMode==="target"?"check":"charge"):(summary.bodyAlerts.length?summary.bodyAlerts[0].icon:"car"); tint: summary.stateColor }
        Label { objectName: "homeEnergyState"; text: summary.stateText; font.pixelSize: 20; font.weight: Font.Normal; color: summary.stateColor; Layout.maximumWidth: 225; elide: Text.ElideRight }
        Label { objectName: "homeChargePower"; visible: summary.hasEnergyState && EnergyVisual.flow && Vehicle.chargePowerKnown; text: Vehicle.chargePower.toFixed(0)+" kW"; font.pixelSize: 20; font.weight: Font.Normal; color: Theme.ink }
        Label { text: "·"; font.pixelSize: 17; color: Theme.muted }
        Label { objectName: "homeEnergyDetail"; text: summary.detailText; font.pixelSize: 16; color: Theme.muted; Layout.fillWidth: true; elide: Text.ElideRight }
        UiButton { objectName: "homeBodyAlerts"; visible: summary.hasEnergyState && summary.bodyAlerts.length>0; Layout.preferredWidth: 100; Layout.preferredHeight: 44; quiet: true; text: summary.bodyAlerts.length+" 项提醒"; textSize: 16; foreground: summary.hasWarning?(Vehicle.night?"#ebbf79":"#a16d27"):Theme.muted; onClicked: summary.shell.openPopup("notifications") }
        UiButton { objectName: "homeViewCharging"; Layout.preferredWidth: 126; Layout.preferredHeight: 44; quiet: true; text: summary.hasEnergyState?"查看补能 ›":"查看提醒 ›"; textSize: 16; foreground: Theme.green; onClicked: {if(summary.hasEnergyState)summary.shell.inspectPart("port");else summary.shell.openPopup("notifications")} }
    }
}
