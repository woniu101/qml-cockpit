import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
Item {
    id:page
    required property var shell
    width:1600;height:790
    property bool phoneView:PhoneAudio.active
    property bool receiverWasActive:PhoneAudio.active
    property int relocationIndex:-1
    FileDialog {id:importDialog;title:"加入本地音频";fileMode:FileDialog.OpenFiles;nameFilters:["音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg *.aac *.opus *.wma)"];onAccepted:Music.importFiles(selectedFiles)}
    FolderDialog {id:folderDialog;title:"添加音乐目录（包含子文件夹）";onAccepted:Music.importFolder(selectedFolder)}
    FileDialog {id:relocationDialog;title:"重新定位音乐文件";fileMode:FileDialog.OpenFile;nameFilters:importDialog.nameFilters;onAccepted:Music.relocate(page.relocationIndex,selectedFile)}
    DropArea {anchors.fill:parent;onDropped:drop=>{if(drop.hasUrls){Music.importFiles(drop.urls);drop.acceptProposedAction()}}}
    Component.onCompleted:receiverWasActive=PhoneAudio.active
    Connections {target:PhoneAudio;function onChanged(){if(PhoneAudio.phase==="connecting"||PhoneAudio.phase==="connected")page.phoneView=true;else if(!PhoneAudio.active&&page.receiverWasActive)page.phoneView=false;page.receiverWasActive=PhoneAudio.active}}
    Label {x:56;y:95;text:"音乐";font.pixelSize:34;font.weight:Font.Medium}
    Row {x:195;y:94;spacing:8
        UiButton {objectName:"musicLocalSource";width:155;height:48;text:"本地音乐";textSize:19;selected:!page.phoneView;caption:PhoneAudio.active?"切换到本地并断开手机音乐":"本地音乐库与歌词";onClicked:{PhoneAudio.disconnectDevice();page.phoneView=false}}
        UiButton {objectName:"musicPhoneAudio";width:205;height:48;text:"手机蓝牙音乐";iconName:"bluetooth";textSize:18;selected:page.phoneView;onClicked:page.phoneView=true}
    }
    Label {objectName:"musicOutputSummary";x:56;y:156;width:930;text:page.phoneView?PhoneAudio.text+" · Windows 默认输出":AudioOutputs.statusText;textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight}
    UiButton {objectName:"musicAudioOutputs";x:812;y:95;width:210;height:48;text:page.phoneView?"系统声音设置":"音频输出";iconName:"volume";textSize:18;onClicked:page.phoneView?AudioOutputs.openSettings():page.shell.openPopup("audio")}
    UiButton {objectName:"musicQueueButton";x:580;y:95;width:211;height:48;text:"播放队列 "+Music.queue.length;textSize:18;visible:!page.phoneView;onClicked:page.shell.openPopup("musicQueue")}
    UiButton {objectName:"importFolder";x:1060;y:99;width:232;height:48;text:"添加音乐目录";iconName:"plus";textSize:19;enabled:!Music.scanning;onClicked:folderDialog.open()}
    UiButton {objectName:"importMusic";x:1310;y:99;width:235;height:48;text:"导入文件";textSize:19;enabled:!Music.scanning;onClicked:importDialog.open()}
    Label {objectName:"musicScanStatus";x:1060;y:163;width:365;text:Music.scanText||("音乐库 "+Music.count+" 首 · "+Music.roots.length+" 个目录");textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
    UiButton {objectName:"cancelMusicScan";x:1430;y:151;width:115;height:38;text:"停止扫描";textSize:15;visible:Music.scanning;onClicked:Music.cancelScan()}
    Item {
        id:local;anchors.fill:parent;visible:!page.phoneView
        TrackArtwork {x:56;y:216;width:310;height:310;source:Music.current.cover||""}
        Label {x:56;y:552;width:310;text:Vehicle.trackTitle;textFormat:Text.PlainText;font.pixelSize:31;maximumLineCount:2;wrapMode:Text.WordWrap;elide:Text.ElideRight}
        Label {x:56;y:640;width:310;text:Music.current.artist||"未知歌手";textFormat:Text.PlainText;font.pixelSize:20;color:Theme.muted;elide:Text.ElideRight}
        Label {x:56;y:676;width:310;text:Music.current.album||"未标注专辑";textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight}
        UiButton {objectName:"favoriteCurrent";x:56;y:720;width:140;height:42;text:Music.current.favorite?"★ 已收藏":"☆ 收藏";textSize:18;selected:!!Music.current.favorite;onClicked:Music.toggleFavorite(Vehicle.track)}
        UiButton {objectName:"relocateMusic";x:208;y:720;width:158;height:42;text:"重新定位";textSize:17;visible:Music.current.available===false;onClicked:{page.relocationIndex=Vehicle.track;relocationDialog.open()}}
        LyricsView {x:416;y:215;width:605;height:337}
        Label {x:426;y:569;width:565;text:Vehicle.mediaStatus==="loading"?"正在加载音源…":Vehicle.mediaStatus==="error"?Vehicle.mediaError:!AudioOutputs.available?"请连接扬声器或耳机":Vehicle.playing?"正在播放":"已暂停";textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
        SoftSlider {objectName:"seekSlider";x:416;y:598;width:605;from:0;to:Math.max(1,Vehicle.duration);value:Vehicle.position;enabled:Vehicle.duration>0&&!PhoneAudio.active&&Vehicle.mediaStatus!=="error";onMoved:Vehicle.seek(value)}
        Label {x:426;y:639;text:page.shell.formatTime(Vehicle.position);font.pixelSize:16;color:Theme.muted}
        Label {x:930;y:639;width:80;text:page.shell.formatTime(Vehicle.duration);font.pixelSize:16;color:Theme.muted;horizontalAlignment:Text.AlignRight}
        UiButton {objectName:"fullMusicPrevious";x:562;y:681;width:65;height:65;quiet:true;iconName:"previous";enabled:!PhoneAudio.active;onClicked:Vehicle.nextTrack(-1)}
        UiButton {objectName:"fullMusicPlay";x:670;y:673;width:80;height:80;selected:true;enabled:AudioOutputs.available&&!PhoneAudio.active&&Music.current.available!==false;iconName:Vehicle.playing?"pause":"play";onClicked:Vehicle.playing=!Vehicle.playing}
        UiButton {objectName:"fullMusicNext";x:792;y:681;width:65;height:65;quiet:true;iconName:"skip";enabled:!PhoneAudio.active;onClicked:Vehicle.nextTrack()}
        UiButton {objectName:"playMode";x:876;y:692;width:145;height:45;text:Vehicle.playMode==="all"?"列表循环":Vehicle.playMode==="one"?"单曲循环":"随机播放";textSize:16;quiet:true;onClicked:Vehicle.playMode=Vehicle.playMode==="all"?"one":Vehicle.playMode==="one"?"shuffle":"all"}
        UiButton {objectName:"retryMusic";x:426;y:686;width:120;height:48;text:"重试";textSize:16;visible:Vehicle.mediaStatus==="error";enabled:!PhoneAudio.active;onClicked:Vehicle.retryTrack()}
        Surface {
            x:1060;y:205;width:485;height:557
            Label {x:22;y:20;text:"音乐库";font.pixelSize:25}
            Label {x:300;y:28;width:160;text:Music.rows.length+" / "+Music.count+" 首";font.pixelSize:15;color:Theme.muted;horizontalAlignment:Text.AlignRight}
            Controls.TextField {objectName:"musicSearch";x:20;y:65;width:310;height:44;placeholderText:"搜索歌曲、歌手或专辑";text:Music.query;onTextEdited:Music.query=text;font.pixelSize:17;color:Theme.ink;selectByMouse:true;background:Rectangle{radius:13;color:Theme.secondary}}
            SoftComboBox {objectName:"musicSort";x:341;y:65;width:124;height:44;model:["歌名排序","歌手排序","加入顺序"];font.pixelSize:15;currentIndex:["title","artist","added"].indexOf(Music.sort);onActivated:Music.sort=["title","artist","added"][currentIndex]}
            SoftComboBox {objectName:"musicCategory";x:20;y:121;width:445;height:43;model:["全部歌曲","我的收藏","按歌手","按专辑","按文件夹"];font.pixelSize:16;currentIndex:["all","favorites","artist","album","folder"].indexOf(Music.category);onActivated:Music.category=["all","favorites","artist","album","folder"][currentIndex]}
            SoftComboBox {objectName:"musicGroup";x:20;y:174;width:445;height:43;visible:["artist","album","folder"].indexOf(Music.category)>=0;model:["全部分组"].concat(Music.groups);font.pixelSize:15;currentIndex:Music.group===""?0:Math.max(0,model.indexOf(Music.group));onActivated:Music.group=currentIndex===0?"":currentText}
            ListView {
                id:songs;objectName:"musicLibraryList";x:15;y:["artist","album","folder"].indexOf(Music.category)>=0?230:179;width:455;height:510-y;clip:true;spacing:8;model:Music.rows;boundsBehavior:Flickable.StopAtBounds
                Controls.ScrollBar.vertical:Controls.ScrollBar {policy:Controls.ScrollBar.AsNeeded}
                delegate:UiButton {
                    required property var modelData
                    objectName:"trackRow"+modelData.index;width:455;height:76;selected:modelData.index===Vehicle.track;enabled:!PhoneAudio.active
                    contentItem:Item {
                        TrackArtwork {x:0;y:3;width:50;height:50;source:modelData.cover||""}
                        Label {x:66;y:4;width:254;text:modelData.title;textFormat:Text.PlainText;font.pixelSize:20;elide:Text.ElideRight}
                        Label {x:66;y:37;width:254;text:modelData.available===false?"文件不可用 · 可重新定位":(modelData.artist||modelData.source||"未知歌手");textFormat:Text.PlainText;font.pixelSize:14;color:Theme.muted;elide:Text.ElideRight}
                        UiButton {x:325;y:8;width:42;height:44;quiet:true;text:modelData.favorite?"★":"☆";textSize:24;caption:"切换收藏";onClicked:Music.toggleFavorite(modelData.index)}
                        UiButton {x:377;y:8;width:42;height:44;quiet:true;text:"×";textSize:24;visible:modelData.index>=3;caption:"从音乐库移除，保留文件";onClicked:Music.remove(modelData.index)}
                    }
                    onClicked:{if(modelData.available===false){page.relocationIndex=modelData.index;relocationDialog.open()}else Music.playFromView(modelData.index)}
                }
                Label {anchors.centerIn:parent;width:parent.width;text:"没有匹配的歌曲";visible:songs.count===0;font.pixelSize:20;color:Theme.muted;horizontalAlignment:Text.AlignHCenter}
            }
            Label {x:22;y:527;width:200;text:"支持子目录 · 文件保留";font.pixelSize:14;color:Theme.muted}
            UiButton {objectName:"musicFoldersButton";x:219;y:515;width:120;height:37;text:"目录管理";textSize:15;onClicked:page.shell.openPopup("musicFolders")}
            UiButton {objectName:"rescanMusic";x:344;y:515;width:120;height:37;text:"重新扫描";textSize:15;enabled:!Music.scanning;onClicked:Music.rescan()}
        }
    }
    Item {
        anchors.fill:parent;visible:page.phoneView
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
            Label {x:28;y:165;width:429;text:"接收声音输出至 Windows 默认设备。\n本地音乐播放列表保留在“本地音乐”中。";font.pixelSize:20;color:Theme.muted;wrapMode:Text.WordWrap}
            UiButton {objectName:"phoneManageConnection";x:28;y:280;width:429;height:58;text:"蓝牙设备与音乐连接";iconName:"bluetooth";textSize:20;onClicked:page.shell.openPopup("bluetooth")}
            UiButton {x:28;y:363;width:429;height:58;text:"系统声音设置";iconName:"volume";textSize:20;onClicked:AudioOutputs.openSettings()}
            Label {x:28;y:471;width:429;text:"切换到本地音乐会断开手机接收，\n本地音乐等待手动播放。";font.pixelSize:16;color:Theme.muted;wrapMode:Text.WordWrap}
        }
    }
}
