import QtQuick
import QtQuick.Controls as Controls
Item {
    id:page
    required property var shell
    width:1600;height:790
    readonly property bool selectedSession:Media.kind==="radio"&&Media.itemId===Radio.currentId
    readonly property string phase:selectedSession?Media.phase:"stopped"
    readonly property bool pending:selectedSession&&Media.busy
    Label {x:56;y:157;text:"收藏喜欢的声音，继续未完的旅程";font.pixelSize:17;color:Theme.muted}
    Surface {
        x:56;y:215;width:925;height:535
        Rectangle {x:34;y:37;width:182;height:182;radius:32;color:Theme.active
            Icon {x:65;y:32;width:52;height:52;name:"radio";tint:Theme.green}
            Label {x:0;y:111;width:182;horizontalAlignment:Text.AlignHCenter;text:Radio.currentId.indexOf("cnr-")===0?"央广":"港台";font.pixelSize:30;color:Theme.green}
        }
        Label {x:258;y:45;width:600;text:Radio.currentCategory;textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted}
        Label {objectName:"radioCurrentTitle";x:256;y:82;width:615;text:Radio.currentName;textFormat:Text.PlainText;font.pixelSize:42;elide:Text.ElideRight}
        Rectangle {x:258;y:147;width:111;height:34;radius:17;color:Theme.active;Label {anchors.centerIn:parent;text:"网络直播";font.pixelSize:17;color:Theme.green}}
        UiButton {objectName:"radioFavorite";x:386;y:140;width:145;height:48;text:Radio.currentFavorite?"已收藏":"收藏";iconName:"heart";textSize:17;quiet:true;onClicked:Radio.toggleFavorite(Radio.currentId)}
        Label {objectName:"radioState";x:258;y:214;width:600;text:page.phase==="resolving"||page.phase==="connecting"?"正在连接电台…":page.phase==="buffering"?"正在缓冲…":page.phase==="reconnecting"?"连接中断，正在重连…":page.phase==="error"?"电台暂时不可用":page.selectedSession&&Media.playing?"正在收听":"尚未播放";font.pixelSize:25;color:page.phase==="error"?Theme.ink:Theme.green}
        Label {x:258;y:264;width:600;text:page.phase==="error"||page.phase==="reconnecting"?Media.error:page.pending?"等待直播音频，可随时取消":"直播内容由电台实时提供";textFormat:Text.PlainText;font.pixelSize:18;color:Theme.muted;wrapMode:Text.WordWrap;maximumLineCount:2;elide:Text.ElideRight}
        UiButton {objectName:"radioPlayStop";x:258;y:339;width:229;height:58;selected:true;enabled:AudioOutputs.available&&!PhoneAudio.active;text:page.pending?"取消连接":page.selectedSession&&Media.playing?"停止收听":page.phase==="error"?"重新连接":"开始收听";iconName:page.pending?"close":page.selectedSession&&Media.playing?"stop":"play";textSize:21;onClicked:Radio.toggle()}
        UiButton {objectName:"radioPrevious";x:509;y:339;width:68;height:58;iconName:"previous";quiet:true;enabled:AudioOutputs.available&&!PhoneAudio.active;caption:"上一收藏台；未收藏时切换全部电台";onClicked:Radio.next(-1)}
        UiButton {objectName:"radioNext";x:591;y:339;width:68;height:58;iconName:"skip";quiet:true;enabled:AudioOutputs.available&&!PhoneAudio.active;caption:"下一收藏台；未收藏时切换全部电台";onClicked:Radio.next(1)}
        Rectangle {x:34;y:442;width:857;height:1;color:Theme.line}
        Icon {x:36;y:474;width:23;height:23;name:"volume";tint:Theme.muted}
        Label {x:70;y:475;width:800;text:PhoneAudio.active?"先在手机蓝牙页断开音乐接收，再收听电台":AudioOutputs.statusText;textFormat:Text.PlainText;font.pixelSize:17;color:Theme.muted;elide:Text.ElideRight}
    }
    Surface {
        x:1005;y:215;width:540;height:535
        Row {x:27;y:22;spacing:8
            Repeater {model:[{label:"全部",value:"all"},{label:"收藏",value:"favorites"},{label:"最近",value:"recent"}]
                UiButton {required property var modelData;objectName:"radioFilter_"+modelData.value;width:88;height:38;text:modelData.label;textSize:18;selected:Radio.filter===modelData.value;onClicked:Radio.filter=modelData.value}
            }
        }
        Controls.TextField {placeholderTextColor:Theme.muted;selectionColor:Theme.active;selectedTextColor:Theme.ink;objectName:"radioSearch";x:337;y:22;width:176;height:38;placeholderText:"搜索电台";text:Radio.query;onTextEdited:Radio.query=text;color:Theme.ink;font.pixelSize:16;selectByMouse:true;background:Rectangle{radius:14;color:Theme.secondary}}
        ListView {objectName:"radioStations";x:22;y:80;width:496;height:411;clip:true;model:Radio;spacing:12;boundsBehavior:Flickable.StopAtBounds
            Controls.ScrollBar.vertical:Controls.ScrollBar{}
            delegate:UiButton {required property string stationId;required property string stationName;required property string stationCategory;required property bool stationSelected;required property bool stationFavorite
                objectName:"radioStation_"+stationId;width:496;height:85;selected:stationSelected;quiet:!stationSelected;enabled:AudioOutputs.available&&!PhoneAudio.active
                contentItem:Item {
                    Rectangle {x:0;y:3;width:52;height:52;radius:14;color:Theme.active;Label{anchors.centerIn:parent;text:stationName.charAt(0);font.pixelSize:22;color:Theme.green}}
                    Label {x:68;y:1;width:335;text:stationName;textFormat:Text.PlainText;font.pixelSize:22;elide:Text.ElideRight}
                    Label {x:68;y:36;width:335;text:stationCategory+(stationFavorite?" · 已收藏":"");textFormat:Text.PlainText;font.pixelSize:16;color:Theme.muted;elide:Text.ElideRight}
                    Icon {x:431;y:18;width:24;height:24;name:stationId===Media.itemId&&page.selectedSession&&Media.playing?"stop":"play";tint:Theme.green}
                }
                onClicked:stationId===Radio.currentId&&Radio.active?Radio.stop():Radio.tune(stationId)
            }
            Label {anchors.centerIn:parent;visible:Radio.count===0;text:Radio.filter==="favorites"?"还没有收藏电台":Radio.filter==="recent"?"还没有收听记录":"没有匹配的电台";font.pixelSize:20;color:Theme.muted}
        }
        Label {x:30;y:498;text:"共 "+Radio.count+" 个电台";font.pixelSize:15;color:Theme.muted}
    }
}
