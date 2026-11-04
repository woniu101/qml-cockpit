import QtQuick
Item {
    id:page
    required property var shell
    width:1600;height:790
    MapView {id:map;anchors.fill:parent}
    Label {x:40;y:95;text:"导航";font.pixelSize:32;font.weight:Font.Medium}
    Surface {
        x:40;y:165;width:398;height:203;color:"#344f42"
        Icon {x:30;y:42;width:69;height:69;name:Vehicle.navigationActive?(Vehicle.turnType==="finish"?"pin":"arrow-left-turn"):"search";tint:"white";mirror:Vehicle.turnType==="right"}
        Label {x:129;y:26;text:Vehicle.navigationActive?(Vehicle.turnDistance>=1000?(Vehicle.turnDistance/1000).toFixed(1):Vehicle.turnDistance):"出发";font.pixelSize:55;color:"white";font.weight:Font.Medium}
        Label {x:277;y:56;text:Vehicle.navigationActive?(Vehicle.turnDistance>=1000?"公里":"米"):"";font.pixelSize:21;color:"#d6e2d5"}
        Label {x:131;y:102;text:Vehicle.navigationActive?(Vehicle.turnType==="finish"?"前方到达":Vehicle.turnType==="left"?"左转进入":"右转进入"):"选择一个目的地";font.pixelSize:20;color:"#c4d6c7"}
        Label {x:131;y:138;width:240;elide:Text.ElideRight;text:Vehicle.navigationActive?Vehicle.turnStreet:"规划本地演示路线";font.pixelSize:27;color:"white"}
        MouseArea {anchors.fill:parent;enabled:!Vehicle.navigationActive;onClicked:page.shell.openPopup("destination")}
    }
    Surface {
        x:40;y:385;width:398;height:160
        Label {x:28;y:4;text:Vehicle.navigationActive?Vehicle.remainingMinutes:"--";font.pixelSize:39}
        Label {x:90;y:26;text:"分钟";font.pixelSize:19;color:Theme.muted}
        Label {x:169;y:17;text:Vehicle.navigationActive?Vehicle.remainingKm.toFixed(1):"--";font.pixelSize:29}
        Label {x:253;y:27;text:"公里";font.pixelSize:18;color:Theme.muted}
        Label {x:28;y:71;text:Vehicle.navigationActive?"预计 "+page.shell.arrivalText+" 到达":"离线示意导航";font.pixelSize:20;color:Theme.muted}
        UiButton {objectName:"navigationDriving";x:18;y:106;width:362;height:42;text:Vehicle.parked?"驻车规划中 · 打开驾驶演示":Vehicle.gear+" 挡 · "+Vehicle.speed+" km/h · 调整车速";textSize:17;quiet:true;foreground:Theme.green;onClicked:page.shell.openPopup("driving")}
    }
    Surface {
        visible:Vehicle.navigationActive;x:665;y:165;width:469;height:84
        Repeater {model:5;Icon{required property int index;x:42+index*71;y:14;width:28;height:28;name:"arrow-up";tint:index<2?Theme.green:Theme.muted;rotation:Vehicle.turnType==="right"?22:Vehicle.turnType==="left"?-22:0}}
        Label {x:10;y:50;width:449;text:Vehicle.parked?"路线已规划，车辆保持驻车":Vehicle.laneHint;font.pixelSize:17;color:Theme.muted;horizontalAlignment:Text.AlignHCenter}
    }
    MediaCard {x:40;y:565;compact:true;shell:page.shell}
    UiButton {objectName:"endNavigation";x:40;y:736;width:145;height:40;iconName:Vehicle.navigationActive?"close":"search";text:Vehicle.navigationActive?"结束导航":"目的地";textSize:18;onClicked:{if(Vehicle.navigationActive)Vehicle.endNavigation();else page.shell.openPopup("destination")}}
    UiButton {x:199;y:736;width:145;height:40;text:"路线总览";textSize:18;iconName:"layers";onClicked:{map.overview=!map.overview;map.zoom=map.overview?0.82:1;map.panX=0;map.panY=0}}
    Repeater {
        model:[{y:170,icon:"layers"},{y:242,icon:"plus"},{y:314,icon:"minus"},{y:644,icon:"locate"}]
        UiButton {required property var modelData;required property int index;x:1487;y:modelData.y;width:65;height:60;iconName:modelData.icon;onClicked:{if(index===0)page.shell.notify("当前为离线示意地图");else if(index===1)map.zoom=Math.min(1.8,map.zoom+0.15);else if(index===2)map.zoom=Math.max(0.7,map.zoom-0.15);else map.recenter()}}
    }
    Surface {x:1379;y:708;width:173;height:65;radius:21
        Label{x:16;y:4;text:Vehicle.speed;font.pixelSize:38}
        Label{x:74;y:29;text:"km/h";font.pixelSize:15;color:Theme.muted}
        Label{x:129;y:17;text:Vehicle.gear;font.pixelSize:24;color:Theme.green}
    }
    Label{x:1230;y:758;text:"离线示意路线";font.pixelSize:14;color:Theme.muted}
}
