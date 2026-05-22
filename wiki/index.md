# Wiki Index

<!-- LLM 维护的内容索引。每个页面一行：链接 + 单行摘要 -->

## Overview

- [Overview](overview.md) — 项目全景：协议层 vs 参考实现层 / 硬件 / 七状态 / 字符包

## Protocol (REFERENCE.md — authoritative)

- [Transport (NUS)](protocol/transport.md) — UUIDs, 帧格式（UTF-8 JSON + `\n`）, MTU, 广播
- [Heartbeat Snapshot](protocol/heartbeat.md) — desktop→device 状态推送（每 10s + 状态变更时）
- [Turn Events](protocol/turn-event.md) — 每个 turn 完成时的一次性事件；>4KB 丢弃
- [Permission Decisions](protocol/permission.md) — device 回 `{cmd:permission,id,decision}`
- [On-Connect One-Shots](protocol/on-connect.md) — `time` 同步 + `owner` 名字
- [Commands & Acks](protocol/commands-acks.md) — `status` / `name` / `owner` / `unpair` + ack 契约
- [Folder Push](protocol/folder-push.md) — `char_begin/file/chunk/file_end/char_end`，<1.8MB
- [Security & Pairing](protocol/security.md) — LE Secure Connections + Bonding + `unpair`

## Firmware Modules

- [BLE Bridge](firmware/ble-bridge.md) — NUS GATT 服务 + 加密绑定 + 行缓冲 TX/RX；HID 共用同 server
- [BLE HID](firmware/ble-hid.md) — BLE 键盘服务，挂在同一个 BLEServer 上，驱动 Voice 页 ASR 热键
- [Data](firmware/data.md) — JSON 解析 + `TamaState` + demo/live/asleep 模式
- [Xfer](firmware/xfer.md) — folder push 接收 + 非心跳命令（status/name/owner/species/unpair）
- [Character](firmware/character.md) — LittleFS GIF / text 模式渲染 + peek + landscape
- [Buddy](firmware/buddy.md) — 18 物种 ASCII 调度 + 5fps tick + 1×/2× scale
- [Stats](firmware/stats.md) — NVS 持久化（tokens/level/velocity/settings/petname/owner/species）
- [Main / UI](firmware/main.md) — 主循环、状态机、按键、屏幕、菜单、时钟面

## Concepts

- [Seven Persona States](concepts/seven-states.md) — sleep/idle/busy/attention/celebrate/dizzy/heart
- [UI Screens](concepts/screens.md) — NORMAL/PET/INFO/VOICE + menu/settings/reset + approval/passkey/clock
- [ASR Integration](concepts/asr-integration.md) — Voice 页热键映射（mac 右 Cmd / win Win+H / 长按 Enter）
- [Character Pack Format](concepts/character-pack.md) — manifest schema、GIF/text 模式、尺寸约束
- [NVS Layout](concepts/nvs-layout.md) — `"buddy"` namespace 所有 keys + 写入纪律
- [Levels & XP](concepts/levels-xp.md) — 50K tokens/级 + bridge restart 处理 + 边沿持久化
- [Mood / Fed / Energy](concepts/mood-fed-energy.md) — PET 屏三个 gauge 的推导
- [Clock Face](concepts/clock-face.md) — USB-charging 时取代主屏；横竖两种姿势

## Buddies (ASCII Species)

- [Buddies Index](buddies/index.md) — 18 物种表 + 文件位置 + bufo 范例

## Tools

- [prep_character.py](tools/prep_character.md) — 跨态归一化 96px GIF 流水线
- [flash_character.py](tools/flash_character.md) — USB `uploadfs` 直接烧角色包
- [test_serial.py](tools/test_serial.md) — 用 USB JSON 循环四种状态
- [test_xfer.py](tools/test_xfer.md) — 模拟 folder push 协议 over serial

## Decisions

- [Governance](decisions/governance.md) — 为什么只接 reference 性修复，新功能去 fork
- [Protocol vs Implementation](decisions/protocol-vs-implementation.md) — 两层独立的稳定性边界
