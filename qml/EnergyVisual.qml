pragma Singleton
import QtQuick
QtObject {
    readonly property bool realSystem: Vehicle.systemMode && !Vehicle.systemFallback
    readonly property real soc: Vehicle.batteryKnown?Math.max(0,Math.min(100,Vehicle.batteryPercent)):0
    readonly property string wholeSocValue: Vehicle.batteryKnown?Math.floor(soc).toString():"—"
    readonly property string wholeSocText: Vehicle.batteryKnown?wholeSocValue+"%":"—"
    readonly property string socText: !Vehicle.batteryKnown?"—":realSystem?wholeSocText:(soc<100?Math.min(soc,99.9):soc).toFixed(1)+"%"
    readonly property string rangeText: Vehicle.batteryKnown?Vehicle.rangeKm+" km":"未提供"
    function bandFor(value) { return value<=10?"critical":value<=20?"low":value<80?"normal":value<100?"high":"full" }
    readonly property string band: Vehicle.batteryKnown?bandFor(soc):"unknown"
    readonly property color levelColor: band==="unknown"?Theme.muted:band==="critical"?(Vehicle.night?"#f19b8b":"#b34b3b"):band==="low"?(Vehicle.night?"#ebbf79":"#a16d27"):band==="high"||band==="full"?(Vehicle.night?"#83d5cc":"#287e79"):Theme.green
    readonly property string levelText: band==="unknown"?"电量未知":band==="critical"?"电量极低":band==="low"?"低电量":band==="normal"?"常规电量":band==="high"?"高电量":"电池已满"
    readonly property color readingColor: Vehicle.energyReady?levelColor:Theme.muted
    readonly property string mode: !Vehicle.energyReady?"stale":realSystem?(Vehicle.charging?"charging":Vehicle.plugged?(soc>=100?"full":"connected"):"battery"):!Vehicle.plugged?"unplugged":Vehicle.charging?"charging":soc>=100?"full":soc>=Vehicle.chargeTarget?"target":"connected"
    readonly property bool flow: mode==="charging"
    readonly property color stateColor: mode==="charging"?(Vehicle.night?"#8ad9ba":"#237d61"):mode==="target"||mode==="full"?levelColor:mode==="stale"?(Vehicle.night?"#ebbf79":"#94723e"):Theme.muted
    readonly property color flowColor: "#52e5b4"
    readonly property string stateText: mode==="charging"?(realSystem?"系统正在充电":"正在充电"):mode==="connected"?(realSystem?"外接电源 · 未充电":"已连接 · 未充电"):mode==="battery"?"电池供电":mode==="target"?"已达到充电目标":mode==="full"?"电池已充满":mode==="stale"?(realSystem?"电源状态待确认":"车辆数据已过期"):"未连接充电枪"
    readonly property string detail: realSystem?(mode==="charging"?"系统未提供充电功率及预计时间":mode==="stale"?"等待系统电源状态刷新":mode==="battery"?"电量随电脑实际状态变化":"电池充电由系统管理"):mode==="charging"?Vehicle.chargePower.toFixed(0)+" kW · 约 "+Vehicle.chargeMinutes+" 分钟到 "+Vehicle.chargeTarget+"%":mode==="connected"?"能量流动已停止 · 可开始或继续充电":mode==="target"||mode==="full"?"目标 "+Vehicle.chargeTarget+"% · 已停止补能":mode==="stale"?"显示最后状态 · 等待连接恢复":"连接充电枪后开始补能"
}
