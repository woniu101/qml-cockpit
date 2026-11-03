# Vehicle protocol v1

本协议连接本机的车机与虚拟整车模型。默认服务名 `qml-cockpit.vehicle.v1`，使用 `QLocalServer` / `QLocalSocket`，Windows 上对应命名管道，仅允许当前用户访问。最多 8 个客户端；未在 3 秒内完成握手的连接会关闭。

两个仓库独立构建。模拟器的 `protocol/VehicleProtocol.h` 为协议副本基准，车机保存相同内容于 `src/protocol/VehicleProtocol.h`。修改格式时应同步两端并更新版本；不是通过引用平级仓库源码来建立依赖。

## 帧和握手

每帧为 **4 字节大端无符号长度 + UTF-8 JSON 对象**。JSON 最大 65536 字节，接收缓冲最多四帧长度；支持拆分帧和合并帧。所有对象必须含整数 `protocol: 1` 和字符串 `type`。长度、JSON 或版本无效则断开。

客户端先发：

```json
{"protocol":1,"type":"hello","role":"cockpit"}
```

服务端回 `hello`（含 `session`、`server`），然后发完整 `snapshot`。收到合法快照才算可控制。`session` 每次启动服务生成一个 UUID；快照 `sequence` 在该会话中递增。车机忽略旧会话和重复/逆序快照。

## 快照

```json
{
  "protocol":1,"type":"snapshot","session":"uuid","sequence":1,"sentAt":1793600000000,
  "state":{
    "soc":82,"gear":"P","speed":0,"tripKm":0,"forwardKm":0,
    "windows":[0,0,0,0],"targets":[0,0,0,0],"windowMoving":[false,false,false,false],
    "doors":[false,false,false,false],"doorMoving":[false,false,false,false],
    "locked":true,"hood":false,"hatch":false,"hoodMoving":false,"hatchMoving":false,
    "mirrors":false,"port":false,"plugged":false,"charging":false,
    "chargeTarget":90,"chargePower":0
  }
}
```

| 字段 | 单位及含义 |
|---|---|
| `soc` | 0–100%，小数；同一值驱动车机顶部、首页、补能页 |
| `gear` / `speed` | P/R/N/D；整数 km/h，P/N 必须 0，R ≤15，D ≤100 |
| `tripKm` / `forwardKm` | 非负公里，累计总行驶 / D 挡前进行驶；用于里程及本地示意导航 |
| `windows` / `targets` | 实际小数 / 目标整数开度 0–100%，0 全关、100 全开 |
| `windowMoving` | 四窗是否运动；数组顺序始终左前、右前、左后、右后 |
| `doors` / `doorMoving` | 车门打开状态 / 开合未完成，顺序同上 |
| `hood` / `hatch` | 前 / 后备厢打开状态；对应 Moving 字段表示开合尚未完成 |
| `mirrors` | true 表示折叠；D/R 自动展开 |
| `locked` / `port` / `plugged` / `charging` | 锁车 / 充电口打开 / 已插枪 / 正在充电 |
| `chargeTarget` / `chargePower` | 目标 50–100% / 当前 kW，未充电时 0 |

服务端约每 100 ms 发布一帧；状态变更、接受或完成指令时也可立即发布。当前是演示能量模型：15 kWh/100 km、60 kWh 电池、55 kW 充电，续航按车机既有固定系数估算。车门/备厢快照表示开合状态和执行标志，3D 角度在车机插值；车窗按实际小数开度驱动玻璃。

## 指令和回执

```json
{"protocol":1,"type":"command","session":"uuid","id":"request-uuid","kind":"setWindow","args":{"index":0,"target":50}}
```

回执为 `type: "result"`，含相同 `session`、`id`、`status`、`reason`。

- `accepted`：已应用目标，正在等待执行状态。
- `completed`：实际状态已达成；发送前先发布快照。充电开始/停止的完成表示相应开关已确认，不表示电池已充满。
- `rejected`：参数、驻车互锁或故障注入拒绝，车辆状态保持原值。
- `cancelled`：同一部件的新指令或控制台注入替换旧操作；停止车窗会取消旧升降任务。

