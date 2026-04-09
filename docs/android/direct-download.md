# Android Direct Download Guide

Technical Standard Note Android is distributed via GitHub Releases (no Play Store dependency).

## For Users

1. Open repository Releases page.
2. Download latest:
   - `technical-standard-note-mobile-<version>-release.apk`
3. Verify SHA256 checksum:
   - compare with `technical-standard-note-mobile-<version>-release.apk.sha256`
4. Install APK (enable "Install unknown apps" for your browser/file manager if needed).
5. For updates:
   - download newer APK
   - install on top of existing app (same signing key required).

## Checksum Verify (Windows)

```powershell
certutil -hashfile .\technical-standard-note-mobile-0.9.0-beta2-release.apk SHA256
```

Compare output with hash from `.sha256` file.

## For Maintainers

Local publish flow (without Play Console):

```powershell
cd android
.\publish-release.ps1 -Tag v0.9.0-beta2 -Prerelease
```

This command will:
- build APK + AAB to `android/dist/` (unless `-SkipBuild`),
- generate/refresh SHA256 files,
- create or update a GitHub Release,
- upload APK artifacts by default (`.apk` + `.apk.sha256`).
- optional: include AAB with `-IncludeAab`.

Release artifacts are generated on tag push (`v*`) by GitHub Actions:
- workflow: `.github/workflows/build.yml`
- Android outputs attached to release:
  - `technical-standard-note-mobile-release.apk`
  - `technical-standard-note-mobile-release.aab`
  - `.sha256` files

If release signing secrets are missing, build uses debug-key fallback and is only suitable for internal testing.

Required secrets for production signing:
- `TSN_RELEASE_STORE_FILE_B64` (base64 content of `.jks`)
- `TSN_RELEASE_STORE_PASSWORD`
- `TSN_RELEASE_KEY_ALIAS`
- `TSN_RELEASE_KEY_PASSWORD`
