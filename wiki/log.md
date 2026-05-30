# Wiki Log

## [2026-05-22] init | Wiki 初始化

创建 `SCHEMA.md`、`wiki/index.md`、`wiki/log.md`、`wiki/overview.md`。

基于项目探索定义了 wiki 子目录（`firmware/`、`protocol/`、`buddies/`、`tools/`、`concepts/`、`decisions/`）与 7 种 page type（overview / module / protocol-msg / buddy / tool / concept / decision）。

特化约定：`protocol/` 权威优先于 `firmware/`，冲突以 `REFERENCE.md` 为准。

## [2026-05-22] ingest | 全量首次 ingest

读取并 ingest 了所有 source 文件，一口气建出 25 个 wiki 页面。

**Protocol (8 pages)** — 完整对齐 REFERENCE.md：
- transport, heartbeat, turn-event, permission, on-connect, commands-acks, folder-push, security

**Firmware (7 pages)** — 全部 src/ 模块：
- ble-bridge, data, xfer, character, buddy, stats, main

**Concepts (7 pages)** — 横切关注点集中：
- seven-states, screens, character-pack, nvs-layout, levels-xp, mood-fed-energy, clock-face

**Buddies (1 page)** — 18 物种综述（buddies/index.md），未单建逐物种页（性价比低）

**Tools (4 pages)** — Python 工具：
- prep_character, flash_character, test_serial, test_xfer

**Decisions (2 pages)** — 治理与分层：
- governance (CONTRIBUTING.md 总结), protocol-vs-implementation

**Magic numbers 集中处**：
- 4KB turn event limit → `protocol/turn-event.md`
- 1.8MB folder cap → `protocol/folder-push.md` + `concepts/character-pack.md`
- 30s heartbeat timeout → `protocol/heartbeat.md`
- 50K tokens/level → `concepts/levels-xp.md`
- 96px GIF width → `concepts/character-pack.md`
- 18 species + index → `buddies/index.md`

**关键交叉引用**：
- 所有 `firmware/` 模块页都反向链接到相关 `protocol/` 页（契约层 → 实现层映射）
- `firmware/main.md` 是 hub，链向 6 个其他 firmware 页 + 5 个 concept 页
- `concepts/nvs-layout.md` 集中所有 NVS keys，`firmware/stats.md` 不重复表格

**`wiki/index.md` 完整重写**，包含所有 25 个页面的一行摘要，按子目录分组。

## [2026-05-23] ingest | Windows ASR HID feature (gan-hid-asr team, CP1-CP6)

新增 BLE HID 键盘扩展，把 Buddy 变成 Windows dictation (Win+H) 的一键触发器。保留 macOS（右 Cmd）作为可切换的双模式（NVS `s_host`）。

**新文件**：
- `src/ble_hid.{cpp,h}` — BLE HID + DIS + Battery 服务，挂在 NUS bridge 同一 BLEServer 上
- `wiki/firmware/ble-hid.md` — HID 模块文档
- `wiki/concepts/asr-integration.md` — 热键映射 + 双模式 + 配对 UX

**编辑**：
- `src/ble_bridge.{cpp,h}` — 暴露 `bleGetServer()` + `bleStartAdvertising()`，推迟 startAdvertising 让 HID UUID 进广播
- `src/stats.h` — `Settings.hostOs`（默认 win），NVS key `s_host`
- `src/main.cpp` — `DISP_VOICE` 第 4 屏、`drawVoice` + `drawMicIcon`、settings "host os" 项、B 按钮在 Voice 页改 release-based + 500ms long-press → Enter
- `wiki/firmware/{ble-bridge,main,stats}.md`、`wiki/concepts/{screens,nvs-layout}.md`、`wiki/decisions/protocol-vs-implementation.md` 反映新连线

**GAN review trail**：6 个 checkpoint，c1 + c5 REVISE→PASS（c1 漏了 HID 加密 perms + CCCD 订阅检查；c5 漏了 wake-swallow 一致性），其余一次通过。Pattern 沉淀在 `.review/log.md`。

**关键决策**：
- HID 是 fork-specific 扩展，不影响 REFERENCE.md 协议契约（见 `decisions/protocol-vs-implementation.md` Non-spec 段）
- HID 服务始终在线，但 B 按键仅在 Voice 页路由到 HID — 其他屏 B 保持原行为
- 双模式而非纯 Windows fork：盖尔可能未来切回 macOS 设备时立即可用

