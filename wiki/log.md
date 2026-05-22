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
