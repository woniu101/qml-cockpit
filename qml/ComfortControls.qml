import QtQuick
Item {
    id: comfort
    required property var page
    readonly property int section: page.section
    Label { x: 24; y: 22; text: comfort.section===2?"座椅与空调":comfort.section===3?"外部灯光":"后视镜"; font.pixelSize: 26; font.weight: Font.Medium }
    Column {
        x: 24; y: 87; width: 347; spacing: 23; visible: comfort.section===2
        Label { text: "前排座椅加热"; font.pixelSize: 23 }
        Row { spacing: 9; Repeater { model: ["关闭","1 档","2 档","3 档"]; UiButton { required property string modelData; required property int index; width: 80; height: 49; text: modelData; textSize: 18; selected: Vehicle.heatLevel===index; onClicked: Vehicle.heatLevel=index } } }
        Label { text: "双区温度"; font.pixelSize: 23 }
        Label { text: "主驾 "+Vehicle.driverTemperature.toFixed(1)+"°  /  副驾 "+Vehicle.passengerTemperature.toFixed(1)+"°"; font.pixelSize: 21; color: Theme.muted }
        UiButton { width: parent.width; height: 51; text: Vehicle.climateSync?"关闭双区同步 SYNC":"开启双区同步 SYNC"; selected: Vehicle.climateSync; onClicked: Vehicle.climateSync=!Vehicle.climateSync }
        Label { text: "出风方向"; font.pixelSize: 23 }
        Row { spacing: 12; Repeater { model: [{key:"face",title:"面部"},{key:"feet",title:"脚部"},{key:"mixed",title:"面部＋脚部"}]; UiButton { required property var modelData; width: 107; height: 49; text: modelData.title; textSize: 17; selected: Vehicle.airDirection===modelData.key; onClicked: Vehicle.airDirection=modelData.key } } }
        UiButton { width: parent.width; height: 54; text: "调整温度与风量"; iconName: "fan"; onClicked: comfort.page.shell.openPopup("climate") }
    }
    Column {
        x: 24; y: 87; width: 347; spacing: 24; visible: comfort.section===3
        Label { text: "近光灯"; font.pixelSize: 23 }
        Row { spacing: 12; Repeater { model: [{key:"off",title:"关闭"},{key:"auto",title:"自动"},{key:"on",title:"开启"}]; UiButton { required property var modelData; width: 107; height: 52; text: modelData.title; textSize: 19; selected: Vehicle.lightMode===modelData.key; onClicked: Vehicle.lightMode=modelData.key } } }
        Label { width: parent.width; text: Vehicle.lightMode==="auto"?"自动模式随日夜主题切换近光灯。":"近光灯当前"+(Vehicle.lightsOn?"已开启":"已关闭"); font.pixelSize: 18; color: Theme.muted; wrapMode: Text.WordWrap }
        UiButton { objectName: "highBeam"; width: parent.width; height: 58; text: Vehicle.highBeam?"关闭远光灯":"开启远光灯"; iconName: "light"; selected: Vehicle.highBeam; onClicked: Vehicle.highBeam=!Vehicle.highBeam }
        UiButton { objectName: "fogLights"; width: parent.width; height: 58; text: Vehicle.fogLights?"关闭雾灯":"开启雾灯"; selected: Vehicle.fogLights; onClicked: Vehicle.fogLights=!Vehicle.fogLights }
        Label { width: parent.width; text: "观察车辆前后灯组的亮度变化"; font.pixelSize: 17; color: Theme.muted }
    }
    Column {
        x: 24; y: 87; width: 347; spacing: 25; visible: comfort.section===4
        Label { text: Vehicle.mirrorsFolded?"当前已折叠":"当前已展开"; font.pixelSize: 24 }
        Label { width: parent.width; text: "驻车时可以折叠后视镜，挂入 D 或 R 挡后自动展开。"; font.pixelSize: 19; color: Theme.muted; wrapMode: Text.WordWrap }
        UiButton { objectName: "foldMirrors"; width: parent.width; height: 65; text: Vehicle.mirrorsFolded?"展开后视镜":"折叠后视镜"; primary: true; iconName: "mirror"; enabled: Vehicle.parked; onClicked: Vehicle.toggleMirrors() }
        UiButton { width: parent.width; height: 52; text: "查看车辆左侧"; onClicked: comfort.page.lookAt(65) }
        UiButton { width: parent.width; height: 52; text: "查看车辆右侧"; onClicked: comfort.page.lookAt(-65) }
    }
}
