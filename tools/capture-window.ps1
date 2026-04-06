[CmdletBinding()]
param(
    [string]$ProcessName = "technical-standard-note",
    [string]$TitleContains = "",
    [string]$OutputPath = "",
    [int]$TimeoutSeconds = 10
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($TimeoutSeconds -lt 1) {
    throw "TimeoutSeconds must be >= 1."
}

Add-Type -AssemblyName System.Drawing

if (-not ("CaptureNative" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class CaptureNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
}
"@
}

function Resolve-OutputFile {
    param([Parameter(Mandatory = $true)][string]$CandidatePath)

    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($CandidatePath)
    $dir = Split-Path -Parent $full
    if (-not [string]::IsNullOrWhiteSpace($dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    return $full
}

function Get-LatestWindowProcess {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$TitleFilter
    )

    $matches = @(
        Get-Process -Name $Name -ErrorAction SilentlyContinue |
            Where-Object { $_.MainWindowHandle -ne 0 }
    )
    if (-not [string]::IsNullOrWhiteSpace($TitleFilter)) {
        $matches = @($matches | Where-Object { $_.MainWindowTitle -like "*$TitleFilter*" })
    }
    if ($matches.Count -eq 0) {
        return $null
    }

    return ($matches | Sort-Object StartTime -Descending | Select-Object -First 1)
}

$targetPath = $OutputPath
if ([string]::IsNullOrWhiteSpace($targetPath)) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $targetPath = ".\artifacts\screenshots\window-$stamp.png"
}
$targetPath = Resolve-OutputFile -CandidatePath $targetPath

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$targetProcess = $null
while ((Get-Date) -lt $deadline) {
    $targetProcess = Get-LatestWindowProcess -Name $ProcessName -TitleFilter $TitleContains
    if ($targetProcess) {
        break
    }
    Start-Sleep -Milliseconds 100
}

if (-not $targetProcess) {
    throw "No visible process found for name '$ProcessName' within $TimeoutSeconds second(s)."
}

$rect = New-Object CaptureNative+RECT
$ok = [CaptureNative]::GetWindowRect($targetProcess.MainWindowHandle, [ref]$rect)
if (-not $ok) {
    throw "GetWindowRect failed for process '$($targetProcess.ProcessName)' (pid $($targetProcess.Id))."
}

$width = [Math]::Max(0, $rect.Right - $rect.Left)
$height = [Math]::Max(0, $rect.Bottom - $rect.Top)
if ($width -le 0 -or $height -le 0) {
    throw "Invalid window bounds captured for process '$($targetProcess.ProcessName)' (pid $($targetProcess.Id))."
}

$bitmap = New-Object System.Drawing.Bitmap($width, $height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen(
        $rect.Left,
        $rect.Top,
        0,
        0,
        $bitmap.Size,
        [System.Drawing.CopyPixelOperation]::SourceCopy
    )
    $bitmap.Save($targetPath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}

Write-Host "Captured window screenshot: $targetPath"
