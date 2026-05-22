# Voice — 把 Buddy 当 Windows 听写按钮用

> 适用于 Windows 10/11。macOS 用户切到 settings → host os → mac 后用法相同，只是热键不一样。

固件除了原本的 Claude 权限审批以外，多了一个 BLE HID 键盘服务。Windows 把 Buddy 当成普通蓝牙键盘配对之后，新增的 **Voice 屏** 就是一个一键听写触发器：

- **B 短按** → Windows 自带听写浮窗（等价于键盘 `Win+H`）
- **B 长按 ≥ 0.5 秒** → 提交（等价于 `Enter`）

Claude desktop 通过同一个 BLE 连接照常跑 NUS 心跳/权限/folder push，两套不冲突。

---

## 1. 准备

- 已经按 README 把固件烧进 M5StickC Plus（`pio run -t upload`）
- 开机后能看到宠物正常显示
- Windows 10/11，自带蓝牙或外接 BLE 适配器

如果你之前配过旧版本，建议先在 Windows 蓝牙设置里把它「移除设备」，再做下面的配对，让 OS 重新识别 HID 服务。

## 2. 与 Windows 配对

**步骤**：

1. 在 Buddy 上按一下任意键唤醒屏幕
2. Windows：**设置 → Bluetooth 和设备 → 添加设备 → Bluetooth**
3. 列表里找 `Claude-XXXX`（XXXX 是 MAC 后两字节）
4. 选中 → Buddy 屏会出现 6 位 passkey
5. 在 Windows 对话框输入这 6 位数 → 确定
6. 配对完成。Windows 设备列表里看到 **「Claude-XXXX (键盘)」**，类型是 *Input Devices / Keyboard*

**这一步对了的标志**：Windows 蓝牙里的设备图标是键盘，不是泛用蓝牙图标。如果是泛用图标，说明 HID Appearance 没被识别，重启 Buddy 再试一次。

> **配对一次就够**。重启 Buddy / Windows 后会自动重连，passkey 不会再次要求。

## 3. 切到 Voice 屏

Voice 是 A 按钮循环里的第 4 屏：

```
A 短按 →  NORMAL → PET → INFO → VOICE → NORMAL → ...
```

按 3 次 A 就到。看到的应该是：

```
┌─────────────────┐
│ Voice       win │  ← 右上角显示当前 host os
│                 │
│   [宠物 peek]    │
│                 │
│       🎤        │  ← 麦克风图标（body 色 = 已联通）
│                 │
│      linked     │  ← 绿色 = HID 已订阅
│                 │
│ B: dictate      │
│        hold B:  │
│           send  │
└─────────────────┘
```

如果状态行写的是 **"pair in OS settings"**（灰色）→ 见 §6 故障排查 1。

## 4. 实际使用：Win+H 听写

1. 把光标停在任意可输入文本的位置 —— 记事本、Word、聊天框、浏览器地址栏、Slack、微信、VS Code 都行
2. 按一下 Buddy 的 **B 按钮**（前面 A、右侧 B，按右边那个）
3. 屏幕右下角出现 Windows 听写浮窗，麦克风开始监听
4. 说出你要写的内容 —— 字会直接出现在原本的文本框里
5. 说完后：
   - 想继续编辑，让浮窗自己消失即可
   - 想直接提交（发消息 / 搜索 / 回车），**长按 B ≥ 0.5 秒** → Enter 触发

短按和长按的区分是在 B 松开时判定的：

- 按下后立刻松开 → 听写
- 按住超过 500ms 时已经发出 Enter（你应该听到一声较高的提示音），松开后不再触发听写

## 5. 切回 mac 模式（可选）

如果你把这个固件烧到了将来要给 Mac 用的设备上：

1. Buddy 长按 A 进入菜单 → settings → **host os**
2. 按 B 切换到 **mac**（默认是 win）

切换后 B 短按发送的是 **Right Cmd**（HID `0xE7`），这是 SuperWhisper / WhisprFlow 等 mac ASR 工具最常用的默认热键。长按依然是 Enter。

设置写进 NVS，重启保留。

