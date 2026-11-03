import QtQuick
import QtQuick3D
Node {
    id: field
    required property bool active
    property bool energyVisible: Vehicle.plugged
    visible: energyVisible
    property real pulse: 0.4
    readonly property bool flowing: active && EnergyVisual.flow
    SequentialAnimation on pulse {
        running: field.flowing; loops: Animation.Infinite
        NumberAnimation { to: 1; duration: 1100; easing.type: Easing.InOutSine }
        NumberAnimation { to: 0.4; duration: 1100; easing.type: Easing.InOutSine }
    }
    Model {
        source: "#Cylinder"; y: 0.7; scale: Qt.vector3d(3.15,0.004,5.5); castsShadows: false; receivesShadows: false
        materials: PrincipledMaterial {
            lighting: PrincipledMaterial.NoLighting; alphaMode: PrincipledMaterial.Blend
            baseColor: EnergyVisual.mode==="charging"?"#63c6a5":EnergyVisual.mode==="full"||EnergyVisual.mode==="target"?"#56aaa3":"#8e9b92"
            opacity: EnergyVisual.mode==="charging"?(0.13+0.13*field.pulse):0.08
        }
    }
    // Two ten-segment rails are an SOC visualization, kept outside the opaque body.
    Repeater3D { model: 20
        Node {
            required property int index
            readonly property int slot: index%10
            readonly property real fill: Math.max(0,Math.min(1,EnergyVisual.soc/10-slot))
            x: index<10?130:-130; y: 2.6; z: -125+slot*25
            Model { source: "#Cube"; scale: Qt.vector3d(0.09,0.015,0.21); castsShadows: false
                materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: "#9faea4"; opacity: 0.28; alphaMode: PrincipledMaterial.Blend }
            }
            Model { source: "#Cube"; visible: parent.fill>0; y: 0.2; z: -10.5+10.5*parent.fill; scale: Qt.vector3d(0.09,0.018,0.21*parent.fill); castsShadows: false
                materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: EnergyVisual.levelColor; opacity: field.flowing?0.75+0.25*field.pulse:0.65; alphaMode: PrincipledMaterial.Blend }
            }
        }
    }
}
