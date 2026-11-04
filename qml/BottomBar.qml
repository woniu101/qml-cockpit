import QtQuick

Item {
    id: bar
    required property var shell
    width: 1600; height: 110
    Rectangle { anchors.fill: parent; color: Theme.dock }
    Rectangle { width: parent.width; height: 1; color: Theme.line }

    Item {
        id: navigation
        anchors.left: parent.left
        width: 370; height: parent.height

        Row {
            anchors.centerIn: parent
            spacing: 13
            Repeater {
                model: [{ name: "home", label: "首页" }, { name: "nav", label: "导航" },
                        { name: "car", label: "车辆" }, { name: "music", label: "媒体" }]
                UiButton {
                    required property var modelData
                    required property int index
                    objectName: "tab" + index
                    width: 70; height: 78
                    quiet: true; vertical: true; textSize: 17
                    selected: bar.shell.page === index
                    iconName: modelData.name; text: modelData.label
                    onClicked: bar.shell.showPage(index)
                }
            }
        }
    }

    Item {
        id: climate
        anchors.left: navigation.right; anchors.right: utilities.left
        height: parent.height

        Row {
            anchors.centerIn: parent
            spacing: 40

            TemperatureControl {
                zoneId: "driver"; title: "主驾"; temperature: Vehicle.driverTemperature
                onAdjust: delta => Vehicle.driverTemperature += delta
                onOpenClimate: bar.shell.openPopup("climate")
            }

            Row {
                spacing: 16
                UiButton {
                    objectName: "climateButton"
                    width: 72; height: 76; quiet: true; vertical: true
                    iconName: "fan"; text: Vehicle.automatic ? "AUTO" : "风量 " + Vehicle.fan
                    textSize: 15; foreground: Theme.green
                    onClicked: bar.shell.openPopup("climate")
                }
                UiButton {
                    objectName: "acButton"
                    width: 72; height: 76; quiet: true; vertical: true
                    iconName: "snow"; text: "A/C"; textSize: 15
                    foreground: Vehicle.acOn ? Theme.green : Theme.muted
                    onClicked: Vehicle.acOn = !Vehicle.acOn
                }
                UiButton {
                    width: 72; height: 76; quiet: true; vertical: true
                    iconName: "defrost"; text: "除雾"; textSize: 15; selected: Vehicle.defrost
                    onClicked: Vehicle.defrost = !Vehicle.defrost
                }
                UiButton {
                    width: 72; height: 76; quiet: true; vertical: true
                    iconName: "seat"; text: "座椅"; textSize: 15; selected: Vehicle.seatHeat
                    onClicked: Vehicle.seatHeat = !Vehicle.seatHeat
                }
            }

            TemperatureControl {
                zoneId: "passenger"; title: "副驾"; temperature: Vehicle.passengerTemperature
                onAdjust: delta => Vehicle.passengerTemperature += delta
                onOpenClimate: bar.shell.openPopup("climate")
            }
        }
    }

    Item {
        id: utilities
        anchors.right: parent.right
        width: 376; height: parent.height

        Row {
            anchors.centerIn: parent
            spacing: 22

            Rectangle {
                width: 222; height: 54
                anchors.verticalCenter: parent.verticalCenter
                radius: height / 2; color: Theme.secondary

                UiButton {
                    objectName: "audioOutputShortcut"
                    x: 4; width: 48; height: parent.height; quiet: true
                    iconName: PhoneAudio.active ? "bluetooth" : "volume"
                    foreground: Theme.muted
                    caption: PhoneAudio.active ? "手机音乐连接与音量说明" : "媒体音频输出与音量"
                    onClicked: bar.shell.openPopup(PhoneAudio.active ? "bluetooth" : "audio")
                }
                SoftSlider {
                    id: volumeSlider
                    objectName: "volumeSlider"
                    x: 55; width: 156; height: 44
                    anchors.verticalCenter: parent.verticalCenter
                    leftPadding: 6; rightPadding: 6
                    visible: !PhoneAudio.active; enabled: AudioOutputs.available
                    from: 0; to: 100; value: MediaState.volume
                    Accessible.name: "媒体音量"
                    onMoved: MediaState.volume = Math.round(value)
                }
                Label {
                    x: volumeSlider.x; width: volumeSlider.width
                    anchors.verticalCenter: parent.verticalCenter
                    text: "手机调音量"; visible: PhoneAudio.active
                    font.pixelSize: 16; color: Theme.muted; horizontalAlignment: Text.AlignHCenter
                }
            }

            UiButton {
                objectName: "assistantShortcut"
                width: 76; height: 78
                text: "助手"; quiet: true; selected: bar.shell.popup === "assistant"
                contentItem: Item {
                    Column {
                        anchors.centerIn: parent
                        spacing: 3
                        AssistantAvatar {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 32; height: 32
                            active: AssistantSpeech.recording || AssistantVoice.playing
                        }
                        Label { text: "助手"; font.pixelSize: 17; color: Theme.green }
                    }
                }
                onClicked: {
                    if (bar.shell.popup === "assistant") {
                        bar.shell.popup = ""
                    } else {
                        if (!AssistantNlu.busy && !AssistantVoice.busy && !AssistantVoice.playing)
                            Assistant.clearFeedback()
                        bar.shell.openPopup("assistant")
                    }
                }
            }
        }
    }

    Repeater {
        model: [navigation.width, utilities.x]
        Rectangle {
            required property int modelData
            x: modelData; width: 1; height: 54
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.line
        }
    }

    component TemperatureControl: Item {
        id: zone
        required property string zoneId
        required property string title
        required property real temperature
        signal adjust(real delta)
        signal openClimate()
        width: 194; height: 76

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 3; text: zone.title; font.pixelSize: 16; color: Theme.muted
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 25; width: 102; height: 48
            text: zone.temperature.toFixed(1) + "°"; font.pixelSize: 38
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            MouseArea { anchors.fill: parent; onClicked: zone.openClimate() }
        }
        UiButton {
            objectName: zone.zoneId + "Minus"
            anchors.left: parent.left
            y: 25; width: 46; height: 48; iconName: "chevron-left"; quiet: true
            onClicked: zone.adjust(-0.5)
        }
        UiButton {
            objectName: zone.zoneId + "Plus"
            anchors.right: parent.right
            y: 25; width: 46; height: 48; iconName: "chevron-right"; quiet: true
            onClicked: zone.adjust(0.5)
        }
    }
}
