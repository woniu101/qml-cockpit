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

座舱助手的原生推理使用 [sherpa-onnx 2.0.1](https://github.com/k2-fsa/sherpa-onnx/tree/v2.0.1)（Apache-2.0）和该 Windows SDK 随附的 [ONNX Runtime 1.28.2](https://github.com/microsoft/onnxruntime/tree/v1.28.2)（MIT）。固定下载地址和 SHA-256 见 `scripts/setup-native-inference.ps1`，许可与上游声明位于 `licenses/`。ONNX Runtime C 接口按 API 20 调用。

自定义唤醒词的发音表由导出工具使用 pypinyin（MIT）生成，运行端仅读取导出表。模型权重与词表的许可独立于推理库；模型目录保留其来源和导出记录。程序不分发用户导入的参考音色或媒体文件。

## v0.2.0 Windows 便携包对应源码

- 应用源码与构建脚本：[QML Cockpit v0.2.0](https://github.com/woniu101/qml-cockpit/tree/v0.2.0)（GPL-3.0-only）。
- Qt DLL 和 QML 插件来自 Qt Online Installer 的 6.12.0 MSVC x64 SDK。下载 [Qt 6.12.0 完整源码](https://download.qt.io/archive/qt/6.12/6.12.0/single/qt-everywhere-src-6.12.0.tar.xz)，参见 [Qt 构建方法](https://doc.qt.io/qt-6.12/build-sources.html)及[第三方归属](https://doc.qt.io/qt-6.12/licenses-used-in-qt.html)。Qt Quick 3D 采用 GPLv3，其他模块按各自许可。随包提供相关许可及 Qt 模块 SPDX 清单。
- FFmpeg DLL 来自同一 Qt SDK，版本 9.0.1，采用 LGPL-2.1-or-later。下载 [FFmpeg 9.0.1 源码](https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz)，参见 [Qt 的 Windows 构建方法](https://doc.qt.io/qt-6.12/qtmultimedia-building-ffmpeg-windows.html)。该构建使用 [zlib 1.3.1](https://www.zlib.net/fossils/zlib-1.3.1.tar.gz)，通过 MSVC 构建共享库。FFmpeg DLL 可替换为同 ABI 的自编译版本。
- 附带的 Microsoft Visual C++ Redistributable 安装程序是独立的 Microsoft 组件，适用其随附条款。

## 实操演示片

视频含《七里香》原曲、MV 片段及模仿音色。项目的源码许可证不改变这些素材各自适用的权利。
