import QtQuick
import QtQuick.Controls as Controls
Item {
    required property var shell
    Label { x: 30; y: 27; text: "电脑电源与网络"; font.pixelSize: 28 }
    Flickable {
        x: 30; y: 93; width: 520; height: 425; clip: true; contentHeight: content.height; boundsBehavior: Flickable.StopAtBounds
        Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }
        Column { id: content; width: 520; spacing: 16
            Label { text: "系统电源"; font.pixelSize: 22 }
            Label { objectName: "systemPowerState"; width: 510; text: Host.power.statusText; color: Theme.green; font.pixelSize: 20; wrapMode: Text.WordWrap }
            Label { objectName: "systemPowerPercent"; text: "系统电量："+(Host.power.socKnown?Host.power.percent+"%":"未提供"); font.pixelSize: 18 }
            Label { text: "供电："+(Host.power.acKnown?(Host.power.onAc?"外接电源":"电池供电"):"未知")+"    节电："+(Host.power.saverKnown?(Host.power.saver?"开启":"关闭"):"未知"); font.pixelSize: 16; color: Theme.muted }
            Label { width: 510; text: "车辆电量来源："+Vehicle.energySourceText; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
            UiButton { objectName: "systemUsePower"; width: 510; height: 46; text: "使用电脑映射"; textSize: 18; visible: !Vehicle.systemMode; onClicked: Vehicle.sourceMode="system" }
            Rectangle { width: 510; height: 1; color: Theme.line }
            Label { text: "网络连接"; font.pixelSize: 22 }
            Label { objectName: "systemNetworkState"; width: 510; text: Host.network.summary; color: Theme.green; font.pixelSize: 20; wrapMode: Text.WordWrap }
            Label { width: 510; text: Host.network.meteredKnown?(Host.network.metered?"按流量计费网络":"非按流量计费网络"):"流量计费状态未提供"; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap }
            Repeater { model: Host.network.interfaces; Label { required property string modelData; width: 510; text: modelData; font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap } }
            Label { width: 510; text: Host.network.transport==="wifi"?"Wi-Fi 信号强度：未提供。":"联网状态由系统报告，实际业务连接以请求结果为准。"; font.pixelSize: 14; color: Theme.muted; wrapMode: Text.WordWrap }
        }
    }
    Label { x: 30; y: 536; width: 370; text: Host.sampling?"更新于 "+Host.power.updated+" / "+Host.network.updated:"测试样本，不代表当前电脑"; font.pixelSize: 14; color: Theme.muted; wrapMode: Text.WordWrap }
    UiButton { objectName: "systemRefresh"; x: 424; y: 528; width: 126; height: 44; text: "刷新"; textSize: 18; onClicked: Host.refresh() }
    Label { x: 30; y: 582; width: 520; text: "电脑映射只读取状态；模拟充电请切换数据来源。"; font.pixelSize: 13; color: Theme.muted }
}