## 6. 故障排查

### 1. Voice 屏写着 "pair in OS settings"，B 按下没反应

设备还没和 Windows 完成配对，或者配对被某一边忘记了。
- 检查 Windows 蓝牙设置里是否还有 "Claude-XXXX (键盘)"
- 没有 → 回到 §2 重做配对
- 有但状态显示 "未连接" → 按 Buddy 任意键唤醒，等 5-10 秒会自动重连

### 2. 屏幕显示 "linked"，但 Win+H 没动静

热键路径本身有可能没开。先用键盘直接按 Win+H 试试 —— 如果键盘按了也没反应，说明 Windows 的听写功能没启用：

**Windows 设置 → 时间和语言 → 听写**（或搜「voice typing」）→ 把开关打开，授权麦克风。

启用一次以后，Buddy 的 B 短按就和你直接敲 Win+H 是同一件事了。

### 3. 听写浮窗不在我光标所在的窗口

Win+H 的语音输入是「跟随焦点」的：浮窗打开时光标在哪个文本框，字就会落到哪个文本框。如果你按 B 时焦点在桌面、任务栏、或一个非文本控件上，浮窗会弹但没法输入。先点一下要写的文本框，再按 B。

### 4. Windows 蓝牙搜不到设备

- Buddy 屏幕是不是黑的？按任意键唤醒
- Buddy 里：长按 A → settings → bluetooth 是不是 on？
- 距离太远 / 干扰多 → 靠近电脑 1 米内

### 5. Claude desktop 连不上 / 心跳断了

HID 和 NUS 共用一个 BLE 连接，没有竞争 —— 但如果你刚做完 Windows 配对，Claude desktop 那一侧需要重新 connect 一次：

**Claude → Help → Troubleshooting → Enable Developer Mode → Developer → Open Hardware Buddy → Connect**

之后两个通道都正常。

### 6. 长按 B 没触发 Enter（听写浮窗收不掉）

少数情况下 Windows 听写浮窗会自己抢焦点。如果是这种情况，先点一下原文本框拿回焦点，再 B 长按。

### 7. 配对成功，但状态显示 "OPEN" 不是 "encrypted"

这意味着 BLE 链路没加密，理论上转录片段会在空中明文流。这不该发生 —— 我们的配置要求 SC+MITM+Bond。

修复：Buddy → settings → reset → factory reset（点两下确认）→ 重启 → 重新配对。

## 7. 进阶：用第三方 ASR 工具

`Win+H` 是开箱即用，但 transcribe 质量取决于 Windows 内置模型。如果你想接 [WisprFlow](https://wisprflow.ai/)、[Whispering](https://github.com/braden-w/whispering) 这类基于 Whisper 的工具：

- 大部分这类工具允许你绑定**自定义全局热键**
- 把它绑成 `Win+H`，就直接复用了 Buddy 现在的逻辑（推荐做法，零代码）
- 或者改 `src/main.cpp` 里 Voice 页 B 按钮的 `hidSendKey(0x08, 0x0B)` 那行，换成你的工具的热键 keycode

HID 修饰符 / keycode 对照速查：

| 你想发的键 | modifier 字节 | keycode |
|---|---|---|
| `Win+H` | `0x08` (Left GUI) | `0x0B` (H) |
| `Ctrl+Shift+;` | `0x01\|0x02` | `0x33` (;) |
| `F13` | `0x00` | `0x68` |
| 右 Cmd 单击 | `0x00` | `0xE7` (Right GUI) |
| Enter | `0x00` | `0x28` |

完整 keycode 表见 USB HID Usage Tables, Keyboard/Keypad Page (0x07)。

---

如果上面的步骤都对了但还有问题，记一下：

1. Buddy 屏幕显示的具体状态（"linked" / "pair in OS settings" / "OPEN"）
2. Buddy 上设置里 `host os` 是 mac 还是 win
3. Windows 设备里看到的设备类型（键盘 / 泛用蓝牙）
4. 直接键盘按 Win+H 能不能弹听写

带着这四条去 issue 区基本就能定位。
