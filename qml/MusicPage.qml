import QtQuick
import QtQuick.Controls
import "." as App
import QtQuick.Dialogs
Item {
    id:page
    required property var shell
    width:1600;height:790
    FileDialog {id:importDialog;title:"加入本地音频";fileMode:FileDialog.OpenFiles;nameFilters:["音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg)","所有文件 (*)"];onAccepted:Vehicle.importTracks(selectedFiles)}
    App.Label {x:56;y:95;text:"音乐";font.pixelSize:34;font.weight:Font.Medium}
    App.Label {x:56;y:151;text:"让旋律陪你走过每一程";font.pixelSize:20;color:Theme.muted}
    Album{x:120;y:242;width:365;height:365}
    App.Label {x:550;y:266;width:470;text:Vehicle.trackTitle;font.pixelSize:42;font.weight:Font.Medium;elide:Text.ElideRight}
    App.Label {x:551;y:335;text:Vehicle.mediaStatus==="loading"?"音源加载中…":Vehicle.mediaStatus==="error"?"音源加载失败":"本地音乐 · "+(Vehicle.playing?"正在播放":"已暂停");font.pixelSize:21;color:Theme.muted}
    SoftSlider{objectName:"seekSlider";x:541;y:416;width:477;from:0;to:Math.max(1,Vehicle.duration);value:Vehicle.position;enabled:Vehicle.duration>0;onMoved:Vehicle.seek(value)}
    App.Label {x:551;y:467;text:page.shell.formatTime(Vehicle.position);font.pixelSize:17;color:Theme.muted}
    App.Label {x:920;y:467;width:90;text:page.shell.formatTime(Vehicle.duration);font.pixelSize:17;color:Theme.muted;horizontalAlignment:Text.AlignRight}
    UiButton{objectName:"fullMusicPrevious";x:600;y:546;width:75;height:75;quiet:true;iconName:"previous";onClicked:Vehicle.nextTrack(-1)}
    UiButton{objectName:"fullMusicPlay";x:731;y:536;width:95;height:95;selected:true;iconName:Vehicle.playing?"pause":"play";onClicked:Vehicle.playing=!Vehicle.playing}
    UiButton{objectName:"fullMusicNext";x:882;y:546;width:75;height:75;quiet:true;iconName:"skip";onClicked:Vehicle.nextTrack()}
    UiButton{objectName:"playMode";x:575;y:666;width:410;height:47;text:Vehicle.playMode==="all"?"列表循环":Vehicle.playMode==="one"?"单曲循环":"随机播放";iconName:"refresh";textSize:18;quiet:true;onClicked:Vehicle.playMode=Vehicle.playMode==="all"?"one":Vehicle.playMode==="one"?"shuffle":"all"}
    Surface {
        x:1090;y:165;width:455;height:580
        App.Label {x:29;y:26;text:"播放列表";font.pixelSize:26}
        UiButton{objectName:"importMusic";x:278;y:22;width:148;height:45;text:"导入本地";iconName:"plus";textSize:17;quiet:true;onClicked:importDialog.open()}
        Flickable {
            x:25;y:94;width:405;height:390;clip:true;contentHeight:songs.height;boundsBehavior:Flickable.StopAtBounds
            ScrollBar.vertical:ScrollBar{policy:ScrollBar.AsNeeded}
            Column {
                id:songs;width:405;spacing:13
                Repeater {
                    model:Vehicle.tracks
                    UiButton {
                        required property var modelData;required property int index
                        objectName:"trackRow"+index;width:405;height:92;selected:index===Vehicle.track
                        contentItem:Item {
                            Album{x:8;y:4;width:64;height:64}
                            App.Label {x:94;y:4;width:224;text:modelData.title;font.pixelSize:23;elide:Text.ElideRight}
                            App.Label {x:94;y:41;width:224;text:modelData.source;font.pixelSize:16;color:Theme.muted;elide:Text.ElideRight}
                            Icon{x:345;y:22;width:24;height:24;name:index===Vehicle.track&&Vehicle.playing?"pause":"play";tint:Theme.green}
                        }
                        onClicked:Vehicle.selectTrack(index)
                    }
                }
            }
        }
        App.Label {x:29;y:515;text:Vehicle.tracks.length+" 首本地音源 · 当前曲目点击可暂停";font.pixelSize:16;color:Theme.muted}
    }
    Surface {
        visible:Vehicle.mediaStatus==="error";x:540;y:366;width:480;height:126
        App.Label {x:18;y:12;width:444;text:Vehicle.mediaError;font.pixelSize:16;maximumLineCount:2;wrapMode:Text.WordWrap;elide:Text.ElideRight;color:Theme.muted}
        UiButton{objectName:"retryMusic";x:18;y:70;width:200;height:41;text:"重试当前音源";textSize:17;onClicked:Vehicle.retryTrack()}
        UiButton{x:237;y:70;width:224;height:41;text:"选择其他本地音源";textSize:17;onClicked:importDialog.open()}
    }
}
