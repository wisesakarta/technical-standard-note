# Android Roadmap

## Goal

Build a mobile companion that keeps the same product DNA:
- lightweight,
- clean and understandable,
- plain-text first.

Status legend:
- `[x]` done
- `[-]` in progress / partial
- `[ ]` not started

## Phase A - Baseline

- [x] Standalone Android directory (`android/`).
- [x] Compose app shell.
- [x] Basic multi-tab editor baseline.
- [x] Status bar baseline.

## Phase B - Core quality

- [x] Move list continuation logic to shared behavior.
- [x] Add file open/save via Storage Access Framework.
- [x] Session restore for last opened tabs.
- [-] Theme parity (light/dark structure consistency).
- [x] In-app theme switch (`Auto`, `Light`, `Dark`) with persisted preference.
- [x] Persist editor font settings (family + size).
- [x] Load custom font from device storage (TTF/OTF).
- [x] In-document utilities (`Go to line`, `Find next/prev`).

## Phase C - Performance and reliability

- [ ] Startup/memory profiling budget with measurable thresholds.
- [x] Crash-safe autosave snapshots.
- [-] Fuzz tests for encoding and line endings (baseline unit tests done, fuzz expansion pending).
- [x] Large-file behavior policy for mobile limits.

## Phase D - UX polish

- [x] Keyboard shortcut support (external keyboard: `Ctrl+N/O/S/W/F/G`).
- [-] Better tab gestures and reordering (command-based move left/right done; drag-reorder pending).
- [-] Refine typography and spacing with `DESIGN_COMPASS.md`.
- [x] Keyboard/input stability for external picker flows (no editor lift when `Load Font` returns).
- [x] GitHub-release pipeline for direct download (APK + checksum).

## Phase E - Beta release readiness

- [x] Beta versioning + changelog discipline (`0.9.0-beta3`).
- [x] Reliability gates baseline (unit tests + codec round-trip tests).
- [-] Manual smoke suite completion and sign-off:
  - open/save/save-as
  - tab lifecycle/new/close/reorder
  - font cycle + load custom font
  - find/go-to-line flows

## Next Sprint (Priority)

1. Finalize theme parity so dark/light share identical structure with color-only differences.
2. Complete performance budget:
   - cold start target,
   - memory target while editing medium and large files,
   - regression script for CI.
3. Complete full smoke sign-off on emulator + at least one physical Android device.
4. Add drag-based tab reorder (touch-first) with no data-loss edge cases.
5. Prepare `v0.9.0-beta2` release notes and artifact publication.
