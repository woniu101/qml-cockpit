"""Reimport the attributed Model 3 asset and add QML animation bindings.

Run with Python 3 after placing source/Sedan.glb; no modelling tool required.
The source geometry and original part pivots are preserved.
"""
from pathlib import Path
import re
import subprocess
import struct
import zlib
from rig_sedan_parts import rig

def normalize_grey_png(path):
    """Preserve pixels while expanding 8-bit greyscale PNGs to RGB textures."""
    data = path.read_bytes()
    if data[25] != 0:
        return
    width, height = struct.unpack_from('>II', data, 16)
    assert data[24] == 8 and data[28] == 0
    cursor, compressed = 8, b''
    while cursor < len(data):
        size = struct.unpack_from('>I', data, cursor)[0]
        if data[cursor+4:cursor+8] == b'IDAT':
            compressed += data[cursor+8:cursor+8+size]
        cursor += size+12
    raw = zlib.decompress(compressed)
    previous, rows = [0]*width, bytearray()
    for y in range(height):
        mode = raw[y*(width+1)]
        row = list(raw[y*(width+1)+1:(y+1)*(width+1)])
        for x in range(width):
            a, b, c = row[x-1] if x else 0, previous[x], previous[x-1] if x else 0
            if mode == 1: predictor = a
            elif mode == 2: predictor = b
            elif mode == 3: predictor = (a+b)//2
            elif mode == 4:
                p = a+b-c
                distances = [abs(p-a),abs(p-b),abs(p-c)]
                predictor = [a,b,c][distances.index(min(distances))]
            else:
                assert mode == 0
                predictor = 0
            row[x] = (row[x]+predictor)&255
        rows.append(0)
        for value in row:
            rows.extend([value,value,value])
        previous = row
    def chunk(kind, body):
        return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body)&0xffffffff)
    result = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
    result += chunk(b'IDAT',zlib.compress(bytes(rows)))+chunk(b'IEND',b'')
    path.write_bytes(result)

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / 'assets/sedan/Sedan.qml'
subprocess.run([r'D:\Qt\6.12.0\msvc2022_64\bin\balsam.exe', '-o', str(TARGET.parent),
                str(rig())], check=True)
for image in (TARGET.parent/'maps').glob('*.png'):
    normalize_grey_png(image)
s = TARGET.read_text(encoding='utf-8')
s = s.replace('Model {', 'Model {\n                                    pickable: true')
s = s.replace('        cullMode:', '        vertexColorsEnabled: false\n        cullMode:')
# glTF atlases use a top-left image origin; Quick 3D's texture coordinates
# need the vertical transform after Balsam conversion (check headlight atlas).
s = s.replace('        generateMipmaps: true', '        flipV: true\n        generateMipmaps: true')
props = ['doorLF', 'doorRF', 'doorLR', 'doorRR', 'windowLF', 'windowRF', 'windowLR', 'windowRR', 'hatch', 'hood', 'mirrorFold', 'wheelSpin']
s = s.replace('    // Resources', '\n'.join(f'    property real {p}: 0' for p in props) + '\n    property bool lightsOn: false\n    property bool highBeam: false\n    property bool fogOn: false\n\n    // Resources', 1)
for part, prop, angle in [('lf','doorLF',-62), ('rf','doorRF',62), ('lr','doorLR',-58), ('rr','doorRR',58)]:
    pattern = rf'(id: door_{part}_dummy\s+objectName: [^\n]+\s+position: [^\n]+\s+)rotation: [^\n]+'
    s, count = re.subn(pattern, rf'\1eulerRotation.z: root.{prop} * {angle}', s)
    assert count == 1, part
    s, count = re.subn(rf'(id: door_{part}_glass_0_0\s+objectName: [^\n]+)',
                        rf'\1\n                                    z: -0.47 * root.window{part.upper()}', s)
    assert count == 1
for node, prop, angle in [('boot_dummy', 'hatch', -48), ('bonnet_dummy', 'hood', 38)]:
    s, count = re.subn(rf'(id: {node}\s+objectName: [^\n]+\s+position: [^\n]+\s+)rotation: [^\n]+',
                       rf'\1eulerRotation.x: root.{prop} * {angle}', s)
    assert count == 1
