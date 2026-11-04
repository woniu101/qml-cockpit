import QtQuick
Item {
    id:page
    required property var shell
    width:1600;height:Media.fullscreen?900:790
    property int view:AppRoutes.Music
    property string lastKind:Media.kind
    property bool receiverWasActive:PhoneAudio.active
    function openSource(kind){view=kind==="radio"?AppRoutes.Radio:kind==="video"?AppRoutes.Video:kind==="phone"?AppRoutes.Phone:AppRoutes.Music;if(view===AppRoutes.Video)video.showPlayer=Media.kind==="video"&&Media.itemId!==""}
    function selectView(value){if(view===AppRoutes.Video&&value!==view)Media.videoPageLeft();view=value;if(value===AppRoutes.Music&&PhoneAudio.active)PhoneAudio.disconnectDevice()}
    onVisibleChanged:if(!visible)Media.videoPageLeft()
    Component.onCompleted:if(PhoneAudio.active)view=AppRoutes.Phone
    Connections {target:PhoneAudio;function onChanged(){if(PhoneAudio.active)page.view=AppRoutes.Phone;else if(page.receiverWasActive)page.view=AppRoutes.Music;page.receiverWasActive=PhoneAudio.active}}
    Connections {target:Media;function onChanged(){if(page.lastKind!==Media.kind){page.lastKind=Media.kind;page.openSource(Media.kind)}}}
    MusicPane {shell:page.shell;visible:page.view===AppRoutes.Music}
    PhoneMusicPane {shell:page.shell;visible:page.view===AppRoutes.Phone}
    RadioPage {shell:page.shell;visible:page.view===AppRoutes.Radio}
    VideoPage {id:video;shell:page.shell;visible:page.view===AppRoutes.Video}
    Item {
        visible:!Media.fullscreen
        Label {x:56;y:95;text:"媒体";font.pixelSize:34}
        Row {x:207;y:94;spacing:6
            UiButton {objectName:"musicLocalSource";width:118;height:48;text:"音乐";textSize:20;selected:page.view===AppRoutes.Music;onClicked:page.selectView(AppRoutes.Music)}
            UiButton {objectName:"mediaRadioTab";width:118;height:48;text:"电台";textSize:20;selected:page.view===AppRoutes.Radio;onClicked:page.selectView(AppRoutes.Radio)}
            UiButton {objectName:"mediaVideoTab";width:118;height:48;text:"视频";textSize:20;selected:page.view===AppRoutes.Video;onClicked:page.selectView(AppRoutes.Video)}
            UiButton {objectName:"musicPhoneAudio";width:146;height:48;text:"手机蓝牙";textSize:20;selected:page.view===AppRoutes.Phone;onClicked:page.selectView(AppRoutes.Phone)}
        }
        UiButton {objectName:page.view===AppRoutes.Video?"videoFoldersButton":"musicAudioOutputs";x:1325;y:95;width:220;height:48;text:page.view===AppRoutes.Video?"目录管理":page.view===AppRoutes.Phone?"系统声音设置":"音频输出";iconName:page.view===AppRoutes.Video?"folder":"volume";textSize:18;onClicked:{if(page.view===AppRoutes.Video)page.shell.openPopup("videoFolders");else if(page.view===AppRoutes.Phone)AudioOutputs.openSettings();else page.shell.openPopup("audio")}}
    }
}
