<p align="center"><img src="assets/branding/logo.png" width="128" alt="QML Cockpit 标志"></p>
<h1 align="center">QML Cockpit</h1>
<p align="center"><strong>用 Qt / QML 构建的智能座舱 Demo</strong></p>

---

<p align="center">
  <a href="https://github.com/woniu101/qml-cockpit/releases/tag/v0.2.0"><img src="https://img.shields.io/badge/Release-v0.2.0-527761" alt="Release v0.2.0"></a>
  <img src="https://img.shields.io/badge/Windows-x64-357ABD" alt="Windows x64">
  <img src="https://img.shields.io/badge/Qt-6.12-41CD52" alt="Qt 6.12">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-blue" alt="GPL-3.0"></a>
</p>

---

<p align="center">
  <a href="#快速开始">下载</a> · <a href="#实操演示">实操演示</a> · <a href="#主要功能">主要功能</a> · <a href="https://github.com/woniu101/qml-cockpit-simulator">整车模拟器</a>
</p>

## 快速开始

下载 [Windows x64 便携包](https://github.com/woniu101/qml-cockpit/releases/download/v0.2.0/QmlCockpit-v0.2.0-windows-x64.zip)，解压后运行 `QmlCockpit.exe`。单独运行时使用内置车辆数据。联调时先启动[整车模拟器](https://github.com/woniu101/qml-cockpit-simulator/releases/tag/v0.2.0)，再在车机顶栏选择「外部模拟器」。

## 实操演示

https://github.com/user-attachments/assets/ce32dbd2-25b4-4e8e-ae83-a713da0a0295

片中展示车窗联调、导航驾驶、充电、语音调温和媒体播放。蓝牙采用拍摄模拟，语音指令通过预录音频输入。

[在哔哩哔哩观看 82 秒实操演示片](https://www.bilibili.com/video/BV1ewW46FEaL/)

## 主要功能

- **3D 车控与补能**：控制门窗、后视镜与充电口，查看实时开度和充电状态。
- **导航与驾驶**：离线示意路线，支持挡位、车速、里程和车辆状态联动。
- **座舱助手**：支持唤醒词、按键收音、语音识别、连续对话、车控指令和语音播报，可自定义参考音色。
- **媒体中心**：支持本地音乐与歌词、本地视频、网络电台及蓝牙音乐。
- **模拟器联调**：双向交换车速、电量、门窗状态和指令回执。

![车机首页，显示「周先生」](docs/screenshots/home.png)

<details>
<summary>查看座舱助手界面</summary>

![座舱助手](docs/screenshots/assistant.png)

</details>

## 从源码构建

使用 Qt 6.12、MSVC 2022 x64 和 CMake。依次运行 `scripts/setup-native-inference.ps1`、`scripts/build.cmd` 和 `scripts/run.cmd`。首次构建请按本机安装路径调整 CMake 预设及脚本。

便携包可直接体验文字助手。唤醒、识别与播报需要另配本地模型和参考音色。开发环境已配置模型时，用 PowerShell 运行 `scripts/run-assistant.ps1`。参见[语音助手配置](design/原生语音运行端-架构与验收.md#运行与依赖)。

## 许可

项目源码采用 [GPL-3.0-only](LICENSE)。第三方依赖、3D 模型及演示片中的素材各自适用其许可，详见[第三方声明](THIRD_PARTY_NOTICES.md)。
