import QtQuick
Item {
    required property var shell
    width: 1600; height: 110
    Rectangle { anchors.fill: parent; color: Theme.dock }
    Rectangle { width: parent.width; height: 1; color: Theme.line }
    Repeater {
        model: [{ name: "home", label: "首页" }, { name: "nav", label: "导航" }, { name: "car", label: "车辆" }, { name: "music", label: "音乐" }]
        UiButton {
            required property var modelData; required property int index
            objectName: "tab" + index
            x: 30 + index * 83; y: 13; width: 69; height: 79; quiet: true; vertical: true; textSize: 17
            selected: shell.page === index; iconName: modelData.name; text: modelData.label
            onClicked: shell.showPage(index)
        }
    }
    Repeater { model: [383, 993, 1225]; Rectangle { required property int modelData; x: modelData; y: 28; width: 1; height: 54; color: Theme.line } }
    Label { x: 445; y: 18; width: 88; horizontalAlignment: Text.AlignHCenter; text: "主驾"; font.pixelSize: 16; color: Theme.muted }
    Label { x: 435; y: 40; width: 110; horizontalAlignment: Text.AlignHCenter; text: Vehicle.driverTemperature.toFixed(1) + "°"; font.pixelSize: 38
        MouseArea { anchors.fill: parent; onClicked: shell.openPopup("climate") }
    }
    UiButton { objectName: "driverMinus"; x: 395; y: 38; width: 46; height: 48; iconName: "chevron-left"; quiet: true; onClicked: Vehicle.driverTemperature -= 0.5 }
    UiButton { objectName: "driverPlus"; x: 538; y: 38; width: 46; height: 48; iconName: "chevron-right"; quiet: true; onClicked: Vehicle.driverTemperature += 0.5 }
    UiButton { objectName: "climateButton"; x: 605; y: 20; width: 70; height: 76; quiet: true; vertical: true; iconName: "fan"; text: Vehicle.automatic ? "AUTO" : "风量 " + Vehicle.fan; textSize: 15; foreground: Theme.green; onClicked: shell.openPopup("climate") }
    UiButton { objectName: "acButton"; x: 698; y: 20; width: 70; height: 76; quiet: true; vertical: true; iconName: "snow"; text: "A/C"; textSize: 15; foreground: Vehicle.acOn ? Theme.green : Theme.muted; onClicked: Vehicle.acOn = !Vehicle.acOn }
    UiButton { x: 792; y: 20; width: 70; height: 76; quiet: true; vertical: true; iconName: "defrost"; text: "除雾"; textSize: 15; selected: Vehicle.defrost; onClicked: Vehicle.defrost = !Vehicle.defrost }
    UiButton { x: 885; y: 20; width: 70; height: 76; quiet: true; vertical: true; iconName: "seat"; text: "座椅"; textSize: 15; selected: Vehicle.seatHeat; onClicked: Vehicle.seatHeat = !Vehicle.seatHeat }
    Label { x: 1060; y: 18; width: 88; horizontalAlignment: Text.AlignHCenter; text: "副驾"; font.pixelSize: 16; color: Theme.muted }
    Label { x: 1050; y: 40; width: 110; horizontalAlignment: Text.AlignHCenter; text: Vehicle.passengerTemperature.toFixed(1) + "°"; font.pixelSize: 38
        MouseArea { anchors.fill: parent; onClicked: shell.openPopup("climate") }
    }
    UiButton { x: 1009; y: 38; width: 46; height: 48; iconName: "chevron-left"; quiet: true; onClicked: Vehicle.passengerTemperature -= 0.5 }
    UiButton { x: 1155; y: 38; width: 46; height: 48; iconName: "chevron-right"; quiet: true; onClicked: Vehicle.passengerTemperature += 0.5 }
    UiButton { objectName:"audioOutputShortcut"; x:1265; y:27; width:44; height:60; quiet:true; iconName:PhoneAudio.active?"bluetooth":"volume"; foreground:Theme.muted; caption:PhoneAudio.active?"手机音乐连接与音量说明":"本地音乐输出与音量"; onClicked:shell.openPopup(PhoneAudio.active?"bluetooth":"audio") }
    SoftSlider { objectName: "volumeSlider"; x: 1312; y: 35; width: 129; height: 44;visible:!PhoneAudio.active; from: 0; to: 100; value: Vehicle.volume; enabled:AudioOutputs.available; onMoved: Vehicle.volume = value }
    Label {x:1312;y:47;width:129;text:"手机调音量";visible:PhoneAudio.active;font.pixelSize:16;color:Theme.muted;horizontalAlignment:Text.AlignHCenter}
    UiButton { x: 1480; y: 27; width: 60; height: 60; iconName: "apps"; quiet: true; caption: "全部应用"; onClicked: shell.openPopup("apps") }
}
