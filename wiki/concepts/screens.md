---
title: UI Screens
type: concept
updated: 2026-05-22
---

# UI Screens

Three top-level `displayMode` values cycled by **A button**: NORMAL, PET, INFO. Plus modal overlays (menu/settings/reset) and condition-driven screens (approval, passkey, clock).

## Cycling

```
A short press  → displayMode = (displayMode + 1) % 3   // NORMAL → PET → INFO → NORMAL
A long press   → menu (toggle)
B short press  → sub-page within INFO/PET, or scroll transcript in NORMAL
```

## NORMAL

Default. Pet full-size in top half; bottom shows either:
- **Transcript HUD** (`drawHUD`) — last 3 wrapped lines from `tama.entries`, newest row bright; older rows dim; B scrolls. Word-wrap at 21 chars via `wrapInto`. Gated by `settings.hud`.
- **Approval panel** (`drawApproval`) — when `tama.promptId[0]`. Shows `tool` name (size 2 if ≤10 chars, else size 1), wrapped `hint` (2 lines), elapsed seconds (turns red after 10s), `A: approve` (green) / `B: deny` (red), or `sent...` after response.

## PET (2 pages)

Buddy peeks at top (half-scale) via `characterSetPeek(true)`. Header: `<owner>'s <pet>  page/2`.

- **Page 0 — stats**: mood (hearts, 0..4), fed (10 pips, 5K tokens each), energy (5 bars), level pill, approved/denied/napped counts, total tokens, today tokens
- **Page 1 — how-to**: explains MOOD/FED/ENERGY/idle-off/buttons

## INFO (6 pages)

Same peek layout. Header: `Info  N/6` + section title.

| Page | Section    | Content                                                                       |
| ---- | ---------- | ----------------------------------------------------------------------------- |
| 0    | ABOUT      | "I watch your Claude desktop sessions…" + how the pet behaves                 |
| 1    | BUTTONS    | A / B / hold A / Power mappings                                               |
| 2    | CLAUDE     | sessions counts, link via (bt/usb/none), BLE state (encrypted/OPEN/-), last-msg age, current persona state |
| 3    | DEVICE     | battery %, V, mA, USB V; uptime, heap, brightness, BT linked status, temp     |
| 4    | BLUETOOTH  | linked/discover/off, btName, MAC, pairing instructions                        |
| 5    | CREDITS    | author, source URL, hardware                                                  |

`menuConfirm` Help/About jumps to pages 1/5 respectively.

## Modal overlays

Rendered on top of whichever screen.

### Menu

6 items: `settings`, `turn off`, `help`, `about`, `demo` (toggle), `close`. Rounded panel centered. A = next, B = confirm. Hint row at bottom shows `Next ↓  Change →`.

### Settings (10 items)

`brightness` (0..4 indicator), `sound`/`bluetooth`/`wifi`/`led`/`transcript` (on/off), `clock rot` (auto/port/land), `ascii pet` (`pos/total` indicator, cycles via `nextPet`), `reset`, `back`.

### Reset (3 items)

`delete char`, `factory reset`, `back`. Tap-twice confirm: first B-tap arms (label flips to `really?` in HOT color), second within 3s executes.

## Conditional screens (preempt everything)

### Approval panel

When `tama.promptId[0]`. Lives inside drawHUD region, only in NORMAL mode. Prompt arrival forces NORMAL + closes all modals.

### Passkey

When `blePasskey() != 0`. Replaces full sprite: large 6-digit number, "BLUETOOTH PAIRING" header, "enter on desktop:" footer. Beeps at 1800Hz on appearance.

### Clock face

When USB-powered + RTC-valid + no activity + no overlay. See [clock-face.md](clock-face.md). Pet sleeps underneath in portrait; landscape direct-to-LCD with pet at left.

## Power states

- **Awake** — normal rendering
- **Dimmed** — brightness 8/100 during nap (face-down)
- **Screen-off** — `M5.Axp.SetLDO2(false)`; 30s idle timeout (USB suppresses)
- **Napping** — face-down ≥15 frames; pauses rendering, accumulates `napSeconds`
- **Hard-off** — Power button held 6s (AXP hardware-managed)

## See Also

- [Main / UI](../firmware/main.md) — all draw functions
- [Seven States](seven-states.md)
- [Clock Face](clock-face.md)
- [Mood / Fed / Energy](mood-fed-energy.md)
