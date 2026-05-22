# SCHEMA — LLM Wiki

## Project

`claude-desktop-buddy` 是 Claude desktop app BLE Hardware Buddy 协议的**参考实现**：一个跑在 M5StickC Plus（ESP32 + Arduino）上的桌面宠物，通过 Nordic UART Service 接收 Claude 的会话/权限事件，并把权限决策回传。

定位特殊性：

- **`REFERENCE.md` 是稳定契约**——其他设备厂商可以只读它就独立实现，无需用到本仓库任何代码。
- 本仓库的固件代码是 *reference implementation*，治理上明确不接收新功能（见 `CONTRIBUTING.md`），fork 才是预期路径。
- 因此 wiki 中关于"协议"的页面权威性 **高于** "实现"的页面：当二者冲突时，按协议页修源码。

## Project Structure

| 路径 | 角色 |
|------|------|
| `REFERENCE.md` | **协议契约**：NUS UUIDs、JSON 消息族、folder push、security/pairing。权威 |
| `README.md` | 用户向：硬件、烧录、配对、控件、七状态、character pack 规约 |
| `CONTRIBUTING.md` | 治理：仅接受参考性修复（协议文档、参考实现 bug），新功能去 fork |
| `LICENSE` | MIT |
| `platformio.ini` | 构建配置：`m5stick-c` board、LittleFS、no_ota 分区、160MHz、Arduino framework |
| `.gitignore` | Build artifacts |
| `src/main.cpp` | 主循环、状态机、UI 屏幕（约 45KB，是入口） |
| `src/ble_bridge.{cpp,h}` | NUS GATT 服务、行缓冲 TX/RX、连接事件 |
| `src/character.{cpp,h}` | LittleFS 上的 GIF 加载/解码/渲染（AnimatedGIF lib） |
| `src/buddy.{cpp,h}` | ASCII 物种调度 + 渲染辅助 |
| `src/buddy_common.h` | ASCII buddy 共用辅助 |
| `src/buddies/*.cpp` | 18 种 ASCII 物种，每个文件实现 7 个动画函数 |
| `src/data.h` | 协议线缆解析、JSON 反序列化（ArduinoJson） |
| `src/xfer.h` | folder push 接收方（char_begin/file/chunk/file_end/char_end） |
| `src/stats.h` | NVS-backed 持久化（统计/设置/owner/species choice） |
| `characters/` | 范例 GIF 角色包；`bufo/` 是参考包 |
| `characters/<name>/manifest.json` | name + colors + 7 个 state→GIF 映射 |
| `tools/prep_character.py` | 把任意尺寸源 GIF 标准化到 96px-wide 同比例集合 |
| `tools/flash_character.py` | 跳过 BLE，直接把 character pack 通过 USB 烧到 LittleFS |
| `tools/test_serial.py`, `tools/test_xfer.py` | 串口/folder-push 调试脚本 |
| `docs/` | 仓库展示资产：`device.jpg`、`menu.png`、`hardware-buddy-window.png`、`manual.html` |

## Wiki Structure

```
wiki/
├── index.md            # 内容索引（必须，按分类组织）
├── log.md              # 操作日志（必须，append-only）
├── overview.md         # 项目全景
├── firmware/           # 固件模块（C++ 实现层）
├── protocol/           # BLE 协议消息族（契约层 — 权威）
├── buddies/            # ASCII 物种综述与单种页（按需）
├── tools/              # Python 工具
├── concepts/           # 跨切概念（NVS 布局、七状态、levels、character pack 规约…）
└── decisions/          # 设计决策（why NUS、why LittleFS+no_ota、治理策略…）
```

## Page Types

- **overview** — 项目全景，导览入口
- **module** — 一个固件模块（职责、关键 API/状态、与其他模块的耦合点、对应文件）
- **protocol-msg** — 一条协议消息/命令（方向、字段表、ack 契约、错误处理、对应 REFERENCE.md 段落）
- **buddy** — 一个 ASCII 物种（七态外观、风格定位、文件位置）
- **tool** — 一个 Python 工具（用途、输入/输出、典型调用、依赖）
- **concept** — 横切概念（七状态、levels & XP、character pack 格式、NVS keys…）
- **decision** — 设计决策（背景、可选项、结论、影响）

## Conventions

- 文件名：kebab-case（如 `ble-bridge.md`、`folder-push.md`）
- 内链：相对路径 markdown link `[页面名](path/to/page.md)`
- 每个 wiki 页面带 YAML frontmatter：
  ```yaml
  ---
  title: 页面标题
  type: module | protocol-msg | buddy | tool | concept | decision | overview
  source: 主要源文件相对路径（若适用）
  updated: YYYY-MM-DD
  ---
  ```
- 页面底部 `## See Also` 列出相关页面链接
- **协议页面权威**：`protocol/` 下页面与 `REFERENCE.md` 必须互相链接；若 `src/` 代码与协议页面冲突，以协议页面为准并在 `firmware/` 对应页面标注 `⚠ Conflict with protocol`
- 关于 magic numbers（4KB turn event limit、1.8MB folder limit、30s heartbeat timeout、50K tokens / level 等）必须在 `concepts/` 或 `protocol/` 页面集中维护，避免在多处复述

## Ingest Workflow

1. 读取 source 文件
2. 与用户讨论要点（除非用户要求静默处理）
3. 写新 wiki 页面或更新已有页面
4. **连锁更新**：检查相关 concept/overview/protocol 页是否受影响
5. 更新 `wiki/index.md`
6. 在 `wiki/log.md` 追加 `## [YYYY-MM-DD] ingest | <描述>`

特化：
- ingest `src/` 下的固件文件 → 写 `firmware/<name>.md`，并交叉链接到对应 `protocol/` 页
- ingest `REFERENCE.md` 段落 → 一段对应一个 `protocol/<msg>.md`
- ingest `tools/*.py` → 写 `tools/<name>.md`

## Query Workflow

1. 读 `wiki/index.md` 定位
2. 读相关 wiki 页面
3. 信息不足时回溯源文件
4. 回答用户
5. 有价值的新分析（对比、总结、发现）→ 询问用户是否存入 wiki

## Lint Checklist

- [ ] 页面间矛盾（特别是 `firmware/` ↔ `protocol/`）
- [ ] 过时信息（被新 source 取代的旧声明）
- [ ] 孤立页面（没有入链）
- [ ] 缺失页面（被引用但不存在的概念）
- [ ] 缺失交叉引用（每个 `firmware/` 页应链回相关 `protocol/` 页）
- [ ] magic number 分散（应集中在 `concepts/` 或 `protocol/`）
- [ ] `REFERENCE.md` 内容与 `protocol/` 页面是否同步

## Log Format

```markdown
## [YYYY-MM-DD] operation | description

简要说明做了什么、影响了哪些页面。
```
