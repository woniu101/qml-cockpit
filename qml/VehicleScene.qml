import QtQuick
import QtQuick.Window
import QtQuick3D
import "../assets/sedan"

Item {
    id: scene
    clip: true
    property bool interactive: true
    property bool activeView: true
    property bool drivingMode: false
    property bool animationPaused: false
    property string energyDetail: "ambient"
    property point framingOffset: Qt.point(0,0)
    readonly property bool energyEffectsVisible: energyDetail==="detailed" && !interior && !drivingMode
    readonly property bool energyFlowRunning: EnergyVisual.flow && energyEffectsVisible && scene.ready && scene.activeView && scene.visible && !scene.animationPaused
    property real travelMeters: 0
    readonly property real wheelAngle: travelMeters/2.345*360
    readonly property real roadOffset: ((travelMeters*100)%300+300)%300
    FrameAnimation {
        running: Vehicle.energyReady && scene.drivingMode && scene.activeView && scene.visible && scene.ready && !scene.animationPaused && Vehicle.speed>0 && (Vehicle.gear==="D"||Vehicle.gear==="R")
        onTriggered: scene.travelMeters+=Vehicle.speed/3.6*frameTime*(Vehicle.gear==="R"?-1:1)
    }
    property int presentedFrames: 0
    property bool interior: false
    property real yaw: 43
    property real elevation: -15
    property real distance: 650
    property string selectedPart: ""
    property string selectedCaption: ""
    property bool renderFailed: false
    property bool rendererEnabled: true
    property real portAngle: Vehicle.portOpen?-85:0
    Behavior on portAngle { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: 600; easing.type: Easing.InOutCubic } }
    readonly property vector3d selectionPoint: {
        const change=scene.presentedFrames+scene.width+scene.height+scene.framingOffset.x+scene.framingOffset.y+scene.yaw+scene.elevation+scene.distance+car.doorLF+car.doorRF+car.doorLR+car.doorRR+car.hatch+car.hood
        return viewport.mapFrom3DScene(car.markerPosition(selectedPart))
    }
    readonly property bool selectedFacing: {
        const side=Math.sin(scene.yaw*Math.PI/180)
        if(selectedPart.indexOf("window")===0||selectedPart.indexOf("door")===0)return Number(selectedPart.slice(-1))%2===0?side>0.08:side< -0.08
        return selectedPart==="hatch"||selectedPart==="port"?Math.cos(scene.yaw*Math.PI/180)<0:true
    }
    property bool motionEnabled: true
    property real targetYaw: 43
    property real targetElevation: -15
    property real targetDistance: 650
    readonly property bool cameraMoving: focusAnimation.running
    readonly property bool ready: presentedFrames>=2 && vehicleAssetsAvailable && !renderFailed
    readonly property real doorLF: car.doorLF
    readonly property real hatch: car.hatch
    readonly property real windowLF: car.windowLF
    signal tapped()
    signal partTapped(string part)
    signal interacted()
    function partAt(px, py) {
        const hit=viewport.pick(px, py)
        return hit.objectHit ? hit.objectHit.objectName : ""
    }
    function moveView(angle,elevation=-15,distance=650) {
        focusAnimation.stop()
        targetYaw=yaw+((angle-yaw+540)%360+360)%360-180
        targetElevation=elevation;targetDistance=distance
        focusAnimation.restart()
    }
    function focusPart(part) {
        interior=false
        const index=Number(part.slice(-1))
        if(part.indexOf("door")===0 || part.indexOf("window")===0)moveView([65,-65,115,-115][index])
        else if(part==="hatch")moveView(145,-18,700)
        else if(part==="hood")moveView(30,-23,680)
        else if(part==="port")moveView(145,-16,650)
    }
    function resetView() { interior=false;moveView(43) }
    ParallelAnimation {
        id: focusAnimation
        NumberAnimation { target: scene; property: "yaw"; to: scene.targetYaw; duration: 450; easing.type: Easing.InOutCubic }
        NumberAnimation { target: scene; property: "elevation"; to: scene.targetElevation; duration: 450; easing.type: Easing.InOutCubic }
        NumberAnimation { target: scene; property: "distance"; to: scene.targetDistance; duration: 450; easing.type: Easing.InOutCubic }
    }
    Connections {
        target: scene.Window.window
        function onFrameSwapped(){if(scene.activeView && scene.presentedFrames<3)scene.presentedFrames++}
    }
    Connections {
        target: Vehicle
        function onVisualReset() {
            scene.travelMeters=0
            focusAnimation.stop();scene.yaw=43;scene.elevation=-15;scene.distance=650;scene.interior=false
            scene.motionEnabled=false
            Qt.callLater(function(){scene.motionEnabled=true})
        }
    }

    // Soft ground contact stays behind the transparent 3D surface.
    Image {
        x: parent.width*0.13+scene.framingOffset.x; y: parent.height*0.57+scene.framingOffset.y
        width: parent.width*0.74; height: parent.height*0.29
        source: "qrc:/ui/ground-shadow.svg"
        opacity: scene.interior || scene.drivingMode ? 0 : 0.65
    }
    View3D {
        id: viewport
        anchors.fill: parent
        visible: scene.activeView && scene.rendererEnabled && vehicleAssetsAvailable
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: Vehicle.quality==="high"?SceneEnvironment.High:SceneEnvironment.Medium
            aoEnabled: Vehicle.quality==="high"
            aoStrength: 0.65
            lightProbe: Texture { source: "../assets/studio.hdr" }
            probeExposure: 1.2
            tonemapMode: SceneEnvironment.TonemapModeAces
        }
        camera: scene.interior ? cabinCamera : (scene.framingOffset.x!==0 || scene.framingOffset.y!==0) ? framedCamera : exteriorCamera
        Node {
            y: 63
            eulerRotation: Qt.vector3d(scene.elevation, scene.yaw, 0)
            PerspectiveCamera { id: exteriorCamera; z: scene.distance; fieldOfView: 30; clipNear: 1; clipFar: 3000 }
            // Shift the projection without changing scale, orbit, picking or the viewport's clip bounds.
            FrustumCamera {
                id: framedCamera
                z: scene.distance; clipNear: exteriorCamera.clipNear; clipFar: exteriorCamera.clipFar
                readonly property real halfHeight: clipNear*Math.tan(exteriorCamera.fieldOfView*Math.PI/360)
                readonly property real halfWidth: halfHeight*scene.width/Math.max(1,scene.height)
                readonly property real centerX: -2*halfWidth*scene.framingOffset.x/Math.max(1,scene.width)
                readonly property real centerY: 2*halfHeight*scene.framingOffset.y/Math.max(1,scene.height)
                left: -halfWidth+centerX; right: halfWidth+centerX
                top: halfHeight+centerY; bottom: -halfHeight+centerY
            }
        }
        PerspectiveCamera { id: cabinCamera; position: Qt.vector3d(0, 133, -65); eulerRotation: Qt.vector3d(-15, 180, 0); fieldOfView: 85; clipNear: 0.5; clipFar: 1500 }
        DirectionalLight { eulerRotation: Qt.vector3d(-35,-35,0); brightness: 1.15; ambientColor: "#79867e"; castsShadow: true; shadowFactor: 55; shadowFilter: 15; shadowBias: 1.5; shadowMapQuality: Light.ShadowMapQualityHigh }
        DirectionalLight { eulerRotation: Qt.vector3d(-15,135,0); brightness: 0.6; color: "#edf3ff" }
        DirectionalLight { eulerRotation: Qt.vector3d(-75,30,0); brightness: 0.45; color: "#fff6e9" }
        Model {
            visible: !scene.drivingMode
            source: "#Rectangle"; y: 0.15; eulerRotation.x: -90; scale: Qt.vector3d(7.5,6,1); castsShadows: false
            materials: PrincipledMaterial {
                baseColor: Theme.background; roughness: 1; opacity: 0.6
                opacityMap: Texture { source: "qrc:/ui/ground-fade.svg" }
                opacityChannel: Material.A; alphaMode: PrincipledMaterial.Blend
            }
        }
        Node {
            visible: scene.drivingMode
            Model { source: "#Rectangle"; y: 0.1; eulerRotation.x: -90; scale: Qt.vector3d(11,40,1); receivesShadows: true
                materials: PrincipledMaterial { baseColor: Vehicle.night?"#202e25":"#48544d"; roughness: 1 }
            }
            Repeater3D { model: 2
                Model { required property int index; source: "#Rectangle"; x: index===0?-430:430; y: 0.3; eulerRotation.x: -90; scale: Qt.vector3d(0.04,40,1); castsShadows: false
                    materials: PrincipledMaterial { baseColor: "#e4e8da"; lighting: PrincipledMaterial.NoLighting }
                }
            }
            Repeater3D { model: 28
                Model { required property int index; source: "#Rectangle"; x: index%2===0?-205:205; y: 0.3; z: Math.floor(index/2)*300-1900-scene.roadOffset; eulerRotation.x: -90; scale: Qt.vector3d(0.045,1.45,1); castsShadows: false
                    materials: PrincipledMaterial { baseColor: "#ecf0e6"; lighting: PrincipledMaterial.NoLighting }
                }
            }
        }
        Node {
            id: portHinge; visible: Vehicle.portOpen || scene.portAngle< -1; position: Qt.vector3d(94,96,-216)
            eulerRotation.y: scene.portAngle
            Model { objectName: "chargePortFlap"; pickable: true; source: "#Cube"; z: -5; scale: Qt.vector3d(0.012,0.105,0.11); materials: PrincipledMaterial { baseColor: "#303a3c"; metalness: 0.3; roughness: 0.22 } }
        }
        Node {
            visible: Vehicle.portOpen || scene.portAngle< -1
            position: Qt.vector3d(94.8,96,-221)
            Model { source: "#Cylinder"; eulerRotation.z: 90; scale: Qt.vector3d(0.075,0.012,0.075); materials: PrincipledMaterial { baseColor: "#17211f"; roughness: 0.7 } }
            Repeater3D {
                model: 5
                Model { required property int index; source: "#Sphere"; x: 0.8; y: Math.cos(index*2*Math.PI/5)*2.2; z: Math.sin(index*2*Math.PI/5)*2.2; scale: Qt.vector3d(0.011,0.011,0.011); materials: PrincipledMaterial { baseColor: "#809a91"; metalness: 0.7; roughness: 0.3 } }
            }
        }
        ChargingField { active: scene.energyFlowRunning; energyVisible: Vehicle.plugged && scene.energyEffectsVisible }
        ChargingConnector { objectName: "chargingConnector"; plugged: Vehicle.plugged && !scene.interior; charging: Vehicle.charging; flowEnabled: scene.energyEffectsVisible; live: Vehicle.energyReady && scene.activeView && scene.visible && !scene.animationPaused && !scene.drivingMode }
        Sedan {
            id: car
            objectName: "sedanModel"
            y: 73
            eulerRotation.y: 180
            doorLF: Vehicle.doors[0] ? 1 : 0
            doorRF: Vehicle.doors[1] ? 1 : 0
            doorLR: Vehicle.doors[2] ? 1 : 0
            doorRR: Vehicle.doors[3] ? 1 : 0
            windowLF: Vehicle.windowActual[0]/100
            windowRF: Vehicle.windowActual[1]/100
            windowLR: Vehicle.windowActual[2]/100
            windowRR: Vehicle.windowActual[3]/100
            hatch: Vehicle.hatchOpen ? 1 : 0
            hood: Vehicle.hoodOpen ? 1 : 0
            lightsOn: Vehicle.lightsOn
            selectedPart: scene.selectedPart
            highBeam: Vehicle.highBeam
            fogOn: Vehicle.fogLights
            mirrorFold: Vehicle.mirrorsFolded?1:0
            wheelSpin: scene.drivingMode?scene.wheelAngle:0
            Behavior on mirrorFold { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: 650; easing.type: Easing.InOutCubic } }
            Behavior on doorLF { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.doorMotionDuration; easing.type: Easing.InOutCubic } }
            Behavior on doorRF { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.doorMotionDuration; easing.type: Easing.InOutCubic } }
            Behavior on doorLR { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.doorMotionDuration; easing.type: Easing.InOutCubic } }
            Behavior on doorRR { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.doorMotionDuration; easing.type: Easing.InOutCubic } }
            Behavior on hatch { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.hatchMotionDuration; easing.type: Easing.InOutCubic } }
            Behavior on hood { enabled: scene.motionEnabled && scene.visible; NumberAnimation { duration: Vehicle.hoodMotionDuration; easing.type: Easing.InOutCubic } }
        }
    }
    Timer { interval: 6000; running: scene.activeView && scene.visible && !scene.ready && !scene.renderFailed; onTriggered: scene.renderFailed=true }
    Rectangle {
        anchors.fill: parent; visible: !scene.ready; color: Theme.secondary; radius: 20
        Image { x: scene.framingOffset.x; y: scene.framingOffset.y; width: parent.width; height: parent.height; visible: scene.renderFailed || !vehicleAssetsAvailable; source: "qrc:/ui/vehicle-pearl.png"; fillMode: Image.PreserveAspectFit; opacity: 0.65 }
        Column { anchors.centerIn: parent; spacing: 14
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: scene.renderFailed?"car":"refresh"; tint: Theme.green; width: 38; height: 38
                RotationAnimation on rotation { from: 0; to: 360; duration: 1000; loops: Animation.Infinite; running: scene.activeView && !scene.ready && !scene.renderFailed }
            }
            Label { anchors.horizontalCenter: parent.horizontalCenter; text: scene.renderFailed||!vehicleAssetsAvailable?"3D 暂不可用，已切换静态预览":"正在准备 3D 车辆…"; font.pixelSize: 18 }
            UiButton { objectName: "reloadVehicle3D"; visible: scene.renderFailed; anchors.horizontalCenter: parent.horizontalCenter; text: "重新加载"; textSize: 17; onClicked: { scene.rendererEnabled=false;scene.presentedFrames=0;scene.renderFailed=false;Qt.callLater(function(){scene.rendererEnabled=true}) } }
        }
    }
    MouseArea {
        id: orbit
        anchors.fill: parent
        enabled: scene.interactive && scene.ready
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property point start
        property real baseYaw
        property real baseElevation
        property bool moved: false
        onPressed: mouse => { scene.interacted();focusAnimation.stop();start=Qt.point(mouse.x,mouse.y);baseYaw=scene.yaw;baseElevation=scene.elevation;moved=false }
        onPositionChanged: mouse => {
            if(!pressed || scene.interior)return
            if(Math.abs(mouse.x-start.x)+Math.abs(mouse.y-start.y)>5)moved=true
            scene.yaw=baseYaw-(mouse.x-start.x)*0.35
            scene.elevation=Math.max(-65,Math.min(5,baseElevation-(mouse.y-start.y)*0.18))
        }
        onReleased: mouse => { if(!moved){scene.partTapped(scene.partAt(mouse.x,mouse.y));scene.tapped()} }
        onWheel: wheel => { scene.interacted();focusAnimation.stop();if(!scene.interior)scene.distance=Math.max(520,Math.min(1100,scene.distance-wheel.angleDelta.y*0.55));wheel.accepted=true }
        onDoubleClicked: scene.resetView()
        onCanceled: moved=true
    }
    PinchHandler {
        target: null; enabled: scene.interactive && scene.ready && !scene.interior
        property real initialDistance: 650
        onActiveChanged: if(active){scene.interacted();focusAnimation.stop();initialDistance=scene.distance}
        onActiveScaleChanged: if(active)scene.distance=Math.max(520,Math.min(1100,initialDistance/activeScale))
    }
    Item {
        objectName: "selectedPartMarker"
        visible: scene.ready && !scene.interior && scene.selectedPart!=="" && scene.selectedFacing && scene.selectionPoint.z>0 && scene.selectionPoint.x>=0 && scene.selectionPoint.x<=scene.width && scene.selectionPoint.y>=0 && scene.selectionPoint.y<=scene.height
        x: Math.max(20,Math.min(scene.width-245,scene.selectionPoint.x));y: Math.max(20,Math.min(scene.height-45,scene.selectionPoint.y))
        Rectangle { x:-8;y:-8;width:16;height:16;radius:8;color:Theme.green;border.width:3;border.color:Theme.surface }
        Rectangle {
            x:16;y:-17;width:Math.min(222,partLabel.implicitWidth+24);height:34;radius:12;color:Theme.surface;border.width:1;border.color:Theme.line
            Label {id:partLabel;anchors.centerIn:parent;text:scene.selectedCaption;font.pixelSize:15;color:Theme.green}
        }
    }
}