## [2026-05-30] ingest | Voice ASR via Buddy mic（方案 B，Phase 0 排雷）

新增 fork 功能设计：用 Buddy 自己的 SPM1423 麦克风录中文，离线 FunASR 转写，SendInput 注入 Claude desktop 输入框。区别于已有的 Win+H 触发（方案 A）。

**新文件**：
- `wiki/decisions/voice-asr.md` — 方案 B 决策页：三硬约束（ESP32 跑不了 ASR / 音频必须外送 / CJK 必须 PC 端 SendInput 注入）、双通道传输（USB 主 / BLE 次）、**BLE 与 Claude desktop 抢单连接的坑**、组件事实、Phase 0 验证结果、分阶段计划
- `spikes/mic/`（固件 + listen.py）、`spikes/asr/`（transcribe.py）、`spikes/inject/`（inject.py）— Phase 0 排雷代码

**编辑（chain-update）**：
- `wiki/decisions/protocol-vs-implementation.md` — Non-spec extensions 增加 voice-asr 一条
- `wiki/concepts/asr-integration.md` — 新增"Relation to 方案 B"段 + See Also 交叉链接，标注 Win+H 路径被 fork 取代
- `wiki/index.md` — Decisions 区新增 voice-asr 条目

**Phase 0 排雷结果（全 PASS）**：
- 0a 麦克风：peak 7329/32767、rms 1583、0 静音/削顶 → SPM1423 音质可用
- 0b FunASR：真实 Buddy 录音转出「你好，好久不见。」准确带标点，0.6s/3s，模型加载 ~52s（companion 须常驻）
- 0c 注入：中文成功落进 Claude desktop（修 WinError 87：INPUT 结构须 40B 全 union + 设 argtypes）

**关键决策**：
- 方案 B 必然需要 PC companion（CJK 注入只能 PC 端做）；现有 BLE HID/Win+H 在此用例被取代
- USB 有线是干净主路径（与 Claude desktop 的 BLE 链路物理隔离）；BLE 无线（Phase 2）须先解决"和 Claude desktop 共享 BLE 外设"冲突
- 先排雷（Phase 0）再搭链路，每个未知数独立验证

## [2026-05-30] ship + lint | Voice ASR 方案 B 上线 + wiki 对齐

Phase 1 完成并整合进主固件，端到端可用：hold B → SPM1423 麦克风 → IMA-ADPCM 4:1 → USB 串口 115200（带帧同步）→ PC companion 解码 → FunASR 转写 → SendInput 注入 + 自动回车发送进 Claude desktop。

**实现 vs 设计阶段的偏差（lint 修正项）**：
- **波特率**：高波特率（921600 / 1Mbaud，160/240MHz）在本板 FTDI 上全乱码 → 退回可靠的 115200 + IMA-ADPCM 压缩塞进带宽（硬件坑，换板子可省 ADPCM）
- **删 HID**：原 BLE HID 键盘在 Windows 抢设备、挤掉 Claude desktop 的 NUS 连接（"连上就掉"）→ `hidInit`/`hidTick` 从 startBt/loop 移除，纯 NUS
- **传输**：只做 USB（Phase 2 BLE 无线已放弃：抢 Claude desktop 的单连接 + BLE 带宽紧）
- **UI**：删电平条、麦克风下移居中（用户反馈）；**自动发送**：注入后自动 Enter

**新文件**：`src/voice_capture.h`、`companion/`（main.py + inject.py，含自动重连）、`start-voice.bat` / `start-voice-hidden.vbs`、`docs/chinese-voice-input.html`（小白安装指南，设计系统风格）。

**wiki 更新**：重写 `decisions/voice-asr.md`；新建 `firmware/voice-capture.md`；`concepts/asr-integration.md`、`firmware/ble-hid.md`、`decisions/protocol-vs-implementation.md` 标注 HID 已移除；更新 `concepts/screens.md`、`firmware/main.md`、`index.md`、`README.md`。

**lint 结论**：过时项已全部对齐；asr-integration / ble-hid 保留为历史并标注移除原因，非孤立。
