import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtMultimedia
Item {
    id:page
    required property var shell
    width:1600;height:Media.fullscreen?900:790
    property bool showPlayer:false
    property bool controlsShown:true
    property bool playlistVisible:true
    property string relocationId:""
    readonly property bool missingAudioOutput:Media.kind==="video"&&Media.hasAudio&&!AudioOutputs.available
    onShowPlayerChanged:if(!showPlayer)Media.videoPageLeft()
    onVisibleChanged:if(!visible)Media.videoPageLeft()
    FileDialog {id:files;title:"导入本地视频";fileMode:FileDialog.OpenFiles;nameFilters:["视频文件 (*.mp4 *.mkv *.webm *.mov *.avi *.m4v *.mpeg *.mpg *.wmv)"];onAccepted:Videos.importFiles(selectedFiles)}
    FolderDialog {id:folder;title:"添加视频目录（包含子目录）";onAccepted:Videos.importFolder(selectedFolder)}
    FileDialog {id:relocate;title:"重新定位视频文件";fileMode:FileDialog.OpenFile;nameFilters:files.nameFilters;onAccepted:Videos.relocate(page.relocationId,selectedFile)}
    DropArea {anchors.fill:parent;enabled:page.visible&&!Media.fullscreen;onDropped:drop=>{if(drop.hasUrls){Videos.importFiles(drop.urls);drop.acceptProposedAction()}}}
    Component.onCompleted:Media.videoOutput=output.videoSink
    Timer {id:hideControls;interval:3500;onTriggered:if(Media.playing)page.controlsShown=false}
    Connections {target:Media;function onChanged(){if(!Media.playing||Media.phase==="blocked"||Media.phase==="error")page.controlsShown=true;else if(!hideControls.running)hideControls.restart()}}
    Label {visible:!Media.fullscreen;x:56;y:157;width:930;text:Videos.scanning?Videos.scanText:Media.videoAllowed?"本地视频 · 支持目录与子目录":"本地视频 · 请在驻车后观看";textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight}
    Item {visible:!page.showPlayer&&!Media.fullscreen;anchors.fill:parent
        Column {x:56;y:216;spacing:16
            Repeater {model:[{label:"全部视频",value:"all"},{label:"最近观看",value:"recent"},{label:"按文件夹",value:"folder"}]
                UiButton {required property var modelData;width:147;height:46;text:modelData.label;textSize:19;selected:Videos.category===modelData.value;onClicked:Videos.category=modelData.value}
            }
        }
        Rectangle {x:227;y:216;width:1;height:531;color:Theme.line}
        Label {x:265;y:216;text:"本地视频";font.pixelSize:26}
        Controls.TextField {placeholderTextColor:Theme.muted;selectionColor:Theme.active;selectedTextColor:Theme.ink;objectName:"videoSearch";x:977;y:210;width:326;height:43;placeholderText:"搜索片名或文件夹";text:Videos.query;onTextEdited:Videos.query=text;color:Theme.ink;font.pixelSize:17;selectByMouse:true;background:Rectangle{radius:15;color:Theme.surface}}
        UiButton {objectName:"videoImportFiles";x:1325;y:210;width:220;height:44;text:"导入视频";iconName:"plus";textSize:18;enabled:!Videos.scanning;onClicked:files.open()}
        SoftComboBox {x:265;y:267;width:1280;height:43;visible:Videos.category==="folder";model:["全部文件夹"].concat(Videos.folders);currentIndex:Videos.folder===""?0:Math.max(0,model.indexOf(Videos.folder));onActivated:Videos.folder=currentIndex===0?"":currentText}
        GridView {id:grid;objectName:"videoLibrary";x:265;y:Videos.category==="folder"?326:276;width:1305;height:Videos.category==="folder"?320:370;clip:true;cellWidth:435;cellHeight:350;model:Videos;boundsBehavior:Flickable.StopAtBounds
            Controls.ScrollBar.vertical:Controls.ScrollBar{}
            delegate:Item {required property int index;objectName:"videoTile"+index;required property string videoUrl;required property string videoTitle;required property string videoFolder;required property string videoCover;required property double videoDuration;required property double videoBookmark;required property bool videoAvailable
                width:410;height:330
                Surface {anchors.fill:parent}
                Rectangle {x:12;y:12;width:386;height:205;radius:17;color:Theme.secondary;clip:true
                    Image {anchors.fill:parent;source:videoCover;fillMode:Image.PreserveAspectCrop;asynchronous:true;visible:videoCover!==""}
                    Icon {anchors.centerIn:parent;width:52;height:52;name:"video";tint:Theme.green;visible:videoCover===""}
                    Rectangle {x:302;y:167;width:70;height:28;radius:10;color:"#23332f";Label{anchors.centerIn:parent;text:page.shell.formatTime(videoDuration);font.pixelSize:15;color:"white"}}
                }
                Label {x:24;y:231;width:360;text:videoTitle;textFormat:Text.PlainText;font.pixelSize:24;elide:Text.ElideRight}
                Label {x:24;y:269;width:360;text:videoAvailable?videoFolder.split(/[\\/]/).pop():"文件不可用 · 点击重新定位";textFormat:Text.PlainText;font.pixelSize:16;color:Theme.muted;elide:Text.ElideRight}
                Label {x:24;y:298;width:350;text:videoBookmark>0?"继续观看 · "+page.shell.formatTime(videoBookmark):"尚未观看";font.pixelSize:16;color:Theme.green}
                MouseArea {anchors.fill:parent;onClicked:{if(!videoAvailable){page.relocationId=videoUrl;relocate.open()}else{Videos.play(videoUrl);page.showPlayer=Media.kind==="video"}}}
                UiButton {x:354;y:22;width:36;height:36;quiet:true;text:"×";textSize:23;caption:"从视频库移除，保留原文件";onClicked:Videos.remove(videoUrl)}
            }
        }
        Surface {x:265;y:276;width:1280;height:370;visible:Videos.count===0
            Icon {x:607;y:68;width:64;height:64;name:"video";tint:Theme.green}
            Label {x:0;y:169;width:1280;horizontalAlignment:Text.AlignHCenter;text:Videos.query!==""?"没有匹配的视频":Videos.category==="recent"?"还没有观看记录":"把喜欢的视频，带到旅途中";font.pixelSize:28}
            Label {x:0;y:219;width:1280;horizontalAlignment:Text.AlignHCenter;text:"添加本地文件或目录，支持包含子文件夹";font.pixelSize:19;color:Theme.muted}
            UiButton {objectName:"videoEmptyAdd";x:521;y:275;width:238;height:54;text:"添加视频目录";iconName:"plus";selected:true;enabled:!Videos.scanning;onClicked:folder.open()}
        }
        Label {x:265;y:690;width:720;text:Videos.scanText||("共 "+Videos.count+" 个视频");font.pixelSize:17;color:Theme.muted;textFormat:Text.PlainText;elide:Text.ElideRight}
        UiButton {objectName:"videoImportFolder";x:1307;y:674;width:238;height:50;text:Videos.scanning?"停止扫描":"添加视频目录";iconName:Videos.scanning?"stop":"plus";textSize:19;onClicked:Videos.scanning?Videos.cancelScan():folder.open()}
    }
    Item {visible:page.showPlayer||Media.fullscreen;anchors.fill:parent
        UiButton {objectName:"videoBack";visible:!Media.fullscreen;x:56;y:216;width:113;height:42;text:"返回";iconName:"back";textSize:18;onClicked:page.showPlayer=false}
        Label {visible:!Media.fullscreen;x:193;y:219;width:690;text:Media.title;textFormat:Text.PlainText;font.pixelSize:24;elide:Text.ElideRight}
        UiButton {objectName:"videoPlaylistToggle";visible:!Media.fullscreen;x:1345;y:216;width:200;height:42;text:page.playlistVisible?"收起列表":"播放列表";textSize:18;onClicked:page.playlistVisible=!page.playlistVisible}
        Item {
            id:frame;objectName:"videoFrame";x:Media.fullscreen?0:56;y:Media.fullscreen?0:276;width:Media.fullscreen?1600:page.playlistVisible?1104:1489;height:Media.fullscreen?900:468
            Rectangle {anchors.fill:parent;radius:Media.fullscreen?0:25;color:"#17251f"}
            VideoOutput {id:output;objectName:"videoOutput";anchors.fill:parent;fillMode:VideoOutput.PreserveAspectFit;visible:Media.phase!=="blocked"&&Media.phase!=="error"&&!page.missingAudioOutput}
            Label {x:50;y:parent.height-180;width:parent.width-100;text:output.videoSink.subtitleText;textFormat:Text.PlainText;horizontalAlignment:Text.AlignHCenter;font.pixelSize:24;color:"white";style:Text.Outline;styleColor:"#23332f";wrapMode:Text.WordWrap;visible:Media.subtitleTrack>=0&&Media.phase!=="blocked"}
            MouseArea {anchors.fill:parent;onClicked:{page.controlsShown=!page.controlsShown;if(page.controlsShown)hideControls.restart()}}
            Column {anchors.centerIn:parent;spacing:24;visible:Media.phase==="blocked"||Media.phase==="error"||page.missingAudioOutput
                Icon {anchors.horizontalCenter:parent.horizontalCenter;width:55;height:55;name:Media.phase==="blocked"?"lock":page.missingAudioOutput?"volume":"video";tint:"#c3d5c6"}
                Label {objectName:"videoRestriction";anchors.horizontalCenter:parent.horizontalCenter;text:Media.phase==="blocked"?"请在驻车后观看视频":Media.phase==="error"?"此视频暂时无法播放":"没有可用的音频输出";font.pixelSize:30;color:"#e5eee5"}
                Label {anchors.horizontalCenter:parent.horizontalCenter;text:Media.phase==="blocked"?"视频已暂停，当前进度已保留":Media.phase==="error"?"可重试，或返回视频库选择其他文件":"视频已暂停，连接扬声器或耳机后手动继续";font.pixelSize:19;color:"#a9b8aa"}
                UiButton {anchors.horizontalCenter:parent.horizontalCenter;visible:Media.phase==="error";width:160;height:48;text:"重试";onClicked:Media.toggle()}
                UiButton {anchors.horizontalCenter:parent.horizontalCenter;visible:page.missingAudioOutput&&Media.phase!=="blocked"&&Media.phase!=="error";width:180;height:48;text:"音频输出";onClicked:page.shell.openPopup("audio")}
            }
            Label {anchors.centerIn:parent;visible:Media.busy;text:"正在加载视频…";font.pixelSize:22;color:"#e5eee5"}
            Label {x:parent.width-128;y:24;width:106;text:"无音轨";horizontalAlignment:Text.AlignRight;font.pixelSize:17;color:"#c3d5c6";visible:!Media.busy&&!Media.hasAudio&&Media.phase!=="error"&&Media.phase!=="blocked"}
            Row {x:20;y:18;spacing:10;visible:page.controlsShown&&Media.phase!=="blocked"
                SoftComboBox {width:165;height:40;visible:Media.audioTracks.length>1;model:Media.audioTracks.map(v=>v.label);currentIndex:Media.audioTrack;onActivated:Media.audioTrack=currentIndex}
                SoftComboBox {width:165;height:40;visible:Media.subtitleTracks.length>1;model:Media.subtitleTracks.map(v=>v.label);currentIndex:Media.subtitleTrack+1;onActivated:Media.subtitleTrack=currentIndex-1}
            }
            Rectangle {
                x:20;y:parent.height-100;width:parent.width-40;height:82;radius:18;color:"#23332f";visible:page.controlsShown&&Media.phase!=="blocked"&&Media.phase!=="error"&&!page.missingAudioOutput
                UiButton {objectName:"videoPlayPause";x:9;y:13;width:50;height:56;quiet:true;foreground:"white";iconName:Media.playing?"pause":"play";onClicked:{Media.toggle();hideControls.restart()}}
                UiButton {objectName:"videoBack10";x:64;y:15;width:46;height:50;quiet:true;foreground:"white";text:"−10";textSize:16;caption:"后退 10 秒";enabled:Media.canSeek;onClicked:{Media.seek(Media.position-10000);hideControls.restart()}}
                UiButton {objectName:"videoForward10";x:112;y:15;width:46;height:50;quiet:true;foreground:"white";text:"+10";textSize:16;caption:"前进 10 秒";enabled:Media.canSeek;onClicked:{Media.seek(Media.position+10000);hideControls.restart()}}
                Label {x:167;y:28;width:135;text:page.shell.formatTime(Media.position)+" / "+page.shell.formatTime(Media.duration);font.pixelSize:16;color:"#e5eee5"}
                SoftSlider {objectName:"videoSeek";x:310;y:19;width:parent.width-660;height:44;from:0;to:Math.max(1,Media.duration);value:Media.position;enabled:Media.canSeek;onMoved:{Media.seek(value);hideControls.restart()}}
                SoftComboBox {objectName:"videoSpeed";x:parent.width-333;y:19;width:93;height:44;model:["0.5×","1.0×","1.25×","1.5×","2.0×"];currentIndex:Math.max(0,[0.5,1,1.25,1.5,2].indexOf(Media.rate));onActivated:Media.rate=[0.5,1,1.25,1.5,2][currentIndex]}
                Icon {x:parent.width-225;y:29;width:24;height:24;name:"volume";tint:Media.hasAudio?"#e5eee5":"#7e8d84"}
                SoftSlider {objectName:"videoVolume";x:parent.width-195;y:19;width:115;height:44;from:0;to:100;value:MediaState.volume;enabled:Media.hasAudio&&AudioOutputs.available;onMoved:{MediaState.volume=Math.round(value);hideControls.restart()}}
                UiButton {objectName:"videoFullscreen";x:parent.width-67;y:15;width:50;height:50;quiet:true;foreground:"white";iconName:Media.fullscreen?"close":"expand";caption:Media.fullscreen?"退出全屏":"全屏观看";onClicked:{Media.fullscreen=!Media.fullscreen;page.controlsShown=true;hideControls.restart()}}
            }
            UiButton {visible:Media.fullscreen&&page.controlsShown;x:32;y:26;width:143;height:50;text:"返回";iconName:"back";onClicked:Media.fullscreen=false}
        }
        Surface {x:1184;y:276;width:361;height:468;visible:!Media.fullscreen&&page.playlistVisible
            Label {x:27;y:23;text:"播放列表";font.pixelSize:24}
            ListView {x:16;y:72;width:329;height:375;clip:true;model:Videos;spacing:12;Controls.ScrollBar.vertical:Controls.ScrollBar{}
                delegate:UiButton {required property string videoUrl;required property string videoTitle;required property string videoCover;required property double videoDuration;required property bool videoAvailable
                    width:329;height:94;selected:Media.itemId===videoUrl;enabled:videoAvailable
                    contentItem:Item {Rectangle{x:0;y:5;width:98;height:65;radius:10;color:Theme.secondary;Image{anchors.fill:parent;source:videoCover;fillMode:Image.PreserveAspectCrop;asynchronous:true}}Label{x:116;y:7;width:185;text:videoTitle;textFormat:Text.PlainText;font.pixelSize:20;elide:Text.ElideRight}Label{x:116;y:43;text:page.shell.formatTime(videoDuration);font.pixelSize:16;color:Theme.muted}}
                    onClicked:Videos.play(videoUrl)
                }
            }
        }
    }
}
