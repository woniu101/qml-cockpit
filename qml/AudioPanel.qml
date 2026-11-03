import QtQuick
import QtQuick.Controls as Controls
Item {
    Label { x:30; y:27; text:"本地音乐输出"; font.pixelSize:28 }
    Label { objectName:"audioRouteStatus"; x:30; y:91; width:395; height:54; text:AudioOutputs.statusText; textFormat:Text.PlainText; wrapMode:Text.WordWrap; maximumLineCount:2; elide:Text.ElideRight; font.pixelSize:18; color:AudioOutputs.available?Theme.green:Theme.muted }
    UiButton { objectName:"audioRefresh"; x:444; y:91; width:106; height:45; text:"刷新"; textSize:17; onClicked:AudioOutputs.refresh() }
    Flickable {
        x:30; y:157; width:520; height:334; contentHeight:rows.height; clip:true; boundsBehavior:Flickable.StopAtBounds
        Controls.ScrollBar.vertical:Controls.ScrollBar { policy:Controls.ScrollBar.AsNeeded }
        Column {
            id:rows; width:510; spacing:12
            UiButton {
                objectName:"audioFollowDefault"; width:510; height:72; selected:AudioOutputs.followDefault; caption:"跟随系统默认音频输出"
                contentItem:Item {
                    Label { x:6; y:0; text:"跟随系统默认"; font.pixelSize:20; color:Theme.ink }
                    Label { x:6; y:33; width:435; text:AudioOutputs.defaultName; textFormat:Text.PlainText; font.pixelSize:15; color:Theme.muted; elide:Text.ElideRight }
                    Icon { x:461; y:13; name:"check"; width:23; height:23; tint:Theme.green; visible:AudioOutputs.followDefault }
                }
                onClicked:AudioOutputs.selectOutput("")
            }
            Repeater {
                model:AudioOutputs.outputs
                UiButton {
                    required property var modelData
                    required property int index
                    objectName:"audioOutput"+index; width:510; height:72; selected:modelData.selected; caption:modelData.name
                    contentItem:Item {
                        Label { x:6; y:0; width:435; text:modelData.name; textFormat:Text.PlainText; font.pixelSize:19; elide:Text.ElideRight }
                        Label { x:6; y:33; text:modelData.isDefault?"系统默认设备":"仅用于本地音乐"; font.pixelSize:15; color:Theme.muted }
                        Icon { x:461; y:13; name:"check"; width:23; height:23; tint:Theme.green; visible:modelData.selected }
                    }
                    onClicked:AudioOutputs.selectOutput(modelData.id)
                }
            }
        }
    }
    Label { x:30; y:508; text:PhoneAudio.active?"手机音量在手机调节；以下为本地音乐音量":"本地音乐音量 "+Vehicle.volume+"%"; font.pixelSize:17; color:Theme.muted }
    SoftSlider { objectName:"audioLocalVolume"; x:20; y:538; width:330; height:44; from:0; to:100; value:Vehicle.volume; enabled:AudioOutputs.available&&!PhoneAudio.active; onMoved:Vehicle.volume=Math.round(value) }
    UiButton { objectName:"audioSystemSettings"; x:370; y:538; width:180; height:44; text:"系统声音设置"; textSize:17; onClicked:AudioOutputs.openSettings() }
    Label { x:30; y:587; width:520; text:AudioOutputs.sampling?"选择仅作用于本地音乐，保留 Windows 默认输出和系统音量。":"测试设备列表；音量已静音，不写入偏好。"; font.pixelSize:12; color:Theme.muted }
}
