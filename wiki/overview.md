---
title: Overview
type: overview
updated: 2026-05-22
---

# Claude Desktop Buddy — Overview

## 一句话

Claude desktop app 通过 BLE Nordic UART Service 把会话状态、转录片段、权限提示推送到一个 ESP32 桌面宠物（M5StickC Plus），用户在硬件上一键 approve/deny；本仓库是该协议的**参考实现**。

## 两层定位

仓库实际上由两个相互独立的层组成：

1. **协议层（契约）** — `REFERENCE.md`
   稳定面。任意能广播 NUS 并解析 newline-delimited JSON 的设备（Arduino、ESP32、nRF52、带 BLE dongle 的 Raspberry Pi…）都可以独立实现，**不需要本仓库任何代码**。
2. **参考实现层** — `src/`、`characters/`、`tools/`
   一个具体设备的代码：M5StickC Plus + AnimatedGIF + ArduinoJson + LittleFS。治理上明确**不接受新功能**（见 `CONTRIBUTING.md`），fork 是预期路径。

理解这一点对 wiki 维护至关重要：`wiki/protocol/` 是规范，`wiki/firmware/` 是一种实现的注解；二者冲突时按规范修代码。

## 硬件 / 构建

- **Board**：M5StickC Plus（ESP32，135×240 portrait，IMU、按钮 A/B、power 按键）
- **Framework**：Arduino（PlatformIO `m5stickc-plus` env）
- **CPU**：160 MHz
- **Filesystem**：LittleFS（角色包存在 `data/` → 设备上）
- **Partitions**：`no_ota.csv`（最大化用户分区，放弃 OTA）
- **关键库**：`M5StickCPlus`、`bitbank2/AnimatedGIF`、`bblanchon/ArduinoJson`
- **烧录**：`pio run -t upload`；首次或换设备 `pio run -t erase && pio run -t upload`

## 协议鸟瞰（详见 `protocol/`）

- **Transport**：Nordic UART Service（UUID `6e400001-...`）；UTF-8 JSON、`\n` 分隔；MTU 边界自行重组
- **Heartbeat snapshot**：desktop → device，状态变更时 + 每 10s 心跳；超过 ~30s 无消息视为断连
- **Turn events**：每个 turn 结束触发；包含 SDK content array；>4KB（UTF-8 字节）丢弃
- **Permission**：`prompt.id` 出现时 device 回 `{"cmd":"permission","id":...,"decision":"once"|"deny"}`
- **One-shot on connect**：`{time:[epoch, tz_offset_sec]}` 和 `{cmd:"owner",name:...}`
- **Commands & acks**：`status`、`name`、`owner`、`unpair`；每条 cmd 期望对应 ack
- **Folder push**：drop folder → `char_begin / file / chunk / file_end / char_end`，base64 chunks，<1.8MB
- **Security**：推荐 LE Secure Connections 绑定（AES-CCM 加密）；`sec:true` 标志在 status ack；`unpair` 命令清 bond

## 实现鸟瞰（详见 `firmware/`）

| 模块 | 职责 |
|---|---|
| `main.cpp` | 主循环、状态机、UI 屏幕（normal / pet / info / approval / menu / settings） |
| `ble_bridge.cpp` | NUS GATT 服务，行缓冲 TX/RX，连接事件 |
| `data.h` | 解析 heartbeat / turn / commands；状态聚合 |
| `xfer.h` | folder push 接收方，写入 LittleFS |
| `character.cpp` | LittleFS 上加载 manifest.json + GIF，按 state 渲染 |
| `buddy.cpp` + `buddies/*.cpp` | 18 物种的 ASCII 调度与逐物种的 7 态动画 |
| `stats.h` | NVS 持久化：tokens、approvals、deny、velocity、nap、level、species choice、owner、display name、settings |

## 七状态

| State | Trigger | Feel |
|---|---|---|
| `sleep` | bridge 未连 | 闭眼、慢呼吸 |
| `idle` | 已连、无紧急 | 眨眼、东张西望 |
| `busy` | session 正在生成 | 出汗、忙碌 |
| `attention` | 待审批权限 | 警觉、LED 闪烁 |
| `celebrate` | 升级（每 50K tokens） | 撒花、蹦跳 |
| `dizzy` | 摇晃设备 | 螺旋眼、晃动 |
| `heart` | 5 秒内 approve | 飘心 |

## Character packs

- 文件夹 + `manifest.json`（name、colors、7 个 state→GIF 映射）
- GIF 宽 96px、高 ≤ ~140px、总大小 < 1.8MB
- state 值可为单 filename 或 array；array 在 loop-end 轮换（适合 idle carousel）
- `bufo/` 为参考包；`tools/prep_character.py` 自动归一化尺寸；`tools/flash_character.py` USB 直烧
- 远程通过 BLE folder push；本地 `pio run -t uploadfs`

## 控件矩阵

|                  | Normal | Pet | Info | Approval |
|---|---|---|---|---|
| **A**（前） | 下一屏 | 下一屏 | 下一屏 | **approve** |
| **B**（右） | 滚动 transcript | 翻页 | 翻页 | **deny** |
| **Hold A** | menu | menu | menu | menu |
| **Power** 短按 | 屏幕开关 | | | |
| **Power** 长按 ~6s | 硬关机 | | | |
| **摇晃** | dizzy | | | — |
| **倒扣** | nap（能量补充） | | | |

屏幕 30s 无交互息屏（待审批时常亮）；任意按键唤醒。

## 启用方式

桌面应用默认禁用 BLE bridge：
1. **Help → Troubleshooting → Enable Developer Mode**
2. **Developer → Open Hardware Buddy…**
3. **Connect** 并选择设备；首连授予系统蓝牙权限

只有开发者模式可用，不是官方支持的产品功能。

## See Also

- `REFERENCE.md` — 协议契约（权威）
- `README.md` — 用户向使用说明
- `CONTRIBUTING.md` — 治理策略
- `wiki/protocol/`（待 ingest）
- `wiki/firmware/`（待 ingest）
