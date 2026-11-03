import QtQuick
import QtQuick.Controls
import "." as App
Item {
    id: settings
    required property var shell
    App.Label { objectName: "settingsTitle"; x: 30; y: 27; text: "显示与偏好"; font.pixelSize: 28 }
    Flickable {
        x: 30; y: 99; width: 520; height: 425; clip: true; contentHeight: content.height; boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {policy:ScrollBar.AsNeeded}
        Column {
            id: content; width: 520; spacing: 24
            App.Label { text: "外观主题"; font.pixelSize: 22 }
            Row { spacing: 12; Repeater { model: [{key:"day",title:"日间"},{key:"night",title:"夜间"},{key:"auto",title:"自动"}]; UiButton { required property var modelData; objectName:"theme_"+modelData.key; width:165;height:51;text:modelData.title;selected:Vehicle.themeMode===modelData.key;onClicked:Vehicle.themeMode=modelData.key } } }
            App.Label { width: parent.width; text: "自动主题在 18:00—06:00 使用夜间外观。"; font.pixelSize: 16; color: Theme.muted }
            UiButton { objectName:"autoFocusSetting"; width:parent.width;height:56;text:Vehicle.autoFocus?"部件自动定位：开启":"部件自动定位：关闭";selected:Vehicle.autoFocus;onClicked:Vehicle.autoFocus=!Vehicle.autoFocus }
            App.Label { text: "3D 画质"; font.pixelSize: 22 }
            Row { spacing: 14; Repeater {model:[{key:"standard",title:"标准"},{key:"high",title:"高画质"}];UiButton {required property var modelData;objectName:"quality_"+modelData.key;width:253;height:51;text:modelData.title;selected:Vehicle.quality===modelData.key;onClicked:Vehicle.quality=modelData.key} } }
            UiButton { width:parent.width;height:56;text:"驾驶演示与车速";iconName:"car";onClicked:settings.shell.openPopup("driving") }
            UiButton { width:parent.width;height:56;text:"车辆数据来源";iconName:"layers";onClicked:settings.shell.openPopup("source") }
            UiButton { width:parent.width;height:56;text:"自动演示控制";iconName:"play";onClicked:settings.shell.openPopup("demo") }
            UiButton { width:parent.width;height:56;text:"全屏 / 窗口";onClicked:{settings.shell.toggleFullScreen();settings.shell.popup=""} }
            UiButton {objectName:"resetButton";width:parent.width;height:56;text:"恢复初始状态";iconName:"refresh";onClicked:{if(settings.shell.demoRunning)settings.shell.toggleDemo();Vehicle.reset();settings.shell.showPage(0)} }
        }
    }
    App.Label { x:30;y:548;width:520;text:"温度、音量与主题等偏好自动保存；门窗运行状态不保存。";font.pixelSize:15;color:Theme.muted;wrapMode:Text.WordWrap }
    App.Label { x:30;y:581;width:520;text:"3D：Ameer Studio · Model 3 · CC BY 4.0";font.pixelSize:12;color:Theme.muted }
}
