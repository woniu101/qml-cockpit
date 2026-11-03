import QtQuick
Item {
    required property var shell
    property bool compact: false
    width: compact ? 398 : 438; height: compact ? 152 : 235
    Surface { anchors.fill: parent }
    TrackArtwork { x: 28; y: 28; width: compact ? 77 : 99; height: width; visible:!PhoneAudio.active;source:Music.current.cover||"" }
    Icon { x:43;y:43;width:compact?47:69;height:width;name:"bluetooth";tint:Theme.green;visible:PhoneAudio.active }
    Label { x: compact ? 123 : 149; y: compact ? 24 : 31; width:compact?145:261;text: PhoneAudio.active?"手机音乐":Vehicle.trackTitle;textFormat:Text.PlainText;elide:Text.ElideRight;font.pixelSize: compact ? 24 : 27; font.weight: Font.Medium }
    Label { x: compact ? 124 : 150; y: compact ? 62 : 76; width:compact?145:265; text: PhoneAudio.active?PhoneAudio.deviceName:(Music.current.artist||Music.current.source||"本地音乐");textFormat:Text.PlainText;elide:Text.ElideRight; font.pixelSize: 18; color: Theme.muted }
    MouseArea { x: 24; y: 20; width: compact ? 250 : 350; height: compact ? 87 : 105; onClicked: shell.showPage(3) }
    UiButton { x: compact ? 278 : 155; y: compact ? 23 : 177; width: 48; height: 44;visible:!PhoneAudio.active; quiet: true; enabled:!compact||AudioOutputs.available; iconName: compact ? (Vehicle.playing ? "pause" : "play") : "previous"; onClicked: compact ? Vehicle.playing = !Vehicle.playing : Vehicle.nextTrack(-1) }
    UiButton { objectName: compact ? "compactNext" : "musicPlay"; x: compact ? 327 : 225; y: compact ? 23 : 177; width: 48; height: 44;visible:!PhoneAudio.active; selected: !compact; quiet: true; enabled:compact||AudioOutputs.available; iconName: compact ? "skip" : (Vehicle.playing ? "pause" : "play"); onClicked: compact ? Vehicle.nextTrack() : Vehicle.playing = !Vehicle.playing }
    UiButton { visible: !compact&&!PhoneAudio.active; x: 305; y: 177; width: 48; height: 44; quiet: true; iconName: "skip"; onClicked: Vehicle.nextTrack() }
    SoftSlider { x: compact ? 113 : 18; y: compact ? 96 : 135; width: compact ? 263 : 400; height: 40;visible:!PhoneAudio.active; from: 0; to: Math.max(1, Vehicle.duration); value: Vehicle.position; onMoved: Vehicle.seek(value) }
    Label { visible: !compact&&!PhoneAudio.active; x: 28; y: 170; text: shell.formatTime(Vehicle.position); font.pixelSize: 15; color: Theme.muted }
    Label { visible: !compact&&!PhoneAudio.active; x: 344; y: 170; width: 65; horizontalAlignment: Text.AlignRight; text: shell.formatTime(Vehicle.duration); font.pixelSize: 15; color: Theme.muted }
    UiButton {objectName:compact?"compactPhoneAudio":"homePhoneAudio";x:compact?113:28;y:compact?99:143;width:compact?263:382;height:compact?37:64;visible:PhoneAudio.active;text:PhoneAudio.phase==="connected"?"已接通 · 在手机播放":PhoneAudio.text;textSize:17;selected:true;onClicked:shell.openPopup("bluetooth")}
}
