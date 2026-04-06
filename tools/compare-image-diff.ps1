[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BaselinePath,
    [Parameter(Mandatory = $true)]
    [string]$CurrentPath,
    [string]$DiffOutputPath = "",
    [double]$MaxDifferentPixelRatio = 0.01,
    [int]$ChannelTolerance = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($MaxDifferentPixelRatio -lt 0.0 -or $MaxDifferentPixelRatio -gt 1.0) {
    throw "MaxDifferentPixelRatio must be between 0.0 and 1.0."
}

if ($ChannelTolerance -lt 0 -or $ChannelTolerance -gt 255) {
    throw "ChannelTolerance must be between 0 and 255."
}

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    if (-not (Test-Path -LiteralPath $full)) {
        throw "Path not found: $full"
    }
    return (Resolve-Path -LiteralPath $full).Path
}

function Resolve-OutputPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    $dir = Split-Path -Parent $full
    if (-not [string]::IsNullOrWhiteSpace($dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    return $full
}

Add-Type -AssemblyName System.Drawing

$baseline = Resolve-ExistingPath -Path $BaselinePath
$current = Resolve-ExistingPath -Path $CurrentPath

if ([string]::IsNullOrWhiteSpace($DiffOutputPath)) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $DiffOutputPath = ".\artifacts\image-diff\diff-$stamp.png"
}
$diffPath = Resolve-OutputPath -Path $DiffOutputPath

$baselineBitmap = [System.Drawing.Bitmap]::new($baseline)
$currentBitmap = [System.Drawing.Bitmap]::new($current)
$diffBitmap = $null

try {
    if ($baselineBitmap.Width -ne $currentBitmap.Width -or $baselineBitmap.Height -ne $currentBitmap.Height) {
        throw "Image size mismatch. Baseline=$($baselineBitmap.Width)x$($baselineBitmap.Height), Current=$($currentBitmap.Width)x$($currentBitmap.Height)"
    }

    $width = $baselineBitmap.Width
    $height = $baselineBitmap.Height
    $diffBitmap = [System.Drawing.Bitmap]::new($width, $height)

    $differentPixels = [int64]0
    $totalPixels = [int64]$width * [int64]$height
    $highlightColor = [System.Drawing.Color]::FromArgb(255, 255, 140, 0)

    for ($y = 0; $y -lt $height; $y++) {
        for ($x = 0; $x -lt $width; $x++) {
            $a = $baselineBitmap.GetPixel($x, $y)
            $b = $currentBitmap.GetPixel($x, $y)

            $changed =
                [Math]::Abs($a.R - $b.R) -gt $ChannelTolerance -or
                [Math]::Abs($a.G - $b.G) -gt $ChannelTolerance -or
                [Math]::Abs($a.B - $b.B) -gt $ChannelTolerance -or
                [Math]::Abs($a.A - $b.A) -gt $ChannelTolerance

            if ($changed) {
                $differentPixels++
                $diffBitmap.SetPixel($x, $y, $highlightColor)
                continue
            }

            $gray = [byte](($a.R + $a.G + $a.B) / 3)
            $diffBitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $gray, $gray, $gray))
        }
    }

    $diffBitmap.Save($diffPath, [System.Drawing.Imaging.ImageFormat]::Png)

    $ratio = if ($totalPixels -eq 0) { 0.0 } else { [double]$differentPixels / [double]$totalPixels }
    $ratioPercent = [Math]::Round($ratio * 100.0, 4)
    $thresholdPercent = [Math]::Round($MaxDifferentPixelRatio * 100.0, 4)

    Write-Host "Baseline       : $baseline"
    Write-Host "Current        : $current"
    Write-Host "Diff image     : $diffPath"
    Write-Host "Image size     : ${width}x${height}"
    Write-Host "Different px   : $differentPixels / $totalPixels"
    Write-Host "Different ratio: $ratioPercent% (max $thresholdPercent%)"

    if ($ratio -gt $MaxDifferentPixelRatio) {
        throw "Image diff gate failed. Different ratio $ratioPercent% exceeds max $thresholdPercent%."
    }

    Write-Host "Image diff gate passed."
}
finally {
    if ($diffBitmap) { $diffBitmap.Dispose() }
    $baselineBitmap.Dispose()
    $currentBitmap.Dispose()
}
