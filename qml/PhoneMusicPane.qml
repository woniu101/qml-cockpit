import QtQuick
Item {
    id:page
    required property var shell
    width:1600;height:790
        Surface {x:56;y:205;width:965;height:557
            Icon {x:55;y:120;width:155;height:155;name:"bluetooth";tint:Theme.green}
            Label {x:260;y:92;width:655;text:"手机蓝牙音乐";font.pixelSize:36}
            Label {x:262;y:153;width:640;text:PhoneAudio.deviceName||"选择已配对的手机开始接收";textFormat:Text.PlainText;font.pixelSize:22;color:Theme.muted;elide:Text.ElideRight}
            Label {x:262;y:220;width:640;text:PhoneAudio.text;font.pixelSize:24;color:PhoneAudio.phase==="connected"?Theme.green:Theme.ink}
            Label {x:262;y:270;width:620;text:"此连接未提供歌名、进度或封面。\n播放、暂停、切歌与音量请在手机控制。";font.pixelSize:20;color:Theme.muted;wrapMode:Text.WordWrap}
            UiButton {objectName:"musicPhoneDetails";x:262;y:377;width:370;height:62;text:PhoneAudio.active?"查看手机音乐连接":"选择手机并连接";iconName:"bluetooth";selected:true;onClicked:page.shell.openPopup("bluetooth")}
            Label {x:55;y:505;width:850;text:"接通表示音频通道已建立；是否正在播放以手机为准。";font.pixelSize:16;color:Theme.muted}
        }
        Surface {x:1060;y:205;width:485;height:557
            Label {x:28;y:27;text:"手机连接";font.pixelSize:26}
            Label {x:28;y:97;width:429;text:BluetoothState.status.text;font.pixelSize:20;color:Theme.green;wrapMode:Text.WordWrap}
            Label {x:28;y:165;width:429;text:"接收声音输出至 Windows 默认设备。\n本地音乐库和播放队列保留在“音乐”页。";font.pixelSize:20;color:Theme.muted;wrapMode:Text.WordWrap}
            UiButton {objectName:"phoneManageConnection";x:28;y:280;width:429;height:58;text:"蓝牙设备与音乐连接";iconName:"bluetooth";textSize:20;onClicked:page.shell.openPopup("bluetooth")}
            UiButton {x:28;y:363;width:429;height:58;text:"系统声音设置";iconName:"volume";textSize:20;onClicked:AudioOutputs.openSettings()}
            Label {x:28;y:471;width:429;text:"切换到“音乐”页会断开手机接收，\n本地音乐等待手动播放。";font.pixelSize:16;color:Theme.muted;wrapMode:Text.WordWrap}
        }
    }
