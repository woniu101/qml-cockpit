import QtQuick
import QtQuick.Controls as Controls
Item {
    id:root
    property bool follow:true
    Label {x:0;y:0;text:"歌词";font.pixelSize:22;color:Theme.muted}
    UiButton {objectName:"reloadLyrics";x:parent.width-120;y:-9;width:120;height:41;text:"刷新歌词";textSize:16;quiet:true;onClicked:{Music.reloadLyrics();root.follow=true}}
    ListView {
        id:list;objectName:"lyricsList";x:0;y:50;width:parent.width;height:parent.height-108;clip:true;spacing:17;model:Music.lyrics;visible:count>0
        boundsBehavior:Flickable.StopAtBounds
        header:Item {width:list.width;height:Math.max(0,list.height/2-30)}
        footer:Item {width:list.width;height:Math.max(0,list.height/2-30)}
        Controls.ScrollBar.vertical:Controls.ScrollBar{policy:Controls.ScrollBar.AsNeeded}
        onDraggingChanged:if(dragging)root.follow=false
        delegate:Item {
            required property var modelData
            required property int index
            width:list.width-12;height:Math.max(35,line.implicitHeight+12)
            opacity:y-list.contentY>=-1&&y-list.contentY+height<=list.height+1?1:0
            Label {id:line;x:10;y:6;width:parent.width-20;text:modelData.text||"♪";textFormat:Text.PlainText;font.pixelSize:index===Music.lyricIndex?27:22;color:index===Music.lyricIndex?Theme.green:Theme.muted;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}
            MouseArea {anchors.fill:parent;onClicked:{Music.seekLyric(index);root.follow=true}}
        }
    }
    Flickable {
        x:12;y:60;width:parent.width-24;height:parent.height-120;visible:Music.lyrics.length===0;contentHeight:plain.implicitHeight;clip:true
        Label {id:plain;width:parent.width;text:Music.lyricsText;textFormat:Text.PlainText;font.pixelSize:21;color:Theme.muted;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}
    }
    Row {x:0;y:parent.height-44;spacing:8
        UiButton {width:65;height:38;text:"−0.5s";textSize:14;enabled:Music.lyrics.length>0;onClicked:Music.lyricOffset-=500}
        Label {width:166;height:38;verticalAlignment:Text.AlignVCenter;horizontalAlignment:Text.AlignHCenter;text:"歌词延后 "+(Music.lyricOffset/1000).toFixed(1)+" s";font.pixelSize:15;color:Theme.muted}
        UiButton {width:65;height:38;text:"+0.5s";textSize:14;enabled:Music.lyrics.length>0;onClicked:Music.lyricOffset+=500}
        UiButton {width:85;height:38;text:"归零";textSize:15;enabled:Music.lyricOffset!==0;onClicked:Music.lyricOffset=0}
    }
    UiButton {x:parent.width-122;y:parent.height-44;width:122;height:38;text:"回到当前";textSize:15;visible:Music.lyrics.length>0;onClicked:{root.follow=true;root.centerCurrent()}}
    function centerCurrent(){if(follow&&Music.lyricIndex>=0)list.positionViewAtIndex(Music.lyricIndex,ListView.Center)}
    Connections {target:Music;function onLyricPositionChanged(){root.centerCurrent()}function onLyricsChanged(){root.follow=true;Qt.callLater(root.centerCurrent)}}
}
