import QtQuick
import QtQuick.Controls as Controls
Item {
    id: panel
    required property var shell
    Component.onCompleted: { BluetoothState.refresh(); PhoneAudio.refresh() }
    Label { x:30; y:27; text:"蓝牙与手机音乐"; font.pixelSize:28 }
    Label { objectName:"bluetoothStateText"; x:30; y:88; width:390; text:BluetoothState.status.text; textFormat:Text.PlainText; font.pixelSize:19; color:BluetoothState.status.state==="on"?Theme.green:Theme.muted; elide:Text.ElideRight }
    UiButton { objectName:"bluetoothRefresh"; x:440; y:77; width:110; height:44; text:BluetoothState.busy||PhoneAudio.scanning?"读取中…":"刷新"; textSize:17; enabled:!BluetoothState.busy&&!PhoneAudio.scanning&&PhoneAudio.phase!=="connecting"&&PhoneAudio.phase!=="disconnecting"; onClicked:{BluetoothState.refresh();PhoneAudio.refresh()} }
    Flickable {
        x:30; y:132; width:520; height:374; clip:true; contentHeight:rows.height; boundsBehavior:Flickable.StopAtBounds
        Controls.ScrollBar.vertical:Controls.ScrollBar { policy:Controls.ScrollBar.AsNeeded }
        Column {
            id:rows; width:510; spacing:12
            Rectangle {
                width:510; height:PhoneAudio.active?140:111; radius:16; color:Theme.secondary
                Icon { x:16; y:18; name:"bluetooth"; width:25; height:25; tint:Theme.green }
                Label { objectName:"phoneAudioState"; x:53; y:16; width:435; text:PhoneAudio.text; font.pixelSize:21; color:PhoneAudio.phase==="connected"?Theme.green:Theme.ink }
                Label { x:16; y:55; width:478; text:PhoneAudio.phase==="connected"?"请在手机播放音乐；播放与音量在手机控制。":PhoneAudio.phase==="connecting"?"请保持手机蓝牙开启，可随时取消连接。":PhoneAudio.phase==="disconnecting"?"正在释放本应用的接收连接…":"连接后通过 Windows 默认扬声器播放手机声音。"; font.pixelSize:15; color:Theme.muted; wrapMode:Text.WordWrap }
                Label { x:16; y:82; width:300; visible:PhoneAudio.active; text:PhoneAudio.deviceName; textFormat:Text.PlainText; font.pixelSize:15; color:Theme.muted; elide:Text.ElideRight }
                UiButton { objectName:"phoneAudioDisconnect"; x:326; y:87; width:168; height:41; visible:PhoneAudio.active; enabled:PhoneAudio.phase!=="disconnecting"; text:PhoneAudio.phase==="connecting"?"取消连接":"断开音乐"; textSize:17; onClicked:PhoneAudio.disconnectDevice() }
            }
            Label { width:510; visible:PhoneAudio.error!==""; text:PhoneAudio.error; textFormat:Text.PlainText; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { width:510; visible:PhoneAudio.scanError!==""; text:PhoneAudio.scanError; textFormat:Text.PlainText; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { width:510; visible:!PhoneAudio.outputAvailable; text:"没有可用扬声器或耳机，连接音频输出后再接收音乐。"; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { text:"可接收音乐的设备"; font.pixelSize:19 }
            Repeater {
                model:PhoneAudio.devices
                Rectangle {
                    required property var modelData
                    required property int index
                    width:510; height:77; radius:15; color:Theme.secondary
                    Label { x:16; y:12; width:302; text:modelData.name; textFormat:Text.PlainText; font.pixelSize:19; elide:Text.ElideRight }
                    Label { x:16; y:44; text:"手机音频 · Windows 接收"; font.pixelSize:14; color:Theme.muted }
                    UiButton { objectName:"phoneAudioConnect"+index; x:326; y:15; width:168; height:47; selected:true; text:"连接音乐"; textSize:18; enabled:PhoneAudio.supported&&!PhoneAudio.active&&!PhoneAudio.scanning&&PhoneAudio.scanError===""&&PhoneAudio.outputAvailable&&BluetoothState.status.state==="on"; onClicked:PhoneAudio.connectDevice(modelData.token) }
                }
            }
            Label { width:510; visible:PhoneAudio.devices.length===0&&!PhoneAudio.scanning&&PhoneAudio.scanError===""; text:"暂无音频设备。请先在 Windows 中配对手机，再刷新；已配对的键盘、鼠标不属于音乐设备。"; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { text:"已配对设备"; visible:BluetoothState.status.state==="on"; font.pixelSize:19 }
            Repeater {
                model:BluetoothState.status.devices
                Rectangle {
                    required property var modelData
                    width:510; height:68; radius:15; color:Theme.secondary
                    Label { x:16; y:9; width:478; text:modelData.name; textFormat:Text.PlainText; font.pixelSize:18; elide:Text.ElideRight }
                    Label { x:16; y:39; width:478; text:modelData.kind+" · 已配对 · "+(!modelData.connectionKnown?"连接状态未知":modelData.connected?"设备已连接":"设备未连接"); font.pixelSize:14; color:modelData.connected?Theme.green:Theme.muted; elide:Text.ElideRight }
                }
            }
            Label { width:510; visible:BluetoothState.status.state==="on"&&BluetoothState.status.devices.length===0&&BluetoothState.status.deviceError===""; text:"暂无已配对设备，可在 Windows 设置中完成配对。"; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { width:510; visible:BluetoothState.status.state==="absent"; text:"未检测到适配器，请连接蓝牙适配器后刷新。"; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { width:510; visible:BluetoothState.status.error!==""; text:BluetoothState.status.error; textFormat:Text.PlainText; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
            Label { width:510; visible:BluetoothState.status.deviceError!==""; text:BluetoothState.status.deviceError; font.pixelSize:16; color:Theme.muted; wrapMode:Text.WordWrap }
        }
    }
    Label { x:30; y:518; width:520; text:"接收时暂停本地音乐；退出应用会断开，不自动重连。"; font.pixelSize:14; color:Theme.muted }
    UiButton { objectName:"bluetoothSettings"; x:30; y:552; width:245; height:42; text:"Windows 蓝牙设置"; textSize:17; onClicked:BluetoothState.openSettings() }
    UiButton { objectName:"bluetoothAudioOutputs"; x:290; y:552; width:260; height:42; text:"系统声音设置"; textSize:17; onClicked:AudioOutputs.openSettings() }
}
