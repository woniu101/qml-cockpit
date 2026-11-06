import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
Item {
    id:page
    required property var shell
    width:1600;height:790
    property int relocationIndex:-1
    FileDialog {id:importDialog;title:"加入本地音频";fileMode:FileDialog.OpenFiles;nameFilters:["音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg *.aac *.opus *.wma)"];onAccepted:Music.importFiles(selectedFiles)}
    FolderDialog {id:folderDialog;title:"添加音乐目录（包含子文件夹）";onAccepted:Music.importFolder(selectedFolder)}
    FileDialog {id:relocationDialog;title:"重新定位音乐文件";fileMode:FileDialog.OpenFile;nameFilters:importDialog.nameFilters;onAccepted:Music.relocate(page.relocationIndex,selectedFile)}
    DropArea {anchors.fill:parent;onDropped:drop=>{if(drop.hasUrls){Music.importFiles(drop.urls);drop.acceptProposedAction()}}}
    Label {objectName:"musicOutputSummary";x:56;y:156;width:930;text:Music.scanText||AudioOutputs.statusText;textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight}
    UiButton {objectName:"musicQueueButton";x:1380;y:151;width:165;height:40;text:"播放队列 · "+Music.queueCount;textSize:19;onClicked:page.shell.openPopup("musicQueue")}
    UiButton {objectName:"importFolder";x:1060;y:151;width:145;height:40;text:"添加目录";iconName:"plus";textSize:19;enabled:!Music.scanning;onClicked:folderDialog.open()}
    UiButton {objectName:"importMusic";x:1220;y:151;width:145;height:40;text:"导入文件";textSize:19;enabled:!Music.scanning;onClicked:importDialog.open()}
    Label {visible:false;objectName:"musicScanStatus";x:1060;y:163;width:365;text:Music.scanText||("音乐库 "+Music.count+" 首 · "+Music.roots.length+" 个目录");textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
    Item {
        id:local;anchors.fill:parent
        TrackArtwork {x:56;y:216;width:310;height:310;source:Music.current.cover||""}
        Column {
            x:56;y:546;width:252;spacing:7
            Label {width:parent.width;text:MediaState.trackTitle;textFormat:Text.PlainText;font.pixelSize:30;maximumLineCount:2;wrapMode:Text.Wrap;elide:Text.ElideRight}
            Label {width:parent.width;text:Music.current.artist||"未知歌手";textFormat:Text.PlainText;font.pixelSize:20;color:Theme.muted;elide:Text.ElideRight}
            Label {
                width:parent.width
                text:(Music.current.album||"").trim()
                visible:text!==""&&text.toLowerCase()!==MediaState.trackTitle.trim().toLowerCase()
                textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight
            }
            UiButton {objectName:"relocateMusic";width:135;height:40;text:"重新定位";textSize:17;visible:Music.current.available===false;onClicked:{page.relocationIndex=MediaState.track;relocationDialog.open()}}
        }
        UiButton {objectName:"favoriteCurrent";x:318;y:543;width:48;height:48;text:Music.current.favorite?"★":"☆";textSize:24;caption:Music.current.favorite?"取消收藏":"收藏当前歌曲";Accessible.name:caption;selected:!!Music.current.favorite;onClicked:Music.toggleFavorite(MediaState.track)}
        LyricsView {x:416;y:215;width:605;height:337}
        Label {x:426;y:569;width:565;text:MediaState.mediaStatus==="loading"?"正在加载音源…":MediaState.mediaStatus==="error"?MediaState.mediaError:!AudioOutputs.available?"请连接扬声器或耳机":MediaState.playing?"正在播放":"已暂停";textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
        SoftSlider {objectName:"seekSlider";x:416;y:598;width:605;from:0;to:Math.max(1,MediaState.duration);value:MediaState.position;enabled:MediaState.duration>0&&!PhoneAudio.active&&MediaState.mediaStatus!=="error";onMoved:MediaState.seek(value)}
        Label {x:426;y:639;text:page.shell.formatTime(MediaState.position);font.pixelSize:16;color:Theme.muted}
        Label {x:930;y:639;width:80;text:page.shell.formatTime(MediaState.duration);font.pixelSize:16;color:Theme.muted;horizontalAlignment:Text.AlignRight}
        UiButton {objectName:"fullMusicPrevious";x:562;y:681;width:65;height:65;quiet:true;iconName:"previous";enabled:!PhoneAudio.active;onClicked:MediaState.nextTrack(-1)}
        UiButton {objectName:"fullMusicPlay";x:670;y:673;width:80;height:80;selected:true;enabled:AudioOutputs.available&&!PhoneAudio.active&&Music.current.available!==false;iconName:MediaState.playing?"pause":"play";onClicked:MediaState.playing=!MediaState.playing}
        UiButton {objectName:"fullMusicNext";x:792;y:681;width:65;height:65;quiet:true;iconName:"skip";enabled:!PhoneAudio.active;onClicked:MediaState.nextTrack()}
        UiButton {objectName:"playMode";x:876;y:692;width:145;height:45;text:MediaState.playMode==="all"?"列表循环":MediaState.playMode==="one"?"单曲循环":"随机播放";textSize:16;quiet:true;onClicked:MediaState.playMode=MediaState.playMode==="all"?"one":MediaState.playMode==="one"?"shuffle":"all"}
        UiButton {objectName:"retryMusic";x:426;y:686;width:120;height:48;text:"重试";textSize:16;visible:MediaState.mediaStatus==="error";enabled:!PhoneAudio.active;onClicked:MediaState.retryTrack()}
        Surface {
            x:1060;y:205;width:485;height:557
            Label {x:22;y:20;text:"音乐库";font.pixelSize:25}
            Label {x:300;y:28;width:160;text:Music.rows.length+" / "+Music.count+" 首";font.pixelSize:15;color:Theme.muted;horizontalAlignment:Text.AlignRight}
            Controls.TextField {placeholderTextColor:Theme.muted;selectionColor:Theme.active;selectedTextColor:Theme.ink;objectName:"musicSearch";x:20;y:65;width:310;height:44;placeholderText:"搜索歌曲、歌手或专辑";text:Music.query;onTextEdited:Music.query=text;font.pixelSize:17;color:Theme.ink;selectByMouse:true;background:Rectangle{radius:13;color:Theme.secondary}}
            SoftComboBox {objectName:"musicSort";x:341;y:65;width:124;height:44;model:["歌名排序","歌手排序","加入顺序"];font.pixelSize:15;currentIndex:["title","artist","added"].indexOf(Music.sort);onActivated:Music.sort=["title","artist","added"][currentIndex]}
            SoftComboBox {objectName:"musicCategory";x:20;y:121;width:445;height:43;model:["全部歌曲","我的收藏","按歌手","按专辑","按文件夹"];font.pixelSize:16;currentIndex:["all","favorites","artist","album","folder"].indexOf(Music.category);onActivated:Music.category=["all","favorites","artist","album","folder"][currentIndex]}
            SoftComboBox {objectName:"musicGroup";x:20;y:174;width:445;height:43;visible:["artist","album","folder"].indexOf(Music.category)>=0;model:["全部分组"].concat(Music.groups);font.pixelSize:15;currentIndex:Music.group===""?0:Math.max(0,model.indexOf(Music.group));onActivated:Music.group=currentIndex===0?"":currentText}
            ListView {
                id:songs;objectName:"musicLibraryList";x:15;y:["artist","album","folder"].indexOf(Music.category)>=0?230:179;width:455;height:510-y;clip:true;spacing:8;model:Music.rows;boundsBehavior:Flickable.StopAtBounds
                Controls.ScrollBar.vertical:Controls.ScrollBar {policy:Controls.ScrollBar.AsNeeded}
                delegate:UiButton {
                    required property var modelData
                    objectName:"trackRow"+modelData.index;width:455;height:76;selected:modelData.index===MediaState.track;enabled:!PhoneAudio.active
                    contentItem:Item {
                        TrackArtwork {x:0;y:3;width:50;height:50;source:modelData.cover||""}
                        Label {x:66;y:4;width:254;text:modelData.title;textFormat:Text.PlainText;font.pixelSize:20;elide:Text.ElideRight}
                        Label {x:66;y:37;width:254;text:modelData.available===false?"文件不可用 · 可重新定位":(modelData.artist||modelData.source||"未知歌手");textFormat:Text.PlainText;font.pixelSize:14;color:Theme.muted;elide:Text.ElideRight}
                        UiButton {x:325;y:8;width:42;height:44;quiet:true;text:modelData.favorite?"★":"☆";textSize:24;caption:"切换收藏";onClicked:Music.toggleFavorite(modelData.index)}
                        UiButton {x:377;y:8;width:42;height:44;quiet:true;text:"×";textSize:24;caption:"从音乐库移除，保留文件";onClicked:Music.remove(modelData.index)}
                    }
                    onClicked:{if(modelData.available===false){page.relocationIndex=modelData.index;relocationDialog.open()}else Music.playFromView(modelData.index)}
                }
                Label {anchors.centerIn:parent;width:parent.width;text:"没有匹配的歌曲";visible:songs.count===0;font.pixelSize:20;color:Theme.muted;horizontalAlignment:Text.AlignHCenter}
            }
            Label {x:22;y:527;width:200;text:"支持子目录 · 文件保留";font.pixelSize:14;color:Theme.muted}
            UiButton {objectName:"musicFoldersButton";x:219;y:515;width:120;height:37;text:"目录管理";textSize:15;onClicked:page.shell.openPopup("musicFolders")}
            UiButton {objectName:"rescanMusic";x:344;y:515;width:120;height:37;text:Music.scanning?"停止扫描":"重新扫描";textSize:15;onClicked:Music.scanning?Music.cancelScan():Music.rescan()}
        }
    }
}
