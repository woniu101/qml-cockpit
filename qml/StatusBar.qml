import QtQuick
Item {
    required property var shell
    width: 1600; height: 64
    Item {
        objectName: "statusLeftGroup"; x: 40; width: 302; height: 64
        // Preserve the shared alignment box; padding shifts the glyphs 24px right and 2px down.
        Label { objectName: "statusClock"; width: 112; height: 64; leftPadding: 24; topPadding: 4; verticalAlignment: Text.AlignVCenter; text: shell.clockText; font.pixelSize: 26; font.weight: Font.Normal; font.features: {"tnum": 1}; color: Theme.muted }
        Rectangle { x: 128; y: 22; width: 1; height: 20; color: Theme.line }
        Icon { x: 150; y: 21; width: 22; height: 22; name: Vehicle.night?"moon":"sun"; tint: Theme.green }
        Label { x: 184; width: 46; height: 64; verticalAlignment: Text.AlignVCenter; text: "24°"; font.pixelSize: 20; color: Theme.muted }
        Label { x: 244; width: 58; height: 64; verticalAlignment: Text.AlignVCenter; text: "上海"; font.pixelSize: 20; color: Theme.muted }
    }
    UiButton {
        objectName: "simulatorButton"; x: 370; y: 10; width: 174; height: 44
        text: Vehicle.gear+" · "+(Vehicle.parked?"驻车":Vehicle.speed+" km/h"); selected: true; iconName: "car"; textSize: 17
        caption: "驾驶演示与车速"; onClicked: shell.openPopup("driving")
    }
    UiButton { objectName: "sourceButton"; x: 568; y: 10; width: 250; height: 44; visible: !shell.demoRunning; quiet: true; iconName: "layers"; text: Vehicle.externalMode?(Vehicle.externalReady?"联调 · 已连接":"联调 · 等待数据"):Vehicle.systemMode?(Vehicle.systemFallback?"电脑映射 · 模拟电量":Vehicle.energyReady?"电脑映射 · 实际电量":"电脑映射 · 等待电源"):"数据源 · 内置演示"; textSize: 16; foreground: Vehicle.energyReady?Theme.green:Theme.muted; onClicked: shell.openPopup("source") }
    UiButton {
        objectName: "statusEnergyButton"; x: 1136; y: 10; width: 132; height: 44; quiet: true
        caption: EnergyVisual.wholeSocText+" · "+EnergyVisual.stateText+" · 查看补能"
        Accessible.name: "电量 "+EnergyVisual.wholeSocText+"，"+EnergyVisual.stateText+"，查看补能详情"
        contentItem: Item {}
        onClicked: shell.inspectPart("port")
        Rectangle { anchors.fill: parent; radius: 18; color: "transparent"; border.width: 1; border.color: Theme.green; visible: parent.visualFocus }
    }
    Icon { objectName: "statusEnergyIndicator"; x: 1144; y: 24; width: 14; height: 16; visible: !Vehicle.energyReady || EnergyVisual.flow || Vehicle.plugged; name: !Vehicle.energyReady?"refresh":EnergyVisual.flow?"bolt":"plug"; tint: Vehicle.energyReady?EnergyVisual.stateColor:Theme.muted }
    Label { objectName: "statusBattery"; x: 1156; width: 52; height: 64; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: EnergyVisual.wholeSocText; font.pixelSize: 19; color: EnergyVisual.soc<=20?EnergyVisual.readingColor:Theme.muted }
    Rectangle { x: 1220; y: 24; width: 36; height: 16; radius: 4; color: "transparent"; border.color: EnergyVisual.soc<=20?EnergyVisual.readingColor:Theme.muted
        Rectangle { x: 3; y: 3; width: 30*EnergyVisual.soc/100; height: 10; radius: 2; color: EnergyVisual.readingColor }
        Label { anchors.centerIn: parent; visible: !Vehicle.batteryKnown; text: "?"; font.pixelSize: 12; color: Theme.muted }
        Rectangle { x: 39; y: 5; width: 3; height: 6; radius: 1; color: Theme.muted }
    }
    UiButton { objectName:"bluetoothStatusButton"; x:1276; y:10; width:44; height:44; quiet:true; iconName:"bluetooth"; caption:BluetoothState.status.text; foreground:BluetoothState.status.state==="on"?Theme.green:Theme.muted; onClicked:shell.openPopup("bluetooth")
        Rectangle { x:32; y:8; width:6; height:6; radius:3; color:Theme.green; visible:BluetoothState.status.state==="on"&&BluetoothState.status.connectedCount>0 }
    }
    UiButton { objectName: "networkStatusButton"; x: 1328; y: 10; width: 96; height: 44; quiet: true; iconName: Host.network.icon; text: Host.network.label; textSize: 14; foreground: Host.network.reachability==="online"?Theme.green:Theme.muted; caption: Host.network.summary; onClicked: shell.openPopup("system") }
    UiButton {
        objectName: "notificationsButton"; x: 1432; y: 10; width: 44; height: 44; quiet: true; iconName: "bell"; caption: "当前车辆提醒"; onClicked: shell.openPopup("notifications")
        Rectangle { visible: Vehicle.notifications.length>0; x: 29; y: -1; width: 22; height: 19; radius: 9.5; color: Vehicle.warningCount>0?"#ac7638":"#557a65"
            Label { anchors.centerIn: parent; text: Vehicle.notifications.length>9?"9+":Vehicle.notifications.length; font.pixelSize: 12; color: "white" }
        }
    }
    Rectangle { x: 1493; y: 22; width: 1; height: 20; color: Theme.line }
    UiButton {
        id: avatarButton
        objectName: "settingsButton"; x: 1512; y: 10; width: 44; height: 44
        text: "林"; caption: "林先生 · 显示与偏好设置"
        Accessible.name: "林先生的显示与偏好设置"
        onClicked: shell.openPopup("settings")
        background: Item {
            Rectangle {
                anchors.centerIn: parent; width: 36; height: 36; radius: 18; color: Theme.surface
                border.width: avatarButton.visualFocus || shell.popup==="settings"?2:1
                border.color: avatarButton.hovered || avatarButton.visualFocus || shell.popup==="settings"?Theme.green:Vehicle.night?"#6f8978":"#b8cabc"
                Rectangle { anchors.fill: parent; radius: parent.radius; color: Theme.green; opacity: avatarButton.down?0.16:avatarButton.hovered?0.08:0 }
            }
            Rectangle { anchors.fill: parent; radius: width/2; color: "transparent"; border.width: 2; border.color: Theme.green; visible: avatarButton.visualFocus }
        }
        contentItem: Item {
            Label { anchors.centerIn: parent; text: avatarButton.text; color: Theme.green; font.pixelSize: 18; font.weight: Font.Normal }
        }
    }
}
