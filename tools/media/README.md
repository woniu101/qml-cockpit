# 独立媒体验证工具

本目录是独立媒体验证工具，不属于正式车机运行时，也不使用 `VehicleState` 或接管当前蓝牙连接。

- `RadioStreamProbe`：静音播放输入清单中的真实 HTTP(S) 台源，使用 QAudioBufferOutput 确认有效解码帧；验证停止后无继续输出。受控本机 HTTP 端点用于取消等待和 404 行为。JSON 报告不写临时签名 URL，也不保存广播音频。
- `resolve_radio_sources.py`：从广播机构公开播放器配置解析候选源，使用标准库，无手机应用或第三方台源聚合器依赖。临时地址保存于 artifacts。
- `MediaDesignRender`：使用 Qt SVG 和本机字体导出设计稿、最小尺寸、125% 尺寸及总览，不启动正式车机。
- `VideoPlaybackProbe`：复用正式 `MediaSession`，对输入目录中的 MP4/AVI/MKV/WebM 静音验证实际视频帧、音频解码、跳转及续播，不依赖系统扬声器。每个文件独立统计，音频帧不混入下一个无音轨文件。

构建：在主仓库根目录执行 `scripts/build-media-tools.cmd`。台源实测：执行 `scripts/verify-radio.ps1`。详细结果及限制见 `design/media/README.md`。

直播地址可能失效，重复实验必须重新解析。阶段 1A 清单是独立验证输入；正式频道目录位于 `src/media/RadioService.cpp`，播放前由 `RadioSourceResolver` 重新解析。短时解码成功不代表长期可用或已完成听音测试。

视频验证示例（从仓库根目录运行，Qt bin 已在 PATH 中）：

```powershell
.\build\media-tools\VideoPlaybackProbe.exe D:\Media\TestVideos artifacts\media-refactor\video-report.json
```

本轮另使用 Qt 官方 qtmultimedia 测试数据中的 `3colors_with_sound_1s.mp4`（H.264/AAC）及 `color_matrix.mp4`（H.264 无音轨）。固定提交为 `98dc72f68d348ad12be73ae9e15a10e262dc889d`，路径为 `tests/auto/integration/qmediaplayerbackend/testdata/`；下载来源记录在本地 `artifacts/media-refactor/fixtures/qt-sources.json`。这些文件只用于本地验证，不纳入仓库，也不放入用户的视频库。可查阅 [Qt 原始测试目录](https://github.com/qt/qtmultimedia/tree/98dc72f68d348ad12be73ae9e15a10e262dc889d/tests/auto/integration/qmediaplayerbackend/testdata)。
