# Android Beta Release Checklist

## Versioning

1. `versionCode` incremented.
2. `versionName` uses `beta` suffix.
3. Changelog section prepared for tester-facing changes.

## Automated Gates

1. `.\gradlew.bat :app:assembleDebug`
2. `.\gradlew.bat :app:testDebugUnitTest`
3. `.\gradlew.bat :app:lintDebug`

All three must pass before beta tag/cut.

## Manual Smoke

1. Create, edit, close, and reopen tabs.
2. Open file via SAF, save, save-as.
3. Validate line ending and encoding status labels.
4. Verify auto-list continuation on numbered and bullet lines.
5. Verify `Go to line` and `Find` (`Next` / `Previous`).
6. Validate custom font load and persisted default font.
7. Check read-only behavior for medium/large files.
8. Validate light/dark mode structure consistency.
9. Validate external keyboard shortcuts:
   - `Ctrl+N`
   - `Ctrl+O`
   - `Ctrl+S`
   - `Ctrl+W`
   - `Ctrl+F`
   - `Ctrl+G`

## Sign-off

1. No crash in 10-minute continuous typing/editing session.
2. No unresolved P0 bug for data loss/corruption/crash-on-open.
3. APK installs and launches on emulator and at least one physical device.