for side, angle in [('lf', 75), ('rf', -75)]:
    s, count = re.subn(rf'(id: mirror_{side}_pivot\b)', rf'\1\n                        eulerRotation.z: root.mirrorFold * {angle}', s)
    assert count == 1, side
# Both sides share an axle in the source. Rotate the tire/rim/disc meshes
# about that axle; retain the caliper and steering-link meshes in place.
for prefix in ['wheels', 'wheels_001']:
    for material in [1, 2, 3, 4, 6]:
        pattern = rf'(id: {prefix}_wheels_{material}_0\b)'
        s, count = re.subn(pattern, rf'\1\n                                eulerRotation.x: root.wheelSpin', s)
        assert count == 1, (prefix, material)
# Keep texture-based trim intact; tune the main white lacquer independently.
s = s.replace('baseColor: "#ffe9e9e9"\n        roughness: 0.280829519033432',
              'baseColor: "#f3f2ec"\n        metalness: 0.12\n        roughness: 0.24\n        clearcoatAmount: 0.7\n        clearcoatRoughnessAmount: 0.18')
for name in ['right_front_light_material', 'left_front_light_material', 'right_rear_light_material', 'left_rear_light_material', 'breaklight_l_material']:
    pattern = rf'(PrincipledMaterial \{{\s+id: {name}\b)(.*?)(\n    \}})'
    def light(m):
        body = re.sub(r'\n        emissiveFactor: [^\n]+', '', m[2])
        factor = '(root.highBeam ? Qt.vector3d(5, 5, 5) : Qt.vector3d(2.2, 2.2, 2.2))' if 'front' in name else 'Qt.vector3d(1.6, 0.15, 0.12)'
        return m[1] + body + '\n        emissiveFactor: root.lightsOn ? '+factor+' : Qt.vector3d(0, 0, 0)' + m[3]
    s, count = re.subn(pattern, light, s, flags=re.S)
    assert count == 1
# Replace the source's coloured exterior trim atlas with a neutral dark finish.
for name in ['just_BLACK_0_material', 'black_lights_0_material']:
    s = re.sub(rf'(id: {name}\s+objectName: [^\n]+)\s+baseColorMap: [^\n]+',
               r'\1\n        baseColor: "#242b2a"\n        metalness: 0.25', s)
# The supplied pantulans/satin atlas is solid red and was assigned to window
# trim, badges and lamp reflectors. Use a neutral satin metal for the demo.
for name in ['pantulans_0_material', 'satin_red_0_material']:
    pattern = rf'(PrincipledMaterial \{{\s+id: {name}\b)(.*?)(\n    \}})'
    def trim(m):
        body = re.sub(r'\n        (?:baseColorMap|emissiveMap|emissiveFactor|roughness): [^\n]+', '', m[2])
        return m[1] + body + '\n        baseColor: "#b3bbbc"\n        metalness: 0.65\n        roughness: 0.26' + m[3]
    s = re.sub(pattern, trim, s, flags=re.S)
for name in ['left_front_light_material', 'right_front_light_material']:
    pattern = rf'(PrincipledMaterial \{{\s+id: {name}\b)(.*?)(\n    \}})'
    def headlamp(m):
        body = re.sub(r'\n        (?:baseColorMap|emissiveMap): [^\n]+', '', m[2])
        return m[1] + body + '\n        baseColor: root.lightsOn ? "#edf8ff" : "#43525a"' + m[3]
    s = re.sub(pattern, headlamp, s, flags=re.S)
for name in ['foglight_r_material', 'foglight_l_material']:
    pattern = rf'(PrincipledMaterial \{{\s+id: {name}\b)(.*?)(\n    \}})'
    def fog(m):
        body = re.sub(r'\n        (?:baseColorMap|emissiveMap|emissiveFactor): [^\n]+', '', m[2])
        return m[1]+body+'\n        baseColor: "#64737a"\n        emissiveFactor: root.fogOn ? Qt.vector3d(4, 4, 4) : Qt.vector3d(0, 0, 0)'+m[3]
    s, count = re.subn(pattern, fog, s, flags=re.S)
    assert count == 1
