# 蓝牙曲目信息：AVRCP 核验与回退记录

日期：2026-11-03。约束：只使用 Windows 和蓝牙，不安装手机配套应用；调试开发，不生成便携包。

## 当前结果

按用户“继续验证，如果实在实现不了就回退到原本的连接界面”的要求，继续检查后仍未取得 vivo X200 Pro 的真实歌名、歌手、进度或封面，已恢复原有蓝牙连接界面。删除实验性的 PhoneMedia / WindowsPhoneMedia、来源选择面板及其专用测试目标，避免将其他 Windows 应用的会话作为手机曲目展示。

保留 BluetoothAudio 的音频接收、连接与断开、本地播放互斥、输出丢失处理；保留本地音乐库、目录监听、队列、随机历史、歌词及实测音源兼容修复。MediaSessionProbe 和 AvrcpTransportProbe 只作为独立诊断工具，不随正常车机启动采样媒体会话。

AVRCP 协议支持曲目信息，但当前没有验证出本机应用可用、能与现有音频接收共存的读取通道。此结果不等于 Windows 不支持 AVRCP，也不证明所有 Windows 硬件均无法实现。

## 实机与接口证据

| 核验 | 结果与限制 |
|---|---|
| 手机音频 | 原有 AudioPlaybackConnection 已接通 brave的vivo X200 Pro。该 API 提供连接状态，没有歌曲字段。通道已接通不等于已验证扬声器实际出声。 |
| 手机正在播放并切歌时的系统会话 | 用户确认后，15:12:32 只读查询成功，但 sessions=[]；记录 artifacts/music/phone-playing-sessions.json。15:58 重连后仍为空。GSMTC 是系统媒体会话入口，不是直接 AVRCP 接口。 |
| 本地传输对照 | AF_BTH / SOCK_STREAM / BTHPROTO_L2CAP 可创建 socket，但 bind 返回 10050 / WSAENETDOWN；RFCOMM 对照绑定成功。未连接远端或发送遥控命令。创建 socket 不能作为 AVRCP 通道成功的证据。记录 artifacts/devices/avrcp-transport-capability.json。 |
| 驱动共存前提 | 两个 vivo Avrcp 传输节点状态 OK，使用 Microsoft_Bluetooth_AvrcpTransport，INF 为 microsoft_bluetooth_avrcptransport.inf，未列出 UpperFilters / LowerFilters；服务运行。记录 artifacts/devices/avrcp-driver-ownership.json。已有驱动持有相关设备节点，尚未证明自定义驱动或第二条通道可共存。 |
| 本机 DLL 导出 | BthAvrcp.dll 仅导出 DllCanUnloadNow / DllGetClassObject；BthAvrcpAppSvc.dll 仅导出服务入口；BthAvctpSvc.dll 为 COM / 服务入口。没有可直接调用的歌曲读取函数。记录 artifacts/devices/avrcp-bridge-exports.txt。导出表本身不能排除私有 COM 接口。 |
| 注册的 AVRCP COM 类 | 本机存在 Bluetooth AVRCP Profile COM Server（208F7A6F-FB35-4686-9494-AB22B7B2CD78），只读激活 IUnknown 成功；标准 IDispatch 和 IInspectable 查询均返回 0x80004002 / E_NOINTERFACE。没有调用未知私有接口。该测试只能排除这两类通用访问方式，不能证明没有私有接口。记录 artifacts/devices/avrcp-com-check.txt。 |
| SDK 与驱动路径 | 本机公开 SDK 未找到可用于本项目的 AVRCP 元数据读取 API，未安装 WDK 内核头文件。微软公开 Winsock 蓝牙协议为 RFCOMM；L2CAP 驱动接口使用 BRB，需要另外验证驱动入口及共存。 |

未替换驱动、修改配对、服务配置或签名策略。没有为此安装第三方蓝牙栈。

## 已撤回的实验

上一轮 Windows 媒体会话适配可从明确选择的系统应用读取测试曲目，但未取得手机曲目。其原生正向验证、样本界面和历史报告保留在 artifacts 中，仅为当时的实验记录；PhoneMediaNativeCheck、PhoneMediaTests 和“歌曲信息来源”入口已删除，不能作为当前功能或手机验收结果。

当前音乐页只展示手机名称和连接状态，明确提示“此连接未提供歌名、进度或封面”；播放、切歌及音量由手机操作。首页展示蓝牙标识和手机连接入口，不使用本地曲目冒充手机信息。

## 若以后继续 AVRCP

需要先在独立原型中证明有文档的驱动/SDK 桥接接口能够与当前 A2DP Sink、AvrcpTransport、BthAvctpSvc 共存，并从真实手机取得 GetElementAttributes、GetPlayStatus 与通知。此后才能接回 QML。封面还需要独立验证 Cover Art / BIP 支持，不能由元数据成功推定。

## 复验

```powershell
.\scripts\build.cmd
& 'D:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure
.\build\MediaSessionProbe.exe
.\scripts\verify-devices.ps1
.\scripts\verify-devices.ps1 -Scale 1.25
.\scripts\verify-music.ps1 -Scale 1.25
.\scripts\run.cmd --device-qml-check
```

## 回退后验证结果

- Release 构建完成；CTest 6/6 组通过。
- 离屏实例化 7 个设备/音乐相关 QML 组件，绑定警告为零。
- 设备界面在 100% 和 125% 缩放下通过；覆盖连接/取消/断开、本地播放互斥、实际 Qt 输出切换和输出丢失处理。手机连接后端和热插拔拓扑在此自动测试中为受控样本，不是手机实机验收。
- 本地播放器 125% 界面回归通过，含队列、随机历史、目录移除、歌词及最小窗口；独立启动恢复到 6000 ms 并保持暂停的检查通过。
- 16:16:54 独立探测仍重现 RFCOMM 绑定成功、L2CAP 绑定 10050；系统会话仍为空。此时未额外要求用户重新播放，手机正在播放时的证据以上述 15:12:32 记录为准。
- 16:20 调试版重新启动后，通过原有按钮实机重新接通 vivo X200 Pro；面板显示“手机音频已接通”，连接按钮禁用，首页恢复蓝牙标识和手机名称。未进行扬声器听音验收。未生成便携包、提交或推送代码。

## 官方依据

- [Bluetooth SIG AVRCP 1.6.3](https://files.bluetooth.com/wp-content/uploads/dlm_uploads/2024/10/AVRCP_v1.6.3.pdf)
- [AudioPlaybackConnection](https://learn.microsoft.com/en-us/uwp/api/windows.media.audio.audioplaybackconnection)
- [GSMTC 会话](https://learn.microsoft.com/en-us/uwp/api/windows.media.control.globalsystemmediatransportcontrolssession)
- [Windows Bluetooth socket](https://learn.microsoft.com/en-us/windows/win32/bluetooth/bluetooth-and-socket)
- [Windows 蓝牙驱动栈](https://learn.microsoft.com/en-us/windows-hardware/drivers/bluetooth/bluetooth-driver-stack)
- [L2CAP 驱动示例](https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/bluetooth-echo-l2cap-profile-driver/)
