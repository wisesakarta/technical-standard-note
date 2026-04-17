[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath,
    [int]$MaxAttempts = 3,
    [int]$MinExpectedCases = 5,
    [string]$BenchmarkDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($MaxAttempts -lt 1) {
    throw "MaxAttempts must be >= 1."
}

if ($MinExpectedCases -lt 1) {
    throw "MinExpectedCases must be >= 1."
}

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    if (-not (Test-Path -LiteralPath $full)) {
        throw "Path not found: $full"
    }
    return (Resolve-Path -LiteralPath $full).Path
}

function Get-BenchmarkDirectory {
    param([string]$ConfiguredPath)
    if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath)) {
        $dir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ConfiguredPath)
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        return $dir
    }

    $defaultDir = Join-Path $env:LOCALAPPDATA "Otso\benchmarks"
    New-Item -ItemType Directory -Force -Path $defaultDir | Out-Null
    return $defaultDir
}

function Get-LatestBenchmarkReport {
    param([Parameter(Mandatory = $true)][string]$Directory)
    if (-not (Test-Path -LiteralPath $Directory)) {
        return $null
    }
    return Get-ChildItem -LiteralPath $Directory -Filter "benchmark-*.txt" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
}

function Parse-BenchmarkStatuses {
    param([Parameter(Mandatory = $true)][string]$ReportPath)

    $lines = Get-Content -LiteralPath $ReportPath -Encoding UTF8
    $results = New-Object System.Collections.Generic.List[object]
    $currentCase = $null
    foreach ($line in $lines) {
        if ($line -match "^Case:\s*(.+)$") {
            $currentCase = $Matches[1].Trim()
            continue
        }
        if ($line -match "^Status:\s*(.+)$" -and $null -ne $currentCase) {
            $status = $Matches[1].Trim()
            $results.Add([pscustomobject]@{
                    Case   = $currentCase
                    Status = $status
                }) | Out-Null
            $currentCase = $null
        }
    }

    return @($results)
}

$exeFull = Resolve-ExistingPath -Path $ExePath
$benchDir = Get-BenchmarkDirectory -ConfiguredPath $BenchmarkDir
$failures = New-Object System.Collections.Generic.List[string]

Write-Host "[benchmark-gate] Executable : $exeFull"
Write-Host "[benchmark-gate] Report dir : $benchDir"
Write-Host "[benchmark-gate] MaxAttempts: $MaxAttempts"

$passed = $false

for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
    $attemptStart = [DateTime]::UtcNow
    Write-Host "[benchmark-gate] Attempt $attempt/$MaxAttempts running --benchmark-ci ..."

    $exitCode = 0
    & $exeFull --benchmark-ci
    if (Test-Path Variable:\LASTEXITCODE) {
        $exitCode = [int]$LASTEXITCODE
    }

    $report = Get-LatestBenchmarkReport -Directory $benchDir
    if (-not $report) {
        $failures.Add("attempt ${attempt}: no benchmark report found in '$benchDir'") | Out-Null
        continue
    }

    if ($report.LastWriteTimeUtc -lt $attemptStart.AddSeconds(-1)) {
        $failures.Add("attempt ${attempt}: latest report '$($report.FullName)' is stale ($($report.LastWriteTimeUtc.ToString("o"))).") | Out-Null
        continue
    }

    if ($exitCode -ne 0) {
        $failures.Add("attempt ${attempt}: benchmark process exit code $exitCode (report: $($report.FullName)).") | Out-Null
        continue
    }

    $statuses = Parse-BenchmarkStatuses -ReportPath $report.FullName
    if ($statuses.Count -lt $MinExpectedCases) {
        $failures.Add("attempt ${attempt}: expected >= $MinExpectedCases benchmark cases, got $($statuses.Count) (report: $($report.FullName)).") | Out-Null
        continue
    }

    $nonPass = @($statuses | Where-Object { $_.Status -ne "PASS" })
    if ($nonPass.Count -gt 0) {
        $details = ($nonPass | ForEach-Object { "$($_.Case):$($_.Status)" }) -join "; "
        $failures.Add("attempt ${attempt}: non-pass statuses => $details (report: $($report.FullName)).") | Out-Null
        continue
    }

    Write-Host "[benchmark-gate] PASS on attempt $attempt. Report: $($report.FullName)"
    $passed = $true
    break
}

if (-not $passed) {
    Write-Host "[benchmark-gate] FAILED after $MaxAttempts attempt(s)."
    foreach ($f in $failures) {
        Write-Host "  - $f"
    }
    throw "benchmark gate failed"
}
