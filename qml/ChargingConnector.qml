import QtQuick
import QtQuick3D
Node {
    id: connector
    required property bool plugged
    required property bool charging
    required property bool live
    property bool flowEnabled: true
    visible: plugged
    property real glow: 0.5
    property real flowPhase: 0
    readonly property bool flowing: connector.visible && connector.charging && connector.live && connector.flowEnabled
    NumberAnimation on flowPhase { from: 0; to: 1; duration: 2600; loops: Animation.Infinite; running: connector.flowing }
    SequentialAnimation on glow {
        running: connector.visible && connector.charging && connector.live
        loops: Animation.Infinite
        NumberAnimation { to: 1; duration: 950; easing.type: Easing.InOutSine }
        NumberAnimation { to: 0.35; duration: 950; easing.type: Easing.InOutSine }
    }
    PrincipledMaterial { id: rubber; baseColor: "#28332f"; roughness: 0.85 }
    PrincipledMaterial { id: shell; baseColor: "#e1e7df"; roughness: 0.3; metalness: 0.15 }
    PrincipledMaterial { id: trim; baseColor: "#667e70"; roughness: 0.35; metalness: 0.3 }
    PrincipledMaterial { id: steel; baseColor: "#a9b7b3"; metalness: 0.85; roughness: 0.24 }
    PrincipledMaterial {
        id: led
        baseColor: connector.live?(connector.charging?"#78cda0":"#9bb9a3"):"#a2a995"
        emissiveFactor: connector.live&&connector.charging?Qt.vector3d(0.12,0.6,0.3).times(connector.glow):Qt.vector3d(0,0,0)
    }
    // The metallic tip fits into the existing rear-quarter port; the grip bends down to the cable.
    Model { source: "#Cylinder"; position: Qt.vector3d(102,96,-221); eulerRotation.z: 90; scale: Qt.vector3d(0.065,0.14,0.065); materials: steel }
    Model { source: "#Cylinder"; position: Qt.vector3d(110,96,-221); eulerRotation.z: 90; scale: Qt.vector3d(0.095,0.055,0.095); materials: rubber }
    Model { source: "#Cylinder"; position: Qt.vector3d(119,96,-221); eulerRotation.z: 90; scale: Qt.vector3d(0.105,0.17,0.105); materials: shell }
    Model { source: "#Sphere"; position: Qt.vector3d(127,96,-221); scale: Qt.vector3d(0.065,0.105,0.105); materials: shell }
    Model { source: "#Cylinder"; position: Qt.vector3d(112,96,-221); eulerRotation.z: 90; scale: Qt.vector3d(0.109,0.018,0.109); materials: trim }
    Model { source: "#Cube"; position: Qt.vector3d(119,101.2,-221); scale: Qt.vector3d(0.1,0.015,0.04); materials: rubber }
    Model { source: "#Cube"; position: Qt.vector3d(121,100,-226); scale: Qt.vector3d(0.075,0.018,0.009); materials: led }
    Node {
        position: Qt.vector3d(129,85,-221); eulerRotation.z: -18
        Model { source: "#Cylinder"; scale: Qt.vector3d(0.078,0.22,0.075); materials: rubber }
        Model { source: "#Cylinder"; y: 7; scale: Qt.vector3d(0.082,0.065,0.08); materials: shell }
        Repeater3D { model: 3; Model { required property int index; source: "#Cylinder"; y: 1-index*3.2; scale: Qt.vector3d(0.083,0.008,0.08); materials: trim } }
        Model { source: "#Cylinder"; y: -10; scale: Qt.vector3d(0.059,0.045,0.059); materials: rubber }
    }
    Model { geometry: ChargeCableGeometry { id: cableGeometry } materials: rubber }
    Repeater3D { model: 9
        Model {
            required property int index
            visible: connector.flowing
            source: "#Sphere"; scale: Qt.vector3d(0.055,0.055,0.055); castsShadows: false
            position: cableGeometry.pointAt(1-((index/9+connector.flowPhase)%1))
            materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: EnergyVisual.flowColor }
        }
    }
}
