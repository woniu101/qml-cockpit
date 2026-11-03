# Windows 系统状态与蓝牙音频接入方案

评估日期：2026-11-02。依据：当时项目源码、Windows/Qt 官方文档及开发机只读硬件枚举。本文件保留初始评估。当前已实现三种来源、电脑电池映射与网络信息；2026-11-03 增加蓝牙只读状态与本地音乐输出选择，见 [对应实施验收](蓝牙状态与本地音频输出-验收.md)。手机蓝牙音频接收尚未实施。

> 后续定位调整：用户已选择“电脑作为车辆替身”，电脑电池可直接作为车辆 SOC 来源。因此本文件中要求电脑/车辆电量分别显示的建议已由 [整车数据映射与独立模拟器方案 v2](整车数据映射与独立模拟器方案-v2.md) 替代。蓝牙、网络和音频接口评估仍可参考。

## 1. 结论与范围

可以把顶部信息栏改为真实 Windows 设备状态，同时兼容笔记本和台式机。建议分两条实施：

1. 系统状态：电脑供电/电池、蓝牙能力与状态、当前网络类型，以及音频输出设备。
2. 手机蓝牙收音：让电脑作为音频接收端，接收手机媒体声音并通过电脑输出设备播放。

第二条需要独立的音频接收服务和真实硬件验证，不能通过更换蓝牙图标或读取配对设备列表完成。

顶部显示“电脑状态”；首页、车辆充电页和行驶页继续显示“车辆模拟状态”。系统电池与 Vehicle.batteryPercent 分别维护，电脑插电不会触发车辆模拟充电，模拟行驶也不会扣电脑电量。

## 2. 当前项目和开发机情况

- StatusBar 的百分比/电池条绑定 Vehicle.batteryPercent；蓝牙、Wi-Fi、移动信号是静态图标。
- 音乐由 QMediaPlayer + QAudioOutput 播放本地文件，目前音量只作用于本应用的本地播放器。
- 开发机只读枚举未发现系统电池和当前存在的蓝牙设备；发现处于 Up 状态的物理以太网适配器。此结果可作为台式机降级显示的测试样本，不代表其他设备的能力。
- 当前 Qt 6.12 Kit 未发现 QtBluetooth 头文件目录；Windows SDK 的 C++/WinRT 音频头文件存在。
- 目前没有进行手机配对、蓝牙音频接收或实体笔记本测试。

## 3. 信息栏与详情面板

顶部建议收敛为：

```text
时间 / 天气 / 驾驶状态                 电脑电源   蓝牙   当前网络   通知   用户
```

| 项目 | 顶部显示 | 点击后的详情 | 无能力/失败处理 |
|---|---|---|---|
| 电脑电源 | 笔记本“电脑 74%”及充电标记；无电池“外接电源” | 电源类型、电量、充电状态、节电状态；时间估计仅在系统提供时显示 | 无电池不显示 0% 或 100%；读取失败显示“状态未知” |
| 蓝牙 | 未提供能力、关闭、开启未连接、已连接等可区分状态 | 本机蓝牙状态、适用设备、音频接收连接；打开 Windows 蓝牙设置 | 无适配器不进入扫描循环，不把关闭误判为无硬件 |
| 网络 | 实际当前连接对应的有线/Wi-Fi/蜂窝图标 | 当前网络类型、系统报告的联网状态、可用的信号信息 | 未连接/未知分别表示；不能读取信号时不伪造满格 |
| 声音输出 | 保留底栏声音入口 | 默认输出名称、可选输出设备、跟随系统选项 | 无输出设备明确提示；拔掉耳机时按策略回退 |

蓝牙和网络均有状态文字/提示，不只依赖颜色。鼠标悬停可见简短说明，点击可见完整详情；保持触摸点击区域。

现有 Wi-Fi 与移动信号同时常亮应取消。蜂窝信号只在系统实际提供蜂窝连接时显示。有线和无线同时启用时，摘要使用系统推荐/当前连接，详情可以列出其他连接；VPN、虚拟网卡或无法确定的组合显示相应说明，不强行猜成有线。

## 4. 系统状态技术路线

新增 SystemStatus QObject/QML 单例，负责宿主设备状态，不并入 VehicleState。

### 4.1 电源

通过 Windows GetSystemPowerStatus 读取 AC/DC、充电、电量与节电状态。SYSTEM_POWER_STATUS 能区分无系统电池和未知百分比；未知值不当作正常数字使用。[S1][S2]

以系统实际能力判断是否显示电池，不按“笔记本/台式机”名称硬编码。订阅电源变化，配合低频校准；休眠恢复时刷新。状态事件处理结束后再在 Qt 主线程发出通知，避免 UI 阻塞。

### 4.2 网络

首版使用 Qt Network 的 QNetworkInformation，加载 Windows 后端并检查支持的 Feature。它提供连接类型和系统报告的可达性；该结果是提示信息，不能替代一次业务请求的真实成功/失败。[S3]

