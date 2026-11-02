import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import "../assets/car" as Car

ApplicationWindow {
    id: window
    width: 1440; height: 900
    minimumWidth: 1120; minimumHeight: 720
    visible: true
    title: "Car Terminal · 3D 车辆演示"
    color: "#0b121c"
    font.family: "Microsoft YaHei UI"
    property color paintColor: "#b9353b"
    property bool leftDoor: false
    property bool rightDoor: false
    property bool hoodOpen: false
    property bool hatchOpen: false
    property bool lightsOn: true
    property bool turnsignals: false
    property bool blink: true
    property bool orbiting: false
    property bool demoRunning: false
    property int demoStep: 0
    property real cameraYaw: 35
    property real cameraPitch: -17
    property real cameraDistance: 680
    readonly property real renderedDoorAngle: vehicle.leftDoorAngle
    readonly property color renderedPaint: vehicle.actualPaint
    readonly property vector3d renderedHeadlight: vehicle.actualHeadlight

    function resetVehicle() {
        leftDoor = false; rightDoor = false; hoodOpen = false; hatchOpen = false
        lightsOn = true; turnsignals = false
    }
    function setView(yaw) { orbiting = false; cameraYaw = yaw; cameraPitch = -17; cameraDistance = 680 }
    function smokeState(phase) {
        demoRunning = false; orbiting = false
        if (phase === 1) {
            leftDoor = true; rightDoor = true; hoodOpen = true; hatchOpen = true
            paintColor = "#cad6df"; lightsOn = false; cameraYaw = 40
        } else { resetVehicle(); cameraYaw = 180; paintColor = "#436e86" }
    }
    Shortcut { sequence: "F11"; onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { sequence: "Escape"; onActivated: { window.visibility = Window.Windowed; demoRunning = false; orbiting = false } }
    Timer { interval: 450; running: window.turnsignals; repeat: true; onTriggered: window.blink = !window.blink }
    Timer { interval: 30; running: window.orbiting; repeat: true; onTriggered: window.cameraYaw += 0.23 }
    Timer {
        interval: 2800; running: window.demoRunning; repeat: true; triggeredOnStart: true
        onTriggered: {
            switch (window.demoStep % 6) {
            case 0: window.resetVehicle(); window.setView(35); window.paintColor = "#b9353b"; break
            case 1: window.leftDoor = true; window.rightDoor = true; break
            case 2: window.hoodOpen = true; window.hatchOpen = true; break
            case 3: window.paintColor = "#cad6df"; window.setView(145); break
            case 4: window.resetVehicle(); window.turnsignals = true; break
            case 5: window.paintColor = "#436e86"; window.orbiting = true; break
            }
            window.demoStep++
        }
    }
    Rectangle { width: parent.width; height: 1; y: 76; color: "#23303c" }
    Row {
        x: 32; y: 22; spacing: 14
        Rectangle {
            width: 32; height: 32; radius: 9; color: "#82e4cd"
            Text { anchors.centerIn: parent; text: "C"; color: "#102822"; font.pixelSize: 23; font.bold: true }
        }
        Column {
            Text { text: "CAR TERMINAL"; font.pixelSize: 15; font.letterSpacing: 3; color: "#e7f0f5"; font.bold: true }
            Text { text: "智能座舱 / 三维车辆控制"; font.pixelSize: 10; color: "#8b9dad" }
        }
    }
    Row {
        anchors.right: parent.right; anchors.rightMargin: 32; y: 28; spacing: 24
        Text { text: "P  已驻车"; color: "#91e5ca"; font.pixelSize: 14 }
        Text { text: "演示车辆 · 离线场景"; color: "#8b9dad"; font.pixelSize: 14 }
        Text { id: clock; text: Qt.formatTime(new Date(), "hh:mm"); font.pixelSize: 18; color: "#e3ebf2"; Timer { interval: 1000; repeat: true; running: true; onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm") } }
    }

    Item {
        id: stage
        anchors { left: parent.left; top: parent.top; bottom: footer.top; right: controlPanel.left; topMargin: 77; rightMargin: 20 }
        Column {
            x: 36; y: 30; spacing: 7
            Text { text: "探索你的座驾"; color: "#f1f6fb"; font.pixelSize: 32; font.weight: Font.DemiBold }
            Text { text: "CONCEPT 01  /  INTERACTIVE SHOWROOM"; color: "#7f97a8"; font.pixelSize: 11; font.letterSpacing: 2 }
        }
        Rectangle {
            anchors.right: parent.right; anchors.rightMargin: 12; y: 36
            width: 98; height: 28; radius: 14; color: "#17322d"; border.color: "#285548"
            Text { anchors.centerIn: parent; text: "●  实时 3D"; color: "#91e5ca"; font.pixelSize: 11 }
        }
        View3D {
            id: viewport
            anchors { fill: parent; topMargin: 90; bottomMargin: 95 }
            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Transparent
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
                lightProbe: Texture { source: "../assets/studio.hdr" }
                probeExposure: 1.0
                tonemapMode: SceneEnvironment.TonemapModeFilmic
            }
            Node {
                id: cameraPivot
                position: Qt.vector3d(0, 62, 0)
                eulerRotation: Qt.vector3d(window.cameraPitch, window.cameraYaw, 0)
                PerspectiveCamera { id: mainCamera; z: window.cameraDistance; clipNear: 5; clipFar: 5000; fieldOfView: 40 }
            }
            camera: mainCamera
            DirectionalLight { eulerRotation: Qt.vector3d(-45, -30, 0); brightness: 1.1; ambientColor: "#606872"; color: "#e3f2ff"; castsShadow: true; shadowFactor: 55; shadowMapQuality: Light.ShadowMapQualityHigh; shadowBias: 0.2 }
            DirectionalLight { eulerRotation: Qt.vector3d(-20, 140, 0); brightness: 0.7; color: "#acc9ee" }
            Model {
                source: "#Cylinder"; y: -4
                scale: Qt.vector3d(6.2, 0.035, 6.2)
                materials: PrincipledMaterial { baseColor: "#0b121c"; roughness: 0.85; metalness: 0 }
            }
            Car.CarConcept {
                id: vehicle
                scale: Qt.vector3d(100, 100, 100)
                paintColor: window.paintColor
                leftDoorAngle: window.leftDoor ? -58 : 0
                rightDoorAngle: window.rightDoor ? 58 : 0
                hoodAngle: window.hoodOpen ? 38 : 0
                hatchAngle: window.hatchOpen ? -32 : 0
                lightsOn: window.lightsOn
                signalOn: window.turnsignals && window.blink
                Behavior on leftDoorAngle { NumberAnimation { duration: 850; easing.type: Easing.InOutCubic } }
                Behavior on rightDoorAngle { NumberAnimation { duration: 850; easing.type: Easing.InOutCubic } }
                Behavior on hoodAngle { NumberAnimation { duration: 850; easing.type: Easing.InOutCubic } }
                Behavior on hatchAngle { NumberAnimation { duration: 850; easing.type: Easing.InOutCubic } }
            }
            MouseArea {
                anchors.fill: parent
                property real lastX: 0
                property real lastY: 0
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                onPressed: function(mouse) { window.orbiting = false; window.demoRunning = false; lastX = mouse.x; lastY = mouse.y }
                onPositionChanged: function(mouse) {
                    if (!pressed) return
                    window.cameraYaw -= (mouse.x - lastX) * 0.4
                    window.cameraPitch = Math.max(-65, Math.min(-5, window.cameraPitch - (mouse.y - lastY) * 0.25))
                    lastX = mouse.x; lastY = mouse.y
                }
                onWheel: function(wheel) { window.cameraDistance = Math.max(540, Math.min(1100, window.cameraDistance - wheel.angleDelta.y * 0.6)) }
            }
        }
        Column {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 22 }
            spacing: 12
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "拖动旋转 · 滚轮缩放 · F11 全屏"; color: "#788e9e"; font.pixelSize: 11 }
            Row {
                spacing: 8
                Repeater {
                    model: [{label: "前侧视角", yaw: 35}, {label: "正面", yaw: 0}, {label: "侧面", yaw: 90}, {label: "车尾", yaw: 180}]
                    ActionButton { required property var modelData; width: 94; implicitHeight: 38; text: modelData.label; active: !window.orbiting && Math.abs(window.cameraYaw % 360 - modelData.yaw) < 1; onClicked: window.setView(modelData.yaw) }
                }
                ActionButton { width: 94; implicitHeight: 38; text: "环绕展示"; active: window.orbiting; onClicked: window.orbiting = !window.orbiting }
            }
        }
    }

    Rectangle {
        id: controlPanel
        anchors { right: parent.right; top: parent.top; bottom: footer.top; rightMargin: 28; topMargin: 101; bottomMargin: 24 }
        width: 304; radius: 20; color: "#101b27"; border.color: "#253340"
        ColumnLayout {
            anchors { fill: parent; margins: 22 } spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Text { text: "车辆控制"; font.pixelSize: 21; font.weight: Font.DemiBold; color: "#eaf1f6"; Layout.fillWidth: true }
                Text { text: "LIVE"; font.pixelSize: 10; font.letterSpacing: 2; color: "#82e4cd" }
            }
            Text { text: "选择部件，查看车辆实时状态"; font.pixelSize: 11; color: "#8296a7" }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#263440"; Layout.topMargin: 3; Layout.bottomMargin: 3 }
            GridLayout {
                columns: 2; uniformCellWidths: true; columnSpacing: 10; rowSpacing: 10; Layout.fillWidth: true
                ActionButton { Layout.fillWidth: true; text: "左侧车门"; caption: window.leftDoor ? "已打开" : "已关闭"; active: window.leftDoor; onClicked: { window.demoRunning = false; window.leftDoor = !window.leftDoor } }
                ActionButton { Layout.fillWidth: true; text: "右侧车门"; caption: window.rightDoor ? "已打开" : "已关闭"; active: window.rightDoor; onClicked: { window.demoRunning = false; window.rightDoor = !window.rightDoor } }
                ActionButton { Layout.fillWidth: true; text: "前盖"; caption: window.hoodOpen ? "已打开" : "已关闭"; active: window.hoodOpen; onClicked: { window.demoRunning = false; window.hoodOpen = !window.hoodOpen } }
                ActionButton { Layout.fillWidth: true; text: "后舱盖"; caption: window.hatchOpen ? "已打开" : "已关闭"; active: window.hatchOpen; onClicked: { window.demoRunning = false; window.hatchOpen = !window.hatchOpen } }
                ActionButton { Layout.fillWidth: true; text: "车辆灯光"; caption: window.lightsOn ? "已开启" : "已关闭"; active: window.lightsOn; onClicked: { window.demoRunning = false; window.lightsOn = !window.lightsOn } }
                ActionButton { Layout.fillWidth: true; text: "双闪警示"; caption: window.turnsignals ? "闪烁中" : "已关闭"; active: window.turnsignals; onClicked: { window.demoRunning = false; window.turnsignals = !window.turnsignals } }
            }
            Text { text: "个性车漆"; font.pixelSize: 13; color: "#bdcbd6"; Layout.topMargin: 10 }
            RowLayout {
                Layout.fillWidth: true; spacing: 16
                Repeater {
                    model: ["#b9353b", "#cad6df", "#436e86", "#363c43"]
                    Rectangle {
                        required property string modelData
                        width: 46; height: 46; radius: 23; color: "transparent"
                        border.width: 2; border.color: window.paintColor == modelData ? "#a2f4df" : "#31414f"
                        Rectangle { anchors.fill: parent; anchors.margins: 5; radius: 20; color: parent.modelData }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { window.demoRunning = false; window.paintColor = parent.modelData } }
                    }
                }
            }
            Text { text: "模型：Car Concept / 双门概念跑车"; color: "#7d94a6"; font.pixelSize: 10; Layout.topMargin: 2 }
            Item { Layout.fillHeight: true; Layout.minimumHeight: 2 }
            ActionButton {
                Layout.fillWidth: true; implicitHeight: 46; text: window.demoRunning ? "停止自动演示" : "一键自动演示"; active: window.demoRunning
                onClicked: { window.demoStep = 0; window.demoRunning = !window.demoRunning; if (!window.demoRunning) window.orbiting = false }
            }
            Button {
                Layout.fillWidth: true; text: "恢复车辆状态"; implicitHeight: 30
                contentItem: Text { text: parent.text; color: "#8da3b4"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Item {}
                onClicked: { window.demoRunning = false; window.resetVehicle(); window.setView(35); window.paintColor = "#b9353b" }
            }
        }
    }
    Rectangle {
        id: footer
        height: 104; anchors { left: parent.left; right: parent.right; bottom: parent.bottom } color: "#0e1823"
        Rectangle { width: parent.width; height: 1; color: "#263441" }
        Row {
            x: 36; y: 19; spacing: 55
            Repeater {
                model: [{value: "82", unit: "%", label: "模拟电量"}, {value: "426", unit: "km", label: "模拟续航"}, {value: "P", unit: "", label: "驻车状态"}]
                Column {
                    required property var modelData
                    spacing: 4
                    Row { spacing: 5; Text { text: modelData.value; color: "#eaf4f8"; font.pixelSize: 27; font.weight: Font.Light } Text { text: modelData.unit; color: "#8ba0af"; font.pixelSize: 12; anchors.baseline: parent.children[0].baseline } }
                    Text { text: modelData.label; color: "#8196a8"; font.pixelSize: 10 }
                }
            }
        }
        Column {
            anchors { right: parent.right; rightMargin: 32; verticalCenter: parent.verticalCenter } spacing: 6
            Text { anchors.right: parent.right; text: "Qt Quick 3D · 实时渲染"; color: "#a8bdcc"; font.pixelSize: 12 }
            Text { anchors.right: parent.right; text: "Car Concept © 2024 DGG · Eric Chadwick · CC BY 4.0"; color: "#657e91"; font.pixelSize: 9 }
        }
    }
}
