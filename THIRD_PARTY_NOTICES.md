# 素材与依赖说明

## 当前 3D 车辆

Tesla 2018 Model 3 by **Ameer Studio** (https://sketchfab.com/uchiha.321abc).

Original: https://sketchfab.com/3d-models/tesla-2018-model-3-5ef9b845aaf44203b6d04e2c677e444f

License: **Creative Commons Attribution 4.0 International**, https://creativecommons.org/licenses/by/4.0/

GLB obtained from https://github.com/nesdesignco/FormDrive/blob/main/public/models/tesla-model-3-2018.glb with the attribution notice in that repository's `public/models/TESLA-LICENSE.md`. The model license is separate from the repository application's MIT license. No React application code is bundled in this Qt project.

Modifications: converted with Qt Balsam to QML / mesh / texture resources; added bindings to the original door, glass, front and rear trunk pivots, selected-part material highlighting and camera focus; adjusted white paint, dark trim, lens and light materials; transformed texture orientation, expanded grayscale PNGs to RGB without changing pixel values, and added exterior / interior cameras. Mirror triangles were separated from the front-door meshes into new pivot nodes, preserving their original vertex attributes, UVs and materials. The original GLB is retained; the derived GLB and split report are in `assets/sedan/source/derived/` and `parts-rig.json`. A simplified charge-port flap, connector and cable were added as project-authored procedural geometry. No endorsement by Tesla or the artist is implied; model marks belong to their owners.

The original attribution notice and download SHA-256 are in `assets/sedan/source/`; the package includes them in `licenses/`, together with the full CC BY 4.0 text.

v6 adds rotation bindings to the original tire, rim, hub and brake-disc meshes around their source axle; the caliper and steering-link meshes remain stationary. The road plane and lane markings are project-authored procedural geometry.

## 静态设计素材

车辆二维外观图由内置 imagegen 根据本项目提示词生成，无品牌标识。原图和生成记录保存在 `design/v1/assets/vehicle-pearl.png` 与 `design/v1/vehicle-prompt.md`。

界面 SVG 图标和音乐封面由本项目代码绘制。本地环境音乐由 `scripts/prepare_audio.py` 合成，无外部录音或商业歌曲素材。Microsoft YaHei 字体使用运行电脑提供的系统字体，未随包复制字体文件。

## 保留的概念车技术样例

Car Concept model and textures © 2024 Darmstadt Graphics Group GmbH, artist Eric Chadwick.

Source: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/CarConcept

License: Creative Commons Attribution 4.0 International, https://creativecommons.org/licenses/by/4.0/

Modifications: converted with Qt Balsam to QML, mesh and texture resources; added state properties and part rotations; adjusted paint and light materials. This model is only used by the separate legacy 3D preview.

Khronos and 3D Commerce marks in that source asset remain the marks of their owners; their separate notice is preserved in `assets/source/Khronos-marks.txt` and the packaged licenses directory. No endorsement is implied.

The original model license metadata and download SHA-256 are preserved in `assets/source/`.

## Qt 和媒体组件

座舱助手的原生推理使用 sherpa-onnx 2.0.1（Apache-2.0）和该官方 Windows SDK 随附的 ONNX Runtime（MIT）。源码分别位于 https://github.com/k2-fsa/sherpa-onnx 与 https://github.com/microsoft/onnxruntime 。固定下载地址和 SHA-256 见 `scripts/setup-native-inference.ps1`，本地 SDK 来源记录在 `.deps/assistant-native/sources.json`。ONNX Runtime C 接口按 API 20 调用，使用 SDK 随附的兼容运行库，禁止混用 Python 虚拟环境中的 DLL。

自定义唤醒词的发音表由导出工具使用 pypinyin（MIT）生成，运行端仅读取导出表。模型权重与词表的许可独立于推理库；模型目录保留其来源和导出记录。程序不分发用户导入的参考音色或媒体文件。

运行时使用 Qt 6.12 模块，以及随 Qt Multimedia 部署的 FFmpeg 组件。相关文件保留原文件名与版权。Qt 本机许可文本位于 `D:\Qt\Licenses`，打包时一并复制到 `licenses/Qt`。各模块的具体许可以对应文本为准。
