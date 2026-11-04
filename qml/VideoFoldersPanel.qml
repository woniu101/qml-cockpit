import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
Item {
    required property var shell
    Label {x:30;y:27;text:"视频目录";font.pixelSize:28}
    Label {x:30;y:88;width:520;text:"包含子目录；移除索引会保留原文件。";font.pixelSize:17;color:Theme.muted}
    FolderDialog {id:folder;title:"添加视频目录";onAccepted:Videos.importFolder(selectedFolder)}
    Flickable {x:30;y:134;width:520;height:345;contentHeight:roots.height;clip:true;Controls.ScrollBar.vertical:Controls.ScrollBar{}
        Column {id:roots;width:510;spacing:12
            Repeater {model:Videos.roots
                Rectangle {required property string modelData;width:510;height:101;radius:16;color:Theme.secondary
                    Label{x:16;y:14;width:478;text:modelData;textFormat:Text.PlainText;font.pixelSize:17;elide:Text.ElideMiddle}
                    UiButton{x:343;y:51;width:150;height:38;text:"移除目录";textSize:16;enabled:!Videos.scanning;onClicked:Videos.removeRoot(modelData)}
                }
            }
            Label {visible:Videos.roots.length===0;width:510;text:"还没有添加视频目录";font.pixelSize:20;color:Theme.muted}
        }
    }
    UiButton {x:30;y:505;width:245;height:52;text:Videos.scanning?"停止扫描":"添加目录";iconName:Videos.scanning?"stop":"plus";onClicked:Videos.scanning?Videos.cancelScan():folder.open()}
    UiButton {x:290;y:505;width:260;height:52;text:"重新扫描";enabled:!Videos.scanning;onClicked:Videos.rescan()}
    Label{x:30;y:577;width:520;text:Videos.scanText;textFormat:Text.PlainText;font.pixelSize:14;color:Theme.muted;elide:Text.ElideRight}
}
