import QtQuick
Item {
    required property var shell
    property bool compact: false
    width: compact ? 398 : 438; height: compact ? 152 : 235
    Surface { anchors.fill: parent }
    Album { x: 28; y: 28; width: compact ? 77 : 99; height: width }
    Label { x: compact ? 123 : 149; y: compact ? 24 : 31; text: Vehicle.trackTitle; font.pixelSize: compact ? 24 : 27; font.weight: Font.Medium }
    Label { x: compact ? 124 : 150; y: compact ? 62 : 76; text: "轻音乐精选"; font.pixelSize: 18; color: Theme.muted }
    MouseArea { x: 24; y: 20; width: compact ? 250 : 350; height: compact ? 87 : 105; onClicked: shell.showPage(3) }
    UiButton { x: compact ? 278 : 155; y: compact ? 23 : 177; width: 48; height: 44; quiet: true; iconName: compact ? (Vehicle.playing ? "pause" : "play") : "previous"; onClicked: compact ? Vehicle.playing = !Vehicle.playing : Vehicle.nextTrack(-1) }
    UiButton { objectName: compact ? "compactNext" : "musicPlay"; x: compact ? 327 : 225; y: compact ? 23 : 177; width: 48; height: 44; selected: !compact; quiet: true; iconName: compact ? "skip" : (Vehicle.playing ? "pause" : "play"); onClicked: compact ? Vehicle.nextTrack() : Vehicle.playing = !Vehicle.playing }
    UiButton { visible: !compact; x: 305; y: 177; width: 48; height: 44; quiet: true; iconName: "skip"; onClicked: Vehicle.nextTrack() }
    SoftSlider { x: compact ? 113 : 18; y: compact ? 96 : 135; width: compact ? 263 : 400; height: 40; from: 0; to: Math.max(1, Vehicle.duration); value: Vehicle.position; onMoved: Vehicle.seek(value) }
    Label { visible: !compact; x: 28; y: 170; text: shell.formatTime(Vehicle.position); font.pixelSize: 15; color: Theme.muted }
    Label { visible: !compact; x: 344; y: 170; width: 65; horizontalAlignment: Text.AlignRight; text: shell.formatTime(Vehicle.duration); font.pixelSize: 15; color: Theme.muted }
}
