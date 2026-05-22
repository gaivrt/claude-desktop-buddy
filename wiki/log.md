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
