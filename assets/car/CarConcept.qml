import QtQuick
import QtQuick3D

Node {
    id: root
    // Application bindings added to the imported CC-BY-4.0 model.
    property color paintColor: "#b9353b"
    property real leftDoorAngle: 0
    property real rightDoorAngle: 0
    property real hoodAngle: 0
    property real hatchAngle: 0
    property bool lightsOn: true
    property bool signalOn: false
    readonly property color actualPaint: paint_1_Carmine_material.baseColor
    readonly property vector3d actualHeadlight: headlight_material.emissiveFactor
    objectName: "Root"

    // Resources
    property url textureData: "maps/textureData.png"
    property url textureData5: "maps/textureData5.png"
    property url textureData7: "maps/textureData7.png"
    property url textureData9: "maps/textureData9.png"
    property url textureData30: "maps/textureData30.png"
    property url textureData17: "maps/textureData17.png"
    property url textureData61: "maps/textureData61.png"
    property url textureData98: "maps/textureData98.png"
    property url textureData209: "maps/textureData209.png"
    property url textureData211: "maps/textureData211.png"
    property url textureData215: "maps/textureData215.png"
    property url textureData217: "maps/textureData217.png"
    property url textureData220: "maps/textureData220.png"
    Texture {
        id: _4_texture
        pivotV: 1
        scaleU: 200
        scaleV: 400
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData30
    }
    Texture {
        id: _2_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData5
    }
    Texture {
        id: _3_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData7
    }
    Texture {
        id: _0_texture
        indexUV: 1
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData9
    }
    Texture {
        id: _7_texture
        pivotV: 1
        scaleU: 30
        scaleV: 30
        magFilter: Texture.Nearest
        minFilter: Texture.Nearest
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData98
    }
    Texture {
        id: _7_texture97
        pivotV: 1
        rotationUV: 33.540836334228516
        scaleU: 30
        scaleV: 30
        magFilter: Texture.Nearest
        minFilter: Texture.Nearest
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData98
    }
    Texture {
        id: _5_texture
        pivotV: 1
        scaleU: 100
        scaleV: 100
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData61
    }
    Texture {
        id: _1_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData17
    }
    Texture {
        id: _4_texture39
        pivotV: 1
        scaleU: 2
        scaleV: 3
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData30
    }
    Texture {
        id: _6_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData
    }
    Texture {
        id: _4_texture42
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData30
    }
    Texture {
        id: _11_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData217
    }
    Texture {
        id: _8_texture
        pivotV: 1
        scaleU: 20
        scaleV: 10
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData209
    }
    Texture {
        id: _9_texture
        pivotV: 1
        scaleU: 20
        scaleV: 10
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData211
    }
    Texture {
        id: _10_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData215
    }
    Texture {
        id: _12_texture
        pivotV: 1
        positionV: -1.5
        scaleV: 4
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: root.textureData220
    }
    PrincipledMaterial {
        id: interior_1_material
        objectName: "Interior 1"
        baseColor: "#ff0c0c0c"
        roughness: 0.4000000059604645
        normalMap: _4_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: interior_3_Carmine_material
        objectName: "Interior 3 Carmine"
        baseColor: "#ff800f00"
        roughness: 0.4000000059604645
        normalMap: _4_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: floormat_material
        objectName: "Floormat"
        baseColor: "#ff020202"
        roughness: 0.5
        normalMap: _4_texture39
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: panel_Sides_material
        objectName: "Panel Sides"
        baseColor: "#ff0c0c0c"
        roughness: 0.4000000059604645
        normalMap: _4_texture42
        normalStrength: 0
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: hardware_material
        objectName: "Hardware"
        baseColor: "#ff8e8e8e"
        metalness: 1
        roughness: 0.5
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: interior_2_material
        objectName: "Interior 2"
        baseColor: "#ff0c0c0c"
        roughness: 0.4000000059604645
        normalMap: _5_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: dashboard_material
        objectName: "Dashboard"
        baseColor: "#ff020202"
        roughness: 0.5
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _6_texture
        emissiveFactor: Qt.vector3d(1, 0.259027, 0)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: license_material
        objectName: "License"
        baseColorMap: _1_texture
        roughness: 0.800000011920929
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: paint_2_Carmine_material
        objectName: "Paint 2 Carmine"
        baseColor: "#ff373737"
        metalness: 1
        roughness: 0.30000001192092896
        normalMap: _7_texture97
        normalStrength: 0.30000001192092896
        occlusionMap: _0_texture
        occlusionAmount: 0
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
        clearcoatAmount: 0.25
    }
    PrincipledMaterial {
        id: paint_1_Carmine_material
        objectName: "Paint 1 Carmine"
        baseColor: root.paintColor
        metalness: 0.5
        roughness: 0.25
        normalMap: null
        normalStrength: 0.30000001192092896
        occlusionMap: _0_texture
        occlusionAmount: 0
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
        clearcoatAmount: 1
    }
    PrincipledMaterial {
        id: signallight_material
        objectName: "Signallight"
        baseColor: "#ff020202"
        roughness: 0.5
        emissiveFactor: root.signalOn ? Qt.vector3d(5, 1.5, 0) : Qt.vector3d(0, 0, 0)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: brakelight_material
        objectName: "Brakelight"
        baseColor: "#ff020202"
        roughness: 0.5
        emissiveFactor: root.lightsOn ? Qt.vector3d(3, 0.02, 0.01) : Qt.vector3d(0, 0, 0)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: headlight_material
        objectName: "Headlight"
        baseColor: "#ff020202"
        roughness: 0.5
        emissiveFactor: root.lightsOn ? Qt.vector3d(3.5, 4, 5) : Qt.vector3d(0, 0, 0)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: mirror_material
        objectName: "Mirror"
        metalness: 1
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: rim2_material
        objectName: "Rim2"
        metalness: 1
        roughness: 0.04899999871850014
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: rim1_material
        objectName: "Rim1"
        baseColor: "#ff010101"
        roughness: 0.4000000059604645
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: brake_material
        objectName: "Brake"
        baseColor: "#ffd30000"
        metalness: 1
        roughness: 0.20000000298023224
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: disc_material
        objectName: "Disc"
        baseColorMap: _8_texture
        metalness: 1
        roughness: 0.7300000190734863
        normalMap: _9_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: mechanical_material
        objectName: "Mechanical"
        baseColor: "#ff404040"
        metalnessMap: _2_texture
        roughnessMap: _2_texture
        metalness: 1
        roughness: 1
        normalMap: _3_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: glass_material
        objectName: "Glass"
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
        transmissionFactor: 1
    }
    PrincipledMaterial {
        id: tireside_material
        objectName: "Tireside"
        baseColorMap: _10_texture
        roughness: 0.4000000059604645
        normalMap: _11_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: principledMaterial_material
        objectName: "PrincipledMaterial"
        baseColor: "#ff090909"
        roughness: 0.800000011920929
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        emissiveMap: _1_texture
        emissiveFactor: Qt.vector3d(1, 1, 1)
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: tiretread_material
        objectName: "Tiretread"
        baseColor: "#ff181818"
        roughness: 0.6000000238418579
        normalMap: _12_texture
        occlusionMap: _0_texture
        occlusionAmount: 0.8500000238418579
        alphaMode: PrincipledMaterial.Opaque
    }

    // Nodes:
    Model {
        id: bodyUnderside
        objectName: "BodyUnderside"
        rotation: Qt.quaternion(0.707107, -0.707107, 0, 0)
        source: "meshes/bodyUnderside_mesh.mesh"
        materials: [
            mechanical_material
        ]
        Model {
            id: bodyWindshield
            objectName: "BodyWindshield"
            source: "meshes/bodyWindshield_mesh.mesh"
            materials: [
                glass_material
            ]
        }
        Model {
            id: bodyWindshieldGasket
            objectName: "BodyWindshieldGasket"
            source: "meshes/bodyWindshieldGasket_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: bodyWindshieldWipers
            objectName: "BodyWindshieldWipers"
            position: Qt.vector3d(0.68154, -1.42722, 0.849451)
            source: "meshes/bodyWindshieldWipers_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: bodyWindshieldWipersBase
            objectName: "BodyWindshieldWipersBase"
            source: "meshes/bodyWindshieldWipersBase_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: engine
            objectName: "Engine"
            position: Qt.vector3d(0, -2.07569, 0.14)
            source: "meshes/engine_mesh.mesh"
            materials: [
                mechanical_material
            ]
        }
        Model {
            id: interiorCage
            objectName: "InteriorCage"
            source: "meshes/interiorCage_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorDashMid
            objectName: "InteriorDashMid"
            source: "meshes/interiorDashMid_mesh.mesh"
            materials: [
                interior_1_material
            ]
        }
        Model {
            id: interiorDashSides
            objectName: "InteriorDashSides"
            source: "meshes/interiorDashSides_mesh.mesh"
            materials: [
                interior_3_Carmine_material
            ]
        }
        Model {
            id: interiorFloor
            objectName: "InteriorFloor"
            source: "meshes/interiorFloor_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorFloormats
            objectName: "InteriorFloormats"
            source: "meshes/interiorFloormats_mesh.mesh"
            materials: [
                floormat_material
            ]
        }
        Model {
            id: interiorMid
            objectName: "InteriorMid"
            source: "meshes/interiorMid_mesh.mesh"
            materials: [
                panel_Sides_material,
                panel_Sides_material
            ]
        }
        Model {
            id: interiorPedalAccel
            objectName: "InteriorPedalAccel"
            position: Qt.vector3d(-0.0748928, -1.45883, 0.129208)
            source: "meshes/interiorPedalAccel_mesh.mesh"
            materials: [
                floormat_material
            ]
        }
        Model {
            id: interiorPedalAccelArm
            objectName: "InteriorPedalAccelArm"
            position: Qt.vector3d(-0.0762586, -1.45883, 0.129208)
            source: "meshes/interiorPedalAccelArm_mesh.mesh"
            materials: [
                hardware_material
            ]
        }
        Model {
            id: interiorPedalBrake
            objectName: "InteriorPedalBrake"
            position: Qt.vector3d(0.0762205, -1.52258, 0.209799)
            source: "meshes/interiorPedalBrake_mesh.mesh"
            materials: [
                floormat_material
            ]
        }
        Model {
            id: interiorPedalBrakeArm
            objectName: "InteriorPedalBrakeArm"
            position: Qt.vector3d(0.0762586, -1.4272, 0.128699)
            source: "meshes/interiorPedalBrakeArm_mesh.mesh"
            materials: [
                hardware_material
            ]
        }
        Model {
            id: interiorPillar
            objectName: "InteriorPillar"
            source: "meshes/interiorPillar_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorSeatsColor1
            objectName: "InteriorSeatsColor1"
            position: Qt.vector3d(0.508642, -0.133467, -0.159757)
            source: "meshes/interiorSeatsColor1_mesh.mesh"
            materials: [
                interior_3_Carmine_material
            ]
        }
        Model {
            id: interiorSeatsColor2
            objectName: "InteriorSeatsColor2"
            position: Qt.vector3d(0, -0.804644, -0.192855)
            source: "meshes/interiorSeatsColor2_mesh.mesh"
            materials: [
                interior_2_material
            ]
        }
        Model {
            id: interiorSeatsFrame1
            objectName: "InteriorSeatsFrame1"
            position: Qt.vector3d(-4.521e-25, 0.606426, -0.180443)
            source: "meshes/interiorSeatsFrame1_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorSeatsFrame2
            objectName: "InteriorSeatsFrame2"
            position: Qt.vector3d(0, -0.804644, -0.192855)
            source: "meshes/interiorSeatsFrame2_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorSteeringBase
            objectName: "InteriorSteeringBase"
            position: Qt.vector3d(9.31323e-10, -0.929137, 0.641559)
            source: "meshes/interiorSteeringBase_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorSteeringCylinder
            objectName: "InteriorSteeringCylinder"
            position: Qt.vector3d(0.000921506, -1.16254, 0.551265)
            rotation: Qt.quaternion(0.973757, 0.186441, -0.128197, -0.0245454)
            scale: Qt.vector3d(1, 1, 1)
            source: "meshes/interiorSteeringCylinder_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
            Model {
                id: interiorSteeringEmblem
                objectName: "InteriorSteeringEmblem"
                position: Qt.vector3d(0, 0.250249, -0.00218725)
                rotation: Qt.quaternion(0.98216, -0.18805, 3.79296e-09, -3.43737e-09)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/interiorSteeringEmblem_mesh.mesh"
                materials: [
                    hardware_material
                ]
            }
            Model {
                id: interiorSteeringWheel04
                objectName: "InteriorSteeringWheel04"
                position: Qt.vector3d(0, 0.197338, -0.000637293)
                rotation: Qt.quaternion(0.98216, -0.18805, 3.79296e-09, -3.43737e-09)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/interiorSteeringWheel04_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: interiorSteeringWheel01
                objectName: "InteriorSteeringWheel01"
                position: Qt.vector3d(0, 0.250249, -0.00218725)
                rotation: Qt.quaternion(0.98216, -0.18805, 3.79296e-09, -3.43737e-09)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/interiorSteeringWheel01_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorSteeringWheel02
                objectName: "InteriorSteeringWheel02"
                position: Qt.vector3d(0, 0.250249, -0.00218725)
                rotation: Qt.quaternion(0.98216, -0.18805, 3.79296e-09, -3.43737e-09)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/interiorSteeringWheel02_mesh.mesh"
                materials: [
                    interior_1_material
                ]
            }
            Model {
                id: interiorSteeringWheel03
                objectName: "InteriorSteeringWheel03"
                position: Qt.vector3d(0, 0.250249, -0.00218725)
                rotation: Qt.quaternion(0.98216, -0.18805, 3.79296e-09, -3.43737e-09)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/interiorSteeringWheel03_mesh.mesh"
                materials: [
                    interior_1_material
                ]
            }
        }
        Model {
            id: interiorSteeringDash
            objectName: "InteriorSteeringDash"
            position: Qt.vector3d(-0.00583128, -1.02218, 0.613082)
            source: "meshes/interiorSteeringDash_mesh.mesh"
            materials: [
                dashboard_material
            ]
        }
        Model {
            id: interiorSteeringDashColumn
            objectName: "InteriorSteeringDashColumn"
            position: Qt.vector3d(-0.00583128, -1.02218, 0.613082)
            source: "meshes/interiorSteeringDashColumn_mesh.mesh"
            materials: [
                principledMaterial_material
            ]
        }
        Model {
            id: interiorSteeringHandleL
            objectName: "InteriorSteeringHandleL"
            position: Qt.vector3d(0.0722166, -1.1996, 0.529524)
            source: "meshes/interiorSteeringHandleL_mesh.mesh"
            materials: [
                hardware_material
            ]
        }
        Model {
            id: interiorSteeringHandleR
            objectName: "InteriorSteeringHandleR"
            position: Qt.vector3d(-0.0702543, -1.1996, 0.529524)
            source: "meshes/interiorSteeringHandleR_mesh.mesh"
            materials: [
                hardware_material
            ]
        }
        Model {
            id: license_Plate
            objectName: "License Plate"
            position: Qt.vector3d(0, 1.88541, 0.255378)
            source: "meshes/license_Plate_mesh.mesh"
            materials: [
                license_material
            ]
        }
        Model {
            id: bodyRoofPanel
            objectName: "BodyRoofPanel"
            source: "meshes/bodyRoofPanel_mesh.mesh"
            materials: [
                paint_2_Carmine_material
            ]
        }
        Model {
            id: bodyRearPanelsColor1
            eulerRotation: Qt.vector3d(root.hatchAngle, 0, 0)
            objectName: "BodyRearPanelsColor1"
            position: Qt.vector3d(0, 1.92853, 0.423014)
            source: "meshes/bodyRearPanelsColor1_mesh.mesh"
            materials: [
                paint_1_Carmine_material
            ]
            Model {
                id: interiorRearHatch
                objectName: "InteriorRearHatch"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/interiorRearHatch_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: interiorRearPanels
                objectName: "InteriorRearPanels"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/interiorRearPanels_mesh.mesh"
                materials: [
                    mechanical_material,
                    interior_1_material
                ]
            }
            Model {
                id: bodyWindowsRearSides
                objectName: "BodyWindowsRearSides"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/bodyWindowsRearSides_mesh.mesh"
                materials: [
                    glass_material
                ]
            }
            Model {
                id: bodyTurnsignalsRear
                objectName: "BodyTurnsignalsRear"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/bodyTurnsignalsRear_mesh.mesh"
                materials: [
                    signallight_material
                ]
            }
            Model {
                id: bodyTaillightsPanels
                objectName: "BodyTaillightsPanels"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/bodyTaillightsPanels_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyTaillights
                objectName: "BodyTaillights"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/bodyTaillights_mesh.mesh"
                materials: [
                    brakelight_material
                ]
            }
            Model {
                id: bodyRearwindow
                objectName: "BodyRearwindow"
                position: Qt.vector3d(0, -1.92704, -0.429743)
                rotation: Qt.quaternion(1, -2.38419e-07, 0, 0)
                source: "meshes/bodyRearwindow_mesh.mesh"
                materials: [
                    glass_material
                ]
            }
        }
        Model {
            id: bodyPillars
            objectName: "BodyPillars"
            source: "meshes/bodyPillars_mesh.mesh"
            materials: [
                paint_1_Carmine_material
            ]
        }
        Model {
            id: bodyPanelsColor2
            objectName: "BodyPanelsColor2"
            source: "meshes/bodyPanelsColor2_mesh.mesh"
            materials: [
                paint_2_Carmine_material
            ]
        }
        Model {
            id: bodyHood
            eulerRotation: Qt.vector3d(root.hoodAngle, 0, 0)
            objectName: "BodyHood"
            position: Qt.vector3d(-1.08825e-09, -2.37939, 0.176015)
            source: "meshes/bodyHood_mesh.mesh"
            materials: [
                paint_1_Carmine_material
            ]
            Model {
                id: bodyHoodInterior01
                objectName: "BodyHoodInterior01"
                position: Qt.vector3d(1.08825e-09, 2.37843, -0.188471)
                source: "meshes/bodyHoodInterior01_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyHoodInterior02
                objectName: "BodyHoodInterior02"
                position: Qt.vector3d(1.08825e-09, 2.37843, -0.188471)
                source: "meshes/bodyHoodInterior02_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyHoodTopgrill
                objectName: "BodyHoodTopgrill"
                position: Qt.vector3d(1.08825e-09, 2.37843, -0.188471)
                source: "meshes/bodyHoodTopgrill_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyHoodUnder
                objectName: "BodyHoodUnder"
                position: Qt.vector3d(1.08825e-09, 2.37843, -0.188471)
                source: "meshes/bodyHoodUnder_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyHeadlights
                objectName: "BodyHeadlights"
                position: Qt.vector3d(1.08825e-09, 2.37843, -0.188471)
                source: "meshes/bodyHeadlights_mesh.mesh"
                materials: [
                    headlight_material
                ]
            }
        }
        Model {
            id: bodyDoorRColor1
            eulerRotation: Qt.vector3d(0, 0, root.rightDoorAngle)
            objectName: "BodyDoorRColor1"
            position: Qt.vector3d(-1.0872, -0.997543, 0.642966)
            source: "meshes/bodyDoorRColor1_mesh.mesh"
            materials: [
                paint_1_Carmine_material
            ]
            Model {
                id: bodyDoorRColor2
                objectName: "BodyDoorRColor2"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRColor2_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyDoorRHandle01
                objectName: "BodyDoorRHandle01"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRHandle01_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyDoorRHandle02
                objectName: "BodyDoorRHandle02"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRHandle02_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyDoorRMirror
                objectName: "BodyDoorRMirror"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRMirror_mesh.mesh"
                materials: [
                    mirror_material
                ]
            }
            Model {
                id: bodyDoorRMirrorColor1
                objectName: "BodyDoorRMirrorColor1"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRMirrorColor1_mesh.mesh"
                materials: [
                    paint_1_Carmine_material
                ]
            }
            Model {
                id: bodyDoorRMirrorColor2
                objectName: "BodyDoorRMirrorColor2"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRMirrorColor2_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyDoorRWindow
                objectName: "BodyDoorRWindow"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRWindow_mesh.mesh"
                materials: [
                    glass_material
                ]
            }
            Model {
                id: bodyDoorRWindowGasket
                objectName: "BodyDoorRWindowGasket"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/bodyDoorRWindowGasket_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: interiorDoorR01
                objectName: "InteriorDoorR01"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR01_mesh.mesh"
                materials: [
                    mechanical_material,
                    interior_1_material
                ]
            }
            Model {
                id: interiorDoorR02
                objectName: "InteriorDoorR02"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR02_mesh.mesh"
                materials: [
                    interior_1_material
                ]
            }
            Model {
                id: interiorDoorR03
                objectName: "InteriorDoorR03"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR03_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorR04
                objectName: "InteriorDoorR04"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR04_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorR05
                objectName: "InteriorDoorR05"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR05_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorR06
                objectName: "InteriorDoorR06"
                position: Qt.vector3d(1.09068, 0.993742, -0.642966)
                rotation: Qt.quaternion(1, 0, 0, 4.47035e-08)
                source: "meshes/interiorDoorR06_mesh.mesh"
                materials: [
                    hardware_material
                ]
            }
        }
        Model {
            id: bodyDoorLColor1
            eulerRotation: Qt.vector3d(0, 0, root.leftDoorAngle)
            objectName: "BodyDoorLColor1"
            position: Qt.vector3d(1.08868, -0.997571, 0.643019)
            source: "meshes/bodyDoorLColor1_mesh.mesh"
            materials: [
                paint_1_Carmine_material
            ]
            Model {
                id: interiorDoorL04
                objectName: "InteriorDoorL04"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL04_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorL01
                objectName: "InteriorDoorL01"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL01_mesh.mesh"
                materials: [
                    mechanical_material,
                    interior_1_material
                ]
            }
            Model {
                id: interiorDoorL02
                objectName: "InteriorDoorL02"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL02_mesh.mesh"
                materials: [
                    interior_1_material
                ]
            }
            Model {
                id: interiorDoorL03
                objectName: "InteriorDoorL03"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL03_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorL05
                objectName: "InteriorDoorL05"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL05_mesh.mesh"
                materials: [
                    interior_3_Carmine_material
                ]
            }
            Model {
                id: interiorDoorL06
                objectName: "InteriorDoorL06"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/interiorDoorL06_mesh.mesh"
                materials: [
                    hardware_material
                ]
            }
            Model {
                id: bodyDoorLColor2
                objectName: "BodyDoorLColor2"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLColor2_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyDoorLHandle01
                objectName: "BodyDoorLHandle01"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLHandle01_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyDoorLHandle02
                objectName: "BodyDoorLHandle02"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLHandle02_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
            Model {
                id: bodyDoorLMirror
                objectName: "BodyDoorLMirror"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLMirror_mesh.mesh"
                materials: [
                    mirror_material
                ]
            }
            Model {
                id: bodyDoorLMirrorColor1
                objectName: "BodyDoorLMirrorColor1"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLMirrorColor1_mesh.mesh"
                materials: [
                    paint_1_Carmine_material
                ]
            }
            Model {
                id: bodyDoorLWindowGasket
                objectName: "BodyDoorLWindowGasket"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLWindowGasket_mesh.mesh"
                materials: [
                    principledMaterial_material
                ]
            }
            Model {
                id: bodyDoorLWindow
                objectName: "BodyDoorLWindow"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLWindow_mesh.mesh"
                materials: [
                    glass_material
                ]
            }
            Model {
                id: bodyDoorLMirrorColor2
                objectName: "BodyDoorLMirrorColor2"
                position: Qt.vector3d(-1.09216, 0.993765, -0.643019)
                rotation: Qt.quaternion(1, 0, 0, -2.38419e-07)
                source: "meshes/bodyDoorLMirrorColor2_mesh.mesh"
                materials: [
                    paint_2_Carmine_material
                ]
            }
        }
        Node {
            id: wheelFrontL
            objectName: "WheelFrontL"
            position: Qt.vector3d(0.975493, -1.48568, 0.383759)
            rotation: Qt.quaternion(0.482963, 0.836516, -0.224144, -0.12941)
            Model {
                id: wheelFrontLRim
                objectName: "WheelFrontLRim"
                position: Qt.vector3d(-0.00121474, 0, -3.74019e-05)
                source: "meshes/wheelFrontLRim_mesh.mesh"
                materials: [
                    rim2_material,
                    rim1_material
                ]
            }
            Model {
                id: wheelFrontLBrakePad
                objectName: "WheelFrontLBrakePad"
                position: Qt.vector3d(-0.00121474, 0, -3.70443e-05)
                source: "meshes/wheelFrontLBrakePad_mesh.mesh"
                materials: [
                    brake_material
                ]
            }
            Model {
                id: wheelFrontLBrakeDisc
                objectName: "WheelFrontLBrakeDisc"
                position: Qt.vector3d(-0.00121474, 0, -3.70443e-05)
                source: "meshes/wheelFrontLBrakeDisc_mesh.mesh"
                materials: [
                    disc_material
                ]
            }
            Model {
                id: nodes_84_
                objectName: "nodes[84]"
                source: "meshes/nodes_84__mesh.mesh"
                materials: [
                    tireside_material,
                    tiretread_material
                ]
            }
        }
        Node {
            id: wheelFrontR
            objectName: "WheelFrontR"
            position: Qt.vector3d(-0.976675, -1.48397, 0.383759)
            rotation: Qt.quaternion(0.892399, 0.369644, -0.0990458, -0.239118)
            scale: Qt.vector3d(1, 1, 1)
            Model {
                id: wheelFrontRRim
                objectName: "WheelFrontRRim"
                position: Qt.vector3d(-0.000290811, -0.00142258, 1.19209e-07)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelFrontRRim_mesh.mesh"
                materials: [
                    rim2_material,
                    rim1_material
                ]
            }
            Model {
                id: wheelFrontRBrakePad
                objectName: "WheelFrontRBrakePad"
                position: Qt.vector3d(-0.000290871, -0.00142246, 0)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelFrontRBrakePad_mesh.mesh"
                materials: [
                    brake_material
                ]
            }
            Model {
                id: wheelFrontRBrakeDisc
                objectName: "WheelFrontRBrakeDisc"
                position: Qt.vector3d(-0.000290871, -0.00142246, 0)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelFrontRBrakeDisc_mesh.mesh"
                materials: [
                    disc_material
                ]
            }
            Model {
                id: nodes_89_
                objectName: "nodes[89]"
                source: "meshes/nodes_89__mesh.mesh"
                materials: [
                    tireside_material,
                    tiretread_material
                ]
            }
        }
        Node {
            id: wheelRearL
            objectName: "WheelRearL"
            position: Qt.vector3d(0.982514, 1.31406, 0.383759)
            rotation: Qt.quaternion(0.573576, -0.819152, 0, 0)
            scale: Qt.vector3d(1, 1, 1)
            Model {
                id: wheelRearLBrakeDisc
                objectName: "WheelRearLBrakeDisc"
                position: Qt.vector3d(-3.57628e-07, -5.60284e-05, 0)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelRearLBrakeDisc_mesh.mesh"
                materials: [
                    disc_material
                ]
            }
            Model {
                id: wheelRearLBrakePad
                objectName: "WheelRearLBrakePad"
                position: Qt.vector3d(-3.57628e-07, -5.60284e-05, 0)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelRearLBrakePad_mesh.mesh"
                materials: [
                    brake_material
                ]
            }
            Model {
                id: wheelRearLRim
                objectName: "WheelRearLRim"
                position: Qt.vector3d(-3.57628e-07, -5.60284e-05, 0)
                scale: Qt.vector3d(1, 1, 1)
                source: "meshes/wheelRearLRim_mesh.mesh"
                materials: [
                    rim2_material,
                    rim1_material
                ]
            }
            Model {
                id: nodes_94_
                objectName: "nodes[94]"
                source: "meshes/nodes_94__mesh.mesh"
                materials: [
                    tireside_material,
                    tiretread_material
                ]
            }
        }
        Node {
            id: wheelRearR
            objectName: "WheelRearR"
            position: Qt.vector3d(-0.981705, 1.31406, 0.383759)
            rotation: Qt.quaternion(0.887011, -0.461749, 0, 0)
            Model {
                id: wheelRearRBrakeDisc
                objectName: "WheelRearRBrakeDisc"
                position: Qt.vector3d(-0.000809312, -5.26309e-05, 0)
                source: "meshes/wheelRearRBrakeDisc_mesh.mesh"
                materials: [
                    disc_material
                ]
            }
            Model {
                id: wheelRearRBrakePad
                objectName: "WheelRearRBrakePad"
                position: Qt.vector3d(-0.000809312, -5.26309e-05, 0)
                source: "meshes/wheelRearRBrakePad_mesh.mesh"
                materials: [
                    brake_material
                ]
            }
            Model {
                id: wheelRearRRim
                objectName: "WheelRearRRim"
                position: Qt.vector3d(-0.000809312, -5.21839e-05, 1.19209e-07)
                source: "meshes/wheelRearRRim_mesh.mesh"
                materials: [
                    rim2_material,
                    rim1_material
                ]
            }
            Model {
                id: nodes_99_
                objectName: "nodes[99]"
                source: "meshes/nodes_99__mesh.mesh"
                materials: [
                    tireside_material,
                    tiretread_material
                ]
            }
        }
        Model {
            id: axles
            objectName: "Axles"
            position: Qt.vector3d(-0.000161111, -0.0906695, 0.383868)
            source: "meshes/axles_mesh.mesh"
            materials: [
                mechanical_material
            ]
        }
    }

    // Animations:
}