如果 Qt 后端不支持 TransportMedium，则通过 Windows ConnectionProfile 和适配器接口类型补充基本的有线/Wi-Fi 分类；仍无法判断时显示“类型未知”。

Wi-Fi 信号等细节通过 Windows 原生接口补充。支持时优先使用不含位置敏感数据的实时连接质量查询；旧系统或驱动不支持则显示“信号未知”。SSID/BSSID 等涉及的位置授权单独处理，不因拒绝授权阻断普通网络状态显示。[S4][S5]

不把读取网络状态扩展成自动联网或修改网络设置。首版提供 Windows 网络设置入口，连接与密码输入交给系统。网络详情不会写入 GitHub、日志或演示快照。

### 4.3 蓝牙状态

Windows 平台优先采用 C++/WinRT 的 BluetoothAdapter/Radio 与设备事件，读取适配器能力、开关状态和设备信息。探测无硬件、权限不足、服务错误、接口不支持分别处理。

蓝牙“已配对”“设备连接”“可进行媒体接收”需要分别建模。连接了鼠标或键盘不能显示“手机音乐已连接”。首版配对和系统开关使用 Windows 设置界面；应用不在启动时自动配对或开启无线电。

Windows 原生实现放在独立平台文件中，其他平台返回明确的 Unsupported 状态。本轮不要求为了基本状态显示额外安装 QtBluetooth；若后续要扩展跨平台设备管理，再评估该模块。[S6]

## 5. 手机蓝牙音频

### 5.1 区分音频方向

| 方向 | 目标 | 实施方式 |
|---|---|---|
| 本项目 → 蓝牙耳机/音箱 | 播放电脑本地音乐 | Windows 完成配对；本地播放器跟随默认输出，或选择对应 QAudioDevice |
| 手机 → 本电脑 | 电脑接收手机音乐/视频等媒体声音 | Windows A2DP 接收端，单独实现接收连接 |

第一种利用现有播放器扩展输出选择即可；第二种是本次需求的重点。Qt 的 QMediaDevices 可枚举输出并监听设备列表变化。[S7]

### 5.2 接收服务

采用 Windows.Media.Audio.AudioPlaybackConnection。微软文档明确支持远端设备向电脑播放声音，并由 Windows 的蓝牙音频组件处理接收与系统输出；该类从 Windows 10 2004 / build 19041 起提供。[S8][S9]

实施前先做小型原型：在当前未打包的 Qt 桌面 EXE 中验证 API 可用性、初始化、设备发现、权限与实际出声，再扩展产品界面。以 Windows 11 为主要验收环境，按运行时能力探测决定支持状态，不能只根据系统版本承诺成功。

首版流程：

1. 用户在 Windows 设置中完成手机配对。
2. 音乐页增加“本地音乐 / 蓝牙音频”音源切换。
3. 列出可用于音频接收的设备，选择一部手机并开启接收。
4. 异步创建、启用并打开连接，显示连接结果；失败给出重试入口。
5. 用户在手机上播放媒体，声音由电脑输出设备播放。
6. 切换回本地音源、断开手机、关闭应用或发生设备移除时，释放接收连接和事件订阅。

首版仅同时接收一部手机。切换到蓝牙时暂停本地 QMediaPlayer，避免双音源混播；断开后不擅自自动播放本地歌曲。

### 5.3 不能一并承诺的功能

- 成功配对不等于已建立媒体接收连接。
- 接收连接就绪不等于手机正在播放歌曲，不能仅凭连接状态显示“正在播放”。
- 曲名、封面、上一首/下一首、播放/暂停涉及额外的远端媒体控制能力，需另做兼容验证；首版以可靠接收媒体声音为目标。
- 手机通话/麦克风回传属于不同的通话音频需求，不纳入首版媒体接收范围。
- Android 与 iPhone 分别做实机验证；系统、驱动、适配器或手机不支持时明确提示，不显示模拟连接成功。

### 5.4 输出和音量

AudioDeviceService 管理本地播放器的输出列表、默认输出变化和设备断开。初期蓝牙接收采用 Windows 的系统输出路径；Qt 播放器中选择的设备不能假定会同时控制 Windows 接收音频的路由。

现有 Vehicle.volume 只控制本地播放器，不直接等于手机音量。蓝牙接收页应提供明确的“系统输出音量”入口；若实现 Windows 主音量控制，使用 Core Audio EndpointVolume，并在 UI 标明其作用范围。普通本地音量仍按应用音量处理，避免无提示地改变其他应用的响度。[S10]

## 6. 模块与更新策略

建议职责：