车机不按点击乐观修改实际值；拖动只预览目标，释放后发送一次。无回执 1.8 秒超时，接受后 6 秒仍未完成则超时。超时是客户端状态，不是车辆停止指令；延迟的操作可能继续完成，之后仍以遥测为准，旧回执不再恢复过期请求。

每个连接缓存最近最多 256 个请求用于去重；相同 ID 和内容重发返回已缓存回执，同 ID 不同内容会断开。去重不跨重连，也不提供持久化或跨进程重启的恰好一次语义；车机不会自动重放未确认操作。

| kind | args |
|---|---|
| `setWindow` / `stopWindow` | `{index:0..3,target:0..100}` / `{index:0..3}` |
| `closeWindows` / `ventilateWindows` | `{}`；全关 / 15% 通风 |
| `setDoor` / `closeDoors` | `{index:0..3,open:boolean}` / `{}` |
| `setLocked` / `setHood` / `setHatch` / `setPort` / `setMirrors` | `{value:boolean}` |
| `setGear` / `setSpeed` | `{value:"P"或"R"或"N"或"D"}` / `{value:整数km/h}` |
| `startDriving` / `park` | `{}`；D 40 km/h / P 0 km/h |
| `setPlugged` / `setChargeTarget` | `{value:boolean}` / `{value:50..100整数}` |
| `startCharging` / `stopCharging` | `{}` |
| `advance` | `{seconds:0.01..3600}`；快进能量和里程，机械动作继续按真实时间运行 |

行驶前必须关闭车门、备厢、充电口并断枪，且开合已完成、电量大于 0。车门、备厢、充电口、后视镜仅 P 挡可操作。充电要求 P 挡、开口、插枪；充电中不能拔枪，插枪后不能关口。

插枪或充电时禁止切入 N/D/R，保持 P 挡。已处于连接或开口状态时，重复设置相同值是无副作用操作。降低充电目标至当前 SOC 以下会停止充电，实际 SOC 保持原值；耗尽电量时停止行驶，快进里程最多累计至剩余电量可支持的距离。

延迟指令在排队与真正执行时分别校验。重置车辆、断开客户端、停止服务会撤销尚未执行的延迟请求；同一部件的新目标、停止或控制台操作可以替换旧的排队操作，返回 cancelled。清除延迟故障只改变后续请求，已经排队的请求仍按原计划处理，除非被上述操作取消。

## 数据失效及恢复

3 秒没有新快照，或握手后 5 秒仍无完整快照，车机会关闭连接并约每秒重试。保留最后数据，禁用外部车辆控制，暂停道路/车轮动画，提示失联；音乐、空调、灯光继续在本地运行。恢复同一会话后按累计 `forwardKm` 补算断连期间的导航距离，新会话从新快照建立基准。

外部模式停止车机内部车辆模拟计时。切回内置演示恢复进入前的本地车辆状态，保留当前本地音乐、空调和显示设置。默认启动使用内置模式；可以通过 `--external` 主动选择外部模式。

## 测试控制通道

测试客户端以 `role: "operator"` 握手，发送 `type: "control"`、`id`、`action`、`args`，收到 `controlResult`（`id`、`ok`、`reason`）。此接口只用于本机控制台与测试，不作为量产车辆接口。

- `soc {value}`：注入 0–100% SOC。
- `window {index,value}`：注入 0–100 整数实际开度并停止该窗；覆盖在途任务。
- 其他车辆操作使用与上表相同的 action/args。
- `reset {}`：恢复模拟初值，取消在途任务；服务会话保持不变。
- `disconnect {}`：断开所有 cockpit 连接，operator 连接保留。
- `faults {pauseTelemetry?,reject?,drop?,delayMs?}`：可选布尔开关和 0–10000 ms 延迟。暂停遥测不停止车辆模型；drop 不执行也不回执；delay 推迟执行；reject 拒绝新请求。恢复遥测后重新发布完整快照。

模拟器不会回放历史轨迹，也没有 Windows 系统状态适配、CAN 接口或跨机器服务。协议 v1 的目标是稳定完成开发期的双进程演示与故障验证。
