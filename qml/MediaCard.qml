import QtQuick
Item {
    id:card
    required property var shell
    property bool compact:false
    readonly property bool music:Media.kind==="music"
    readonly property bool radio:Media.kind==="radio"
    readonly property bool video:Media.kind==="video"
    readonly property bool phone:PhoneAudio.active
    readonly property bool localMusic:music&&!phone
    readonly property bool fullMusic:!compact&&localMusic
    readonly property int artworkSize:compact?77:fullMusic?88:99
    width:compact?398:438;height:compact?152:235
    Surface {anchors.fill:parent}
    TrackArtwork {x:28;y:card.fullMusic?24:28;width:card.artworkSize;height:width;visible:card.localMusic;source:Music.current.cover||""}
    Rectangle {x:28;y:28;width:card.artworkSize;height:width;radius:20;color:Theme.active;visible:!card.localMusic
        Icon {anchors.centerIn:parent;width:compact?39:49;height:width;name:card.phone?"bluetooth":card.radio?"radio":"video";tint:Theme.green}
    }
    Label {x:compact?123:card.artworkSize+50;y:compact?24:card.fullMusic?26:31;width:compact?145:card.width-x-28;text:card.phone?"手机音乐":Media.title;textFormat:Text.PlainText;elide:Text.ElideRight;font.pixelSize:compact?24:27;font.weight:Font.Medium}
    Label {x:compact?124:card.artworkSize+51;y:compact?62:card.fullMusic?65:76;width:compact?145:card.width-x-28;text:card.phone?PhoneAudio.deviceName:card.radio?"网络直播":card.video?(Media.phase==="blocked"?"驻车后继续观看":"本地视频"):(Music.current.artist||Music.current.source||"本地音乐");textFormat:Text.PlainText;elide:Text.ElideRight;font.pixelSize:18;color:Theme.muted}
    MouseArea {x:24;y:20;width:compact?250:350;height:compact?87:card.fullMusic?96:105;onClicked:card.shell.showMedia(card.phone?"phone":Media.kind)}
    UiButton {x:278;y:23;width:48;height:44;visible:card.compact&&card.localMusic;quiet:true;enabled:AudioOutputs.available;iconName:MediaState.playing?"pause":"play";onClicked:MediaState.playing=!MediaState.playing}
    UiButton {objectName:"compactNext";x:327;y:23;width:48;height:44;visible:card.compact&&card.localMusic;quiet:true;iconName:"skip";onClicked:MediaState.nextTrack()}
    SoftSlider {
        id: progress
        x:compact?113:18; y:compact?96:117; width:compact?263:card.width-36; height:40
        visible:card.localMusic; from:0; to:Math.max(1,MediaState.duration)
        value:MediaState.position; onMoved:MediaState.seek(value)
    }
    Label {
        visible:card.fullMusic
        x:progress.x+progress.leftPadding; y:151
        text:card.shell.formatTime(MediaState.position); font.pixelSize:15; color:Theme.muted
    }
    Label {
        visible:card.fullMusic
        anchors.right:parent.right; anchors.rightMargin:card.width-progress.x-progress.width+progress.rightPadding
        y:151; text:card.shell.formatTime(MediaState.duration); font.pixelSize:15; color:Theme.muted
    }
    Row {
        visible:card.fullMusic
        anchors.horizontalCenter:parent.horizontalCenter
        y:174; spacing:22
        UiButton {
            width:48; height:48; quiet:true
            iconName:"previous"; caption:"上一曲"
            onClicked:MediaState.nextTrack(-1)
        }
        UiButton {
            id: playButton
            objectName:"musicPlay"
            width:48; height:48; selected:true
            enabled:AudioOutputs.available
            iconName:MediaState.playing?"pause":"play"
            caption:MediaState.playing?"暂停":"播放"
            background:Rectangle {
                radius:width/2
                color:playButton.down?Theme.secondary:Theme.active
                border.width:playButton.visualFocus?1:0; border.color:Theme.green
            }
            onClicked:MediaState.playing=!MediaState.playing
        }
        UiButton {
            width:48; height:48; quiet:true
            iconName:"skip"; caption:"下一曲"
            onClicked:MediaState.nextTrack()
        }
    }
    UiButton {objectName:compact?"compactPhoneAudio":"homePhoneAudio";x:compact?113:28;y:compact?99:143;width:compact?263:382;height:compact?37:64;visible:card.phone;text:PhoneAudio.phase==="connected"?"已接通 · 在手机播放":PhoneAudio.text;textSize:17;selected:true;onClicked:card.shell.openPopup("bluetooth")}
    UiButton {objectName:compact?"compactRadio":"homeRadio";x:compact?113:28;y:compact?99:143;width:compact?263:382;height:compact?37:64;visible:card.radio&&!card.phone;text:Media.busy?"连接中 · 点击取消":Media.playing?"停止收听":"继续收听";iconName:Media.busy?"close":Media.playing?"stop":"play";textSize:17;selected:true;onClicked:Radio.toggle()}
    UiButton {objectName:compact?"compactVideo":"homeVideo";x:compact?113:28;y:compact?99:143;width:compact?263:382;height:compact?37:64;visible:card.video&&!card.phone;text:Media.videoAllowed?"返回视频":"已暂停 · 驻车后继续";iconName:"video";textSize:17;selected:true;onClicked:card.shell.showMedia("video")}
}
