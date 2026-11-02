"""Rebuild the Qt model from the bundled original GLB; no Blender required."""
from pathlib import Path
import math
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MODEL = ROOT / 'assets/car/CarConcept.qml'
subprocess.run([
    r'D:\Qt\6.12.0\msvc2022_64\bin\balsam.exe',
    '-o', str(MODEL.parent), str(ROOT / 'assets/source/CarConcept.glb')
], check=True)
s = MODEL.read_text(encoding='utf-8')
s = s.replace('    id: root\n', '''    id: root
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
''', 1)
for node, expression in {
    'bodyDoorLColor1': 'Qt.vector3d(0, 0, root.leftDoorAngle)',
    'bodyDoorRColor1': 'Qt.vector3d(0, 0, root.rightDoorAngle)',
    'bodyHood': 'Qt.vector3d(root.hoodAngle, 0, 0)',
    'bodyRearPanelsColor1': 'Qt.vector3d(root.hatchAngle, 0, 0)',
}.items():
    s = s.replace(f'id: {node}\n', f'id: {node}\n            eulerRotation: {expression}\n', 1)
s = re.sub(r'(id: paint_1_Carmine_material\b.*?baseColor:) "[^"]+"', r'\1 root.paintColor', s, count=1, flags=re.S)
s = re.sub(r'(id: paint_1_Carmine_material\b.*?metalness:) [^\n]+', r'\1 0.5', s, count=1, flags=re.S)
s = re.sub(r'(id: paint_1_Carmine_material\b.*?normalMap:) [^\n]+', r'\1 null', s, count=1, flags=re.S)
for material, expression in {
    'headlight_material': 'root.lightsOn ? Qt.vector3d(3.5, 4, 5) : Qt.vector3d(0, 0, 0)',
    'brakelight_material': 'root.lightsOn ? Qt.vector3d(3, 0.02, 0.01) : Qt.vector3d(0, 0, 0)',
    'signallight_material': 'root.signalOn ? Qt.vector3d(5, 1.5, 0) : Qt.vector3d(0, 0, 0)',
}.items():
    s = re.sub(r'(id: ' + material + r'\b.*?emissiveFactor:) [^\n]+', lambda m: m[1] + ' ' + expression, s, count=1, flags=re.S)
MODEL.write_text(s, encoding='utf-8')

# A project-owned studio environment: neutral backdrop with large softboxes.
w, h = 512, 256
with (ROOT / 'assets/studio.hdr').open('wb') as f:
    f.write(f'#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y {h} +X {w}\n'.encode())
    for y in range(h):
        v = y / h
        for x in range(w):
            u = x / w
            light = 0.65 + 0.5 * max(0, 1 - v * 1.4)
            for center, width, power in [(0.2, .065, 7), (.72, .11, 5), (.48, .025, 3)]:
                du = min(abs(u-center), 1-abs(u-center))
                light += power * math.exp(-((du/width)**6 + ((v-.32)/.2)**6))
            rgb = (light * .91, light * .96, light)
            mantissa, exponent = math.frexp(max(rgb))
            scale = mantissa * 256 / max(rgb)
            f.write(bytes([min(255, int(c * scale)) for c in rgb] + [exponent + 128]))
print('Prepared animated vehicle and studio environment.')
