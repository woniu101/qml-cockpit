import QtQuick
Item {
    id: map
    property real zoom:1
    property real panX:0
    property real panY:0
    property bool overview:false
    function recenter(){panX=0;panY=0;zoom=1;overview=false}
    Item {
        anchors.fill:parent
        transform:[Translate{x:map.panX;y:map.panY},Scale{origin.x:900;origin.y:450;xScale:map.zoom;yScale:map.zoom}]
        Canvas {
            id:roads;anchors.fill:parent;renderStrategy:Canvas.Cooperative
            Connections {target:Vehicle;function onChanged(){roads.requestPaint()}}
            onPaint:{
                const c=getContext("2d");c.reset()
                c.fillStyle=Vehicle.night?"#26362b":"#eaeee7";c.fillRect(0,0,width,height)
                c.fillStyle=Vehicle.night?"#304c4b":"#ccdfdd"
                c.beginPath();c.moveTo(1406,65);c.bezierCurveTo(1308,179,1491,230,1370,337);c.bezierCurveTo(1310,420,1384,538,1339,639);c.bezierCurveTo(1310,714,1336,735,1269,791);c.lineTo(1600,791);c.lineTo(1600,65);c.fill()
                c.fillStyle=Vehicle.night?"#334a31":"#dde7d6"
                ;[[552,112,173,104],[1000,458,232,132],[638,496,136,115],[1090,94,185,87],[475,637,270,83],[1003,651,235,105]].forEach(r=>{c.beginPath();c.roundedRect(r[0],r[1],r[2],r[3],18,18);c.fill()})
                c.fillStyle=Vehicle.night?"#2c3d2f":"#e2e7de"
                ;[[792,108,197,90],[514,276,107,83],[745,275,170,75],[1022,305,208,97],[502,478,104,118],[789,660,71,98]].forEach(r=>c.fillRect(r[0],r[1],r[2],r[3]))
                const lines=[[458,243,1303,243],[457,425,1352,425],[447,661,1318,661],[678,65,678,790],[899,65,899,790],[1262,66,1262,790],[457,154,1020,154],[484,335,1317,335],[468,542,1320,542],[481,723,1290,723],[792,70,792,780],[1098,65,1098,790],[449,93,1320,673]]
                lines.forEach((l,i)=>{c.beginPath();c.moveTo(l[0],l[1]);c.lineTo(l[2],l[3]);c.lineWidth=i<6?23:14;c.strokeStyle=Vehicle.night?"#384a37":"#d9e0d5";c.stroke();c.lineWidth=i<6?20:11;c.strokeStyle=Vehicle.night?"#435442":"#fcfcf7";c.stroke()})
                c.fillStyle=Vehicle.night?"#91a28b":"#95a18f";c.font='17px "Microsoft YaHei UI"'
                ;[[965,232,"云杉路"],[1094,413,"望江路"],[1035,650,"栖霞路"],[525,536,"云杉公园"],[1043,529,"滨江绿地"],[1408,555,"浦 江"]].forEach(t=>c.fillText(t[2],t[0],t[1]))
                if(Vehicle.navigationActive||Vehicle.routeProgress>0){
                    const points=Vehicle.routePoints
                    c.beginPath();points.forEach((p,i)=>{if(i===0)c.moveTo(p.x,p.y);else c.lineTo(p.x,p.y)})
                    c.lineCap="round";c.lineJoin="round";c.lineWidth=22;c.strokeStyle="#ffffff";c.stroke();c.lineWidth=13;c.strokeStyle="#577e68";c.stroke()
                    const end=points[points.length-1];c.fillStyle="#ffffff";c.beginPath();c.arc(end.x,end.y,19,0,Math.PI*2);c.fill();c.fillStyle="#577e68";c.beginPath();c.arc(end.x,end.y,12,0,Math.PI*2);c.fill()
                }
            }
        }
        Rectangle {
            objectName:"routeMarker";x:Vehicle.routeMarker.x-42;y:Vehicle.routeMarker.y-42;width:84;height:84;radius:42;color:"#23709c7d"
            Behavior on x {NumberAnimation{duration:850}}
            Behavior on y {NumberAnimation{duration:850}}
            Rectangle {anchors.centerIn:parent;width:58;height:58;radius:29;color:"white";Icon{anchors.centerIn:parent;name:"nav";width:41;height:41;tint:"#426c55";rotation:Vehicle.routeHeading}}
        }
        Surface {
            visible:Vehicle.navigationActive;x:1041;y:279;width:245;height:42;radius:16
            Label {anchors.centerIn:parent;text:Vehicle.destination;font.pixelSize:20;width:220;elide:Text.ElideRight;horizontalAlignment:Text.AlignHCenter}
        }
    }
    MouseArea {
        anchors.fill:parent;property point last
        onPressed:mouse=>last=Qt.point(mouse.x,mouse.y)
        onPositionChanged:mouse=>{if(pressed){map.panX+=mouse.x-last.x;map.panY+=mouse.y-last.y;last=Qt.point(mouse.x,mouse.y)}}
        onWheel:wheel=>map.zoom=Math.max(0.7,Math.min(1.8,map.zoom+wheel.angleDelta.y/1200))
    }
    PinchHandler {target:null;property real initialZoom:1;onActiveChanged:if(active)initialZoom=map.zoom;onActiveScaleChanged:if(active)map.zoom=Math.max(0.7,Math.min(1.8,initialZoom*activeScale))}
}
