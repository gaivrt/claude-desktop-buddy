---
title: Governance — "fork it"
type: decision
source: CONTRIBUTING.md
updated: 2026-05-22
---

# Governance: This Repo Is a Reference, Not a Project

## Decision

The repo accepts only:
- **Corrections to `REFERENCE.md`** when protocol docs are wrong or unclear
- **Bugs that break the reference**: pairing failures, render failures, boot crashes

It rejects:
- New features, new pets, new screens
- Ports to other boards (fork instead)
- Refactors, style changes, dependency bumps

## Why

The contract is `REFERENCE.md`. The firmware is *one worked example* of speaking it. Anything beyond keeping the example faithful belongs in a fork — and fork is the explicitly preferred contribution path.

> "Swap the M5Stick for a Pi Pico W. Replace the ASCII pets with an e-ink panel. Put it in a 3D-printed shell. Rip out everything but `ble_bridge.cpp` and the JSON parser."

## Implication for wiki

This decision is the reason the wiki splits `protocol/` from `firmware/` with `protocol/` taking precedence. The protocol is the long-lived artifact. Firmware notes can become stale if a fork diverges — that's expected.

## Implication for changes

When ingesting changes, prefer:
- Updates to `protocol/` when REFERENCE.md changes
- Updates to `firmware/` when the C++ implementation changes
- **Don't propose "improvements"** to source code unrelated to documented protocol behavior — they won't be accepted upstream

## See Also

- [CONTRIBUTING.md](../../CONTRIBUTING.md)
- [Protocol vs Implementation](protocol-vs-implementation.md)