- SystemStatus：电源、网络、蓝牙能力与状态，带可用/未知/不支持状态和更新时间。
- BluetoothAudioService：可接收设备、单设备接收连接、异步状态机与资源释放。
- AudioDeviceService：Qt 本地播放输出设备与系统输出信息。
- VehicleState：继续负责车辆模拟 SOC、挡位、里程和车控。
- QML：顶部状态、系统详情面板和音乐页音源选择；UI 不运行 PowerShell/netsh 来持续抓取状态。

优先事件驱动；订阅变化并在 Qt 主线程更新，低频轮询仅作为校准或旧接口回退。禁止阻塞 UI 线程扫描设备。系统拒绝访问时给出原因和手动入口，避免重复弹系统提示。

默认真实系统状态；开发和截图测试使用独立模拟 Provider，提供电池、无电池、离线、无蓝牙、蓝牙断开等固定样本。若提供用户可选的演示模式，需要明确标注，系统数据失败时不能静默换回演示数字。

## 7. 分阶段交付与验收

| 阶段 | 交付 | 复杂度 / 验收条件 |
|---|---|---|
| A 真实系统状态 | 电源、蓝牙状态、当前网络、详情入口；清理静态假信号；车辆/电脑电量分离 | 低至中；笔记本/台式机、无硬件、离线、权限不足均有清楚显示 |
| B 本地音频输出 | 跟随默认输出、输出选择、耳机/音箱插拔恢复 | 中；本地音乐在实际输出设备上可听，系统默认变化可跟随 |
| C 手机蓝牙接收原型与界面 | Windows 原型验证后接入音源切换、手机选择、开启/断开/重试、接收状态与音量入口 | 较高；必须用真实蓝牙硬件与手机验证出声、断连、休眠恢复、关闭后释放 |
| D 远端媒体控制（可选） | 曲目信息和播放控制 | 后续单独评估，按实际手机/驱动能力显示 |

主要回归：

1. 电脑电池变化不会改变车辆模拟电量，模拟充电不会改变电脑电源显示。
2. 无电池显示外接电源；无蓝牙仍可使用本地音乐和车辆演示。
3. 有线/Wi-Fi 切换、断网、仅局域网、VPN 和未知接口不出现虚假满格信号。
4. 蓝牙配对、接收就绪、断连、失败和无权限可区分，不重复播音。
5. 手机媒体在电脑实际出声，手机断开或退出应用后不残留接收会话。
6. 输出设备拔插、无扬声器、休眠恢复、重复进入退出、主题和 125% DPI 均回归。

建议先完成 A 和 B；C 先验原型再开发完整音乐交互。当前开发机可以验证无电池、无蓝牙和有线网络分支；需要带可用蓝牙的笔记本/电脑及手机补充 C 的真实验收。保持当前开发流程，仅构建调试，不生成便携包。

## 官方资料

- [S1 GetSystemPowerStatus](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getsystempowerstatus)
- [S2 SYSTEM_POWER_STATUS](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-system_power_status)
- [S3 Qt QNetworkInformation](https://doc.qt.io/qt-6/qnetworkinformation.html)
- [S4 Windows Wi-Fi 访问与位置权限变化](https://learn.microsoft.com/en-us/windows/win32/nativewifi/wi-fi-access-location-changes)
- [S5 WLAN_INTF_OPCODE：实时连接质量](https://learn.microsoft.com/en-us/windows/win32/api/wlanapi/ne-wlanapi-wlan_intf_opcode)
- [S6 Qt Bluetooth](https://doc.qt.io/qt-6/qtbluetooth-index.html)
- [S7 Qt QMediaDevices](https://doc.qt.io/qt-6/qmediadevices.html)
- [S8 Windows AudioPlaybackConnection](https://learn.microsoft.com/en-us/uwp/api/windows.media.audio.audioplaybackconnection?view=winrt-26100)
- [S9 Windows 接收远端蓝牙音频指南](https://learn.microsoft.com/en-us/windows/apps/develop/media-playback/enable-remote-audio-playback)
- [S10 Core Audio EndpointVolume](https://learn.microsoft.com/en-us/windows/win32/coreaudio/endpointvolume-api)

[S1]: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getsystempowerstatus
[S2]: https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-system_power_status
[S3]: https://doc.qt.io/qt-6/qnetworkinformation.html
[S4]: https://learn.microsoft.com/en-us/windows/win32/nativewifi/wi-fi-access-location-changes
[S5]: https://learn.microsoft.com/en-us/windows/win32/api/wlanapi/ne-wlanapi-wlan_intf_opcode
[S6]: https://doc.qt.io/qt-6/qtbluetooth-index.html
[S7]: https://doc.qt.io/qt-6/qmediadevices.html
[S8]: https://learn.microsoft.com/en-us/uwp/api/windows.media.audio.audioplaybackconnection?view=winrt-26100
[S9]: https://learn.microsoft.com/en-us/windows/apps/develop/media-playback/enable-remote-audio-playback
[S10]: https://learn.microsoft.com/en-us/windows/win32/coreaudio/endpointvolume-api
