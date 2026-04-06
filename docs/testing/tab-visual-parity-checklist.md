# Tab Visual Parity Checklist

Goal: keep dark mode tab strip structure identical to light mode (color-only variation), and catch flicker/layout regressions before release.

## Scope

- Tab strip background + divider continuity
- Active tab box geometry
- Inactive tab spacing
- Active/inactive text alignment
- No unexpected close glyph or floating divider line

## Capture Baseline

1. Open app in light mode with at least 3 tabs (active in the middle).
2. Capture screenshot:

```powershell
pwsh -NoProfile -File .\tools\capture-window.ps1 `
  -ProcessName technical-standard-note `
  -OutputPath .\artifacts\screenshots\tabs-light-baseline.png
```

3. Switch to dark mode with the same tab order and active tab index.
4. Capture screenshot:

```powershell
pwsh -NoProfile -File .\tools\capture-window.ps1 `
  -ProcessName technical-standard-note `
  -OutputPath .\artifacts\screenshots\tabs-dark-current.png
```

## Compare

Run pixel diff gate (tolerant to anti-aliasing noise):

```powershell
pwsh -NoProfile -File .\tools\compare-image-diff.ps1 `
  -BaselinePath .\artifacts\screenshots\tabs-light-baseline.png `
  -CurrentPath .\artifacts\screenshots\tabs-dark-current.png `
  -DiffOutputPath .\artifacts\image-diff\tabs-light-vs-dark.png `
  -ChannelTolerance 10 `
  -MaxDifferentPixelRatio 0.08
```

Note: this is a structure parity gate, not a color parity gate. Adjust threshold only if layout is intentionally changed.

## Manual Acceptance

1. Active tab has same box model in both modes (padding, height, border thickness).
2. Divider lines touch tab strip baseline (no floating gap).
3. Inactive tabs are visually subordinate but still legible.
4. No close `X` glyph appears when close button feature is disabled.
5. Hover and active transitions do not flicker during fast mouse movement.

## Regression Policy

1. If diff gate fails, inspect the generated diff image first.
2. If differences are structural, block release until fixed.
3. If differences are intentional and reviewed, update baseline screenshot and commit both baseline and rationale.