front_lens = '''    PrincipledMaterial {
        id: frontLensMaterial
        baseColor: "#b4dce3e5"
        roughness: 0.13
        opacity: 0.32
        alphaMode: PrincipledMaterial.Blend
        cullMode: PrincipledMaterial.NoCulling
    }
'''
s = s.replace('    // Nodes:', front_lens + '\n    // Nodes:')
s, count = re.subn(r'(id: tembus_depan_ok_tembus_red_0_0\b.*?materials: \[\s+)tembus_red_0_material', r'\1frontLensMaterial', s, flags=re.S)
assert count == 1, 'front lens'
# Selection materials follow the original moving nodes, so highlighting moves
# with an open door or descending glass rather than a screen-space rectangle.
s = s.replace('    property bool lightsOn: false', '    property bool lightsOn: false\n    property string selectedPart: ""')
selection_materials = '''    PrincipledMaterial {
        id: selectedPaintMaterial
        baseColor: "#98c6aa"
        metalness: 0.08
        roughness: 0.3
        clearcoatAmount: 0.5
        cullMode: PrincipledMaterial.NoCulling
    }
    PrincipledMaterial {
        id: selectedGlassMaterial
        baseColor: "#8acfa8"
        opacity: 0.57
        roughness: 0.24
        alphaMode: PrincipledMaterial.Blend
        cullMode: PrincipledMaterial.NoCulling
    }
'''
s = s.replace('    // Nodes:', selection_materials + '\n    // Nodes:')
for index, part in enumerate(['lf','rf','lr','rr']):
    for mesh in [f'door_{part}_primary_0', f'door_{part}_ok_primary_0', f'door_{part}_ok_primary_002_0']:
        pattern = rf'(id: {mesh}\b.*?materials:) \[\s+(\w+)\s+\]'
        s = re.sub(pattern, lambda m: m[1]+f' root.selectedPart === "door{index}" ? [selectedPaintMaterial] : [{m[2]}]', s, count=1, flags=re.S)
    pattern = rf'(id: door_{part}_glass_0_0\b.*?materials:) \[\s+(\w+)\s+\]'
    s, count = re.subn(pattern, lambda m: m[1]+f' root.selectedPart === "window{index}" ? [selectedGlassMaterial] : [{m[2]}]', s, count=1, flags=re.S)
    assert count == 1
for mesh, key in [('boot_primary_0','hatch'),('bonnet_ok_primary_0','hood')]:
    pattern = rf'(id: {mesh}\b.*?materials:) \[\s+(\w+)\s+\]'
    s, count = re.subn(pattern, lambda m: m[1]+f' root.selectedPart === "{key}" ? [selectedPaintMaterial] : [{m[2]}]', s, count=1, flags=re.S)
    assert count == 1, key
anchor = '''    function markerPosition(part) {
        if (part.indexOf("window") === 0 || part.indexOf("door") === 0) {
            const index = Number(part.slice(-1))
            return [door_lf_dummy,door_rf_dummy,door_lr_dummy,door_rr_dummy][index].mapPositionToScene(Qt.vector3d(index%2 === 0 ? -0.04 : 0.04,-0.48,0.52))
        }
        if (part === "hood") return bonnet_dummy.mapPositionToScene(Qt.vector3d(0,0.45,0.05))
        if (part === "hatch") return boot_dummy.mapPositionToScene(Qt.vector3d(0,-0.35,0.05))
        return Qt.vector3d(99,105,-220)
    }
'''
s = s.replace('    // Resources', anchor+'\n    // Resources', 1)
TARGET.write_text('// Adapted from Ameer Studio, CC BY 4.0. See source/MODEL-LICENSE.md.\n' + s, encoding='utf-8')
print('Rigged four doors, four windows, front/rear trunk and lights:', TARGET)
