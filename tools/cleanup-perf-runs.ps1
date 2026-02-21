[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$RunsRoot = ".\research\perf-runs",
    [int]$KeepLatest = 2,
    [string[]]$KeepRunIds = @(),
    [switch]$DeleteCorpusFromKeptRuns
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($KeepLatest -lt 0) {
    throw "KeepLatest must be >= 0."
}

$root = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($RunsRoot)
if (-not (Test-Path -LiteralPath $root)) {
    throw "Runs root not found: $root"
}

$runDirs = @(
    Get-ChildItem -LiteralPath $root -Directory |
        Where-Object { $_.Name -like "run-*" } |
        Sort-Object Name -Descending
)

if ($runDirs.Count -eq 0) {
    Write-Host "No run-* directories found under $root"
    exit 0
}

$keepSet = New-Object "System.Collections.Generic.HashSet[string]" ([System.StringComparer]::OrdinalIgnoreCase)
for ($i = 0; $i -lt [Math]::Min($KeepLatest, $runDirs.Count); $i++) {
    $null = $keepSet.Add($runDirs[$i].Name)
}
foreach ($id in $KeepRunIds) {
    if (-not [string]::IsNullOrWhiteSpace($id)) {
        $null = $keepSet.Add($id.Trim())
    }
}

$deleted = New-Object System.Collections.Generic.List[string]
$kept = New-Object System.Collections.Generic.List[string]
$freedBytes = 0L

foreach ($run in $runDirs) {
    $size = (Get-ChildItem -LiteralPath $run.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
    if ($null -eq $size) { $size = 0L }

    if ($keepSet.Contains($run.Name)) {
        $kept.Add($run.Name) | Out-Null
        if ($DeleteCorpusFromKeptRuns) {
            $corpus = Join-Path $run.FullName "corpus"
            if (Test-Path -LiteralPath $corpus) {
                $corpusSize = (Get-ChildItem -LiteralPath $corpus -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
                if ($null -eq $corpusSize) { $corpusSize = 0L }
                if ($PSCmdlet.ShouldProcess($corpus, "Remove kept-run corpus")) {
                    Remove-Item -LiteralPath $corpus -Recurse -Force
                    $freedBytes += [int64]$corpusSize
                }
            }
        }
        continue
    }

    if ($PSCmdlet.ShouldProcess($run.FullName, "Remove old perf run directory")) {
        Remove-Item -LiteralPath $run.FullName -Recurse -Force
        $deleted.Add($run.Name) | Out-Null
        $freedBytes += [int64]$size
    }
}

$freedMB = [Math]::Round($freedBytes / 1MB, 2)
Write-Host "Perf-runs cleanup complete."
Write-Host "Kept runs   : $($kept.Count)"
Write-Host "Deleted runs: $($deleted.Count)"
Write-Host "Freed size  : ${freedMB} MB"

if ($deleted.Count -gt 0) {
    Write-Host ""
    Write-Host "Deleted:"
    $deleted | Sort-Object | ForEach-Object { Write-Host " - $_" }
}
