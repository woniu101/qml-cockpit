import QtQuick
Item {
    id: controls
    required property var shell
    Label { x:30;y:27;text:"自动演示";font.pixelSize:28 }
    Label { x:30;y:100;width:520;text:"依次展示驻车、音乐、路线规划、驾驶、门窗、备厢和补能。";font.pixelSize:22;wrapMode:Text.WordWrap }
    Label { x:30;y:202;width:520;text:"手动操作会暂停演示；退出后恢复进入前的车辆与空调状态。";font.pixelSize:18;color:Theme.muted;wrapMode:Text.WordWrap }
    UiButton {objectName:"autoDemoButton";x:30;y:294;width:520;height:62;text:controls.shell.demoRunning?"退出并恢复原状态":"开始自动演示";primary:true;iconName:"play";onClicked:controls.shell.toggleDemo() }
    Row {x:30;y:379;spacing:14
        UiButton {objectName:"demoPause";width:253;height:54;text:controls.shell.demoPaused?"继续":"暂停";enabled:controls.shell.demoRunning;onClicked:controls.shell.demoPause()}
        UiButton {objectName:"demoNext";width:253;height:54;text:"单步：下一步";onClicked:controls.shell.demoNext()}
    }
    Label {x:30;y:468;text:"演示速度";font.pixelSize:20}
    Row {x:30;y:508;spacing:12;Repeater {model:[0.5,1,2];UiButton {required property real modelData;width:165;height:49;text:modelData+"×";onClicked:controls.shell.demoSpeed(modelData)}}}
}
