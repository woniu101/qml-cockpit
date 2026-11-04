import QtQuick
import QtQuick.Controls as Controls

Item {
    Label { x:30; y:25; text:"天气与城市"; font.pixelSize:28 }
    Icon {
        x:30; y:83; width:24; height:24
        name:Weather.icon; tint:Theme.green
        visible:Weather.city !== ""
    }
    Label {
        x:Weather.city === "" ? 30 : 66; y:80; width:550-x
        text:Weather.city === "" ? "选择要显示天气的城市"
             : Weather.temperature === "" ? Weather.city : Weather.city + "  " + Weather.temperature
        textFormat:Text.PlainText; font.pixelSize:24; elide:Text.ElideRight
    }
    Icon {
        x:30; y:118; width:16; height:16; name:"refresh"; tint:Theme.green
        visible:Weather.refreshing
        RotationAnimation on rotation { from:0; to:360; duration:1000; loops:Animation.Infinite; running:Weather.refreshing }
    }
    Label {
        x:Weather.refreshing ? 54 : 30; y:116; width:550-x
        text:Weather.city === "" ? "可搜索城市，或按网络位置查找" : Weather.status
        color:Theme.muted; font.pixelSize:16; elide:Text.ElideRight
    }
    Controls.TextField {
        id:citySearch
        objectName:"weatherCitySearch"
        x:30; y:159; width:404; height:46
        placeholderText:"输入城市名称或拼音"; placeholderTextColor:Theme.muted
        font.pixelSize:18; color:Theme.ink; maximumLength:80
        selectByMouse:true
        background:Rectangle { radius:15; color:Theme.secondary }
        onAccepted:Weather.search(text)
    }
    UiButton {
        x:444; y:159; width:106; height:46
        text:"搜索"; textSize:18; selected:true
        enabled:!Weather.searching && citySearch.text.trim().length >= 2
        onClicked:Weather.search(citySearch.text)
    }
    Rectangle {
        x:30; y:221; width:520; height:66; radius:16; color:Theme.secondary
        Icon { x:14; anchors.verticalCenter:parent.verticalCenter; width:23; height:23; name:"nav"; tint:Theme.green }
        Label {
            x:48; y:10; width:Weather.suggestedCity === "" ? 344 : 302
            font.pixelSize:17; elide:Text.ElideRight
            text:Weather.suggestedCity || "网络位置"
            textFormat:Text.PlainText
        }
        Label {
            x:48; y:35; width:302; font.pixelSize:14; color:Theme.muted; elide:Text.ElideRight
            text:Weather.locating ? "正在查找网络所在城市…"
                 : Weather.locationFailed ? "定位暂不可用，可手动搜索城市"
                 : Weather.suggestedCity !== "" ? "IP 城市候选 · 可能受网络出口影响"
                 : "按公网 IP 查找大致城市"
        }
        UiButton {
            x:352; y:11; width:44; height:44; quiet:true
            iconName:"refresh"; caption:"重新查找网络所在城市"
            visible:Weather.suggestedCity !== ""
            onClicked:Weather.locateByIp()
        }
        UiButton {
            objectName:"weatherIpLocation"
            x:400; y:11; width:110; height:44; textSize:16; selected:true
            text:Weather.locating ? "定位中" : Weather.suggestedCity !== "" ? "使用此城市"
                 : Weather.locationFailed ? "重试" : "查找城市"
            enabled:!Weather.locating
            onClicked: {
                if (Weather.suggestedCity !== "") Weather.selectSuggestedCity()
                else Weather.locateByIp()
            }
        }
    }
    Label { x:30; y:303; text:Weather.searchStatus; color:Theme.muted; font.pixelSize:16 }
    ListView {
        x:30; y:332; width:520; height:204
        model:Weather.matches; spacing:6; clip:true
        Controls.ScrollBar.vertical:Controls.ScrollBar {}
        delegate:UiButton {
            required property var modelData
            required property int index
            width:500; height:48; textSize:18
            contentItem:Label {
                text:modelData.label; textFormat:Text.PlainText
                verticalAlignment:Text.AlignVCenter; elide:Text.ElideRight
                leftPadding:10; font.pixelSize:18
            }
            Accessible.name:modelData.label
            onClicked:Weather.selectCity(index)
        }
    }
    Label { x:30; y:562; text:"天气 Open-Meteo · IP 定位 IPWHOIS"; color:Theme.muted; font.pixelSize:15 }
    UiButton {
        x:408; y:546; width:142; height:44
        text:Weather.refreshing ? "更新中" : "刷新天气"; textSize:17
        enabled:Weather.city !== "" && !Weather.refreshing
        onClicked:Weather.refresh()
    }
}
