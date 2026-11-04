import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
Item {
    id:panel
    required property var shell
    property bool directories:shell.popup==="musicFolders"
    FolderDialog {id:folderDialog;title:"添加音乐目录（包含子目录）";onAccepted:Music.importFolder(selectedFolder)}
    Label {x:30;y:27;text:panel.directories?"音乐目录管理":"播放队列";font.pixelSize:28}
    Label {objectName:"musicManagerSummary";x:30;y:91;width:520;text:panel.directories?"已添加 "+Music.roots.length+" 个目录":Music.queueText;textFormat:Text.PlainText;font.pixelSize:19;color:Theme.green;elide:Text.ElideRight}
    Label {x:30;y:128;width:520;text:panel.directories?"移除目录会清理其独占的库条目；共享或单独导入的歌曲保留，原文件保留。":MediaState.playMode==="shuffle"?"随机模式在此范围内选歌，上一首返回实际播放历史。":"列表按下方顺序切歌，浏览其他分类不会改变队列。";font.pixelSize:16;color:Theme.muted;wrapMode:Text.WordWrap}
    ListView {
        id:list;objectName:"musicManagerList";x:30;y:190;width:520;height:315;clip:true;spacing:10;model:panel.directories?Music.rootEntries:Music.queue
        Controls.ScrollBar.vertical:Controls.ScrollBar{policy:Controls.ScrollBar.AsNeeded}
        delegate:Rectangle {
            required property var modelData
            required property int index
            width:510;height:panel.directories?111:80;radius:16;color:!panel.directories&&modelData.current?Theme.active:Theme.secondary
            Label {x:16;y:12;width:panel.directories?478:305;text:panel.directories?modelData.path:modelData.title;textFormat:Text.PlainText;font.pixelSize:panel.directories?16:20;elide:Text.ElideMiddle}
            Label {x:16;y:panel.directories?50:45;width:305;text:panel.directories?(modelData.available?"可访问":"目录断开或已移动")+" · "+modelData.count+" 首":(modelData.current?"当前曲目 · ":"")+(modelData.available===false?"音源不可用":modelData.artist||modelData.source||"本地音乐");textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
            UiButton {objectName:"removeMusicRoot"+index;x:343;y:58;width:151;height:42;visible:panel.directories;text:"移除目录";textSize:16;enabled:!Music.scanning;onClicked:Music.removeRoot(modelData.path)}
            MouseArea {anchors.fill:parent;visible:!panel.directories;enabled:!PhoneAudio.active&&modelData.available!==false;onClicked:Music.playQueued(modelData.url)}
            Row {x:333;y:18;spacing:5;visible:!panel.directories
                UiButton {objectName:"queueUp"+index;width:50;height:44;text:"↑";textSize:21;quiet:true;enabled:index>0;caption:"向前移动";onClicked:MediaState.moveQueueTrack(modelData.url,-1)}
                UiButton {objectName:"queueDown"+index;width:50;height:44;text:"↓";textSize:21;quiet:true;enabled:index<Music.queue.length-1;caption:"向后移动";onClicked:MediaState.moveQueueTrack(modelData.url,1)}
                UiButton {objectName:"queueRemove"+index;width:50;height:44;text:"×";textSize:24;quiet:true;enabled:!modelData.current;caption:"从队列移除，保留库条目";onClicked:MediaState.removeQueueTrack(modelData.url)}
            }
        }
        Label {anchors.centerIn:parent;width:parent.width;text:panel.directories?"尚未添加扫描目录":"当前队列为空";visible:list.count===0;font.pixelSize:20;color:Theme.muted;horizontalAlignment:Text.AlignHCenter}
    }
    Label {x:30;y:519;width:520;text:panel.directories?(Music.scanning?Music.scanText:"自动发现新增文件，断开后保留索引并暂停播放。"):"点击播放 · 箭头调整顺序 · 当前曲目保留在队列中。";textFormat:Text.PlainText;font.pixelSize:15;color:Theme.muted;elide:Text.ElideRight}
    UiButton {objectName:"addManagedMusicRoot";x:30;y:552;width:247;height:43;text:panel.directories?"添加目录":"查看音乐目录";textSize:17;enabled:!panel.directories||!Music.scanning;onClicked:panel.directories?folderDialog.open():panel.shell.openPopup("musicFolders")}
    UiButton {objectName:"refreshManagedMusicRoots";x:292;y:552;width:258;height:43;text:panel.directories?(Music.scanning?"停止扫描":"重新扫描"):"返回本地音乐";textSize:17;onClicked:{if(panel.directories){if(Music.scanning)Music.cancelScan();else Music.rescan()}else panel.shell.popup=""}}
}
