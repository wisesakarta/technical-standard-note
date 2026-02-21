[CmdletBinding()]
param(
    [string]$LegacyExe = ".\build\mingw-debug\saka-note.exe",
    [string]$MsNotepadExe = "$env:WINDIR\System32\notepad.exe",
    [string]$OutputDir = ".\research\perf-runs",
    [string]$CorpusDir = "",
    [int]$Iterations = 3,
    [int]$SampleSeconds = 8,
    [int]$SampleIntervalMs = 500,
    [int]$WarmupMs = 3000,
    [switch]$SkipLegacyInternalGate,
    [switch]$NoFileRegeneration,
    [switch]$ForceCloseBeforeEachRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Iterations -lt 1) { throw "Iterations must be >= 1." }
if ($SampleSeconds -lt 2) { throw "SampleSeconds must be >= 2." }
if ($SampleIntervalMs -lt 100) { throw "SampleIntervalMs must be >= 100." }
if ($WarmupMs -lt 0) { throw "WarmupMs must be >= 0." }

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    if (-not (Test-Path -LiteralPath $full)) {
        throw "Path not found: $full"
    }
    return (Resolve-Path -LiteralPath $full).Path
}

function New-TextCorpusFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][long]$TargetBytes
    )

    $line = "SakaNote benchmark corpus line 0123456789 abcdefghijklmnopqrstuvwxyz`r`n"
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    $writer = [System.IO.StreamWriter]::new($stream, $utf8, 4096)
    try {
        while ($stream.Length -lt $TargetBytes) {
            $writer.Write($line)
        }
        $writer.Flush()
    }
    finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

function Get-ProcessIdsByName {
    param([Parameter(Mandatory = $true)][string]$Name)
    return @(
        Get-Process -Name $Name -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty Id
    )
}

function Wait-ForTargetProcess {
    param(
        [Parameter(Mandatory = $true)][string]$ProcessName,
        [AllowNull()][int[]]$BaselineIds = @(),
        [System.Diagnostics.Process]$LauncherProcess,
        [int]$TimeoutMs = 8000
    )

    if ($null -eq $BaselineIds) {
        $BaselineIds = @()
    }

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        if ($LauncherProcess -and -not $LauncherProcess.HasExited) {
            return $LauncherProcess
        }

        $candidates = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue |
            Where-Object { $_.Id -notin $BaselineIds }
        if ($candidates) {
            return ($candidates | Sort-Object StartTime -Descending | Select-Object -First 1)
        }

        Start-Sleep -Milliseconds 100
    }

    if ($LauncherProcess -and -not $LauncherProcess.HasExited) {
        return $LauncherProcess
    }

    return $null
}

function Wait-ForMainWindow {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [int]$TimeoutMs = 10000
    )

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        if ($Process.HasExited) {
            return -1.0
        }

        $Process.Refresh()
        if ($Process.MainWindowHandle -ne 0) {
            return [double]$sw.ElapsedMilliseconds
        }
        Start-Sleep -Milliseconds 50
    }

    return -1.0
}

function Stop-StartedProcess {
    param([System.Diagnostics.Process]$Process)

    if (-not $Process) { return }
    if ($Process.HasExited) { return }

    try {
        $null = $Process.CloseMainWindow()
        if (-not $Process.WaitForExit(2000)) {
            $Process.Kill()
            $Process.WaitForExit(2000) | Out-Null
        }
    }
    catch {
        try {
            $Process.Kill()
            $Process.WaitForExit(2000) | Out-Null
        }
        catch {
        }
    }
}

function Close-ProcessesByName {
    param([Parameter(Mandatory = $true)][string]$Name)

    $running = @(Get-Process -Name $Name -ErrorAction SilentlyContinue)
    foreach ($proc in $running) {
        Stop-StartedProcess -Process $proc
    }
}

function Collect-ProcessSamples {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)][int]$DurationSeconds,
        [Parameter(Mandatory = $true)][int]$IntervalMs
    )

    $samples = New-Object System.Collections.Generic.List[object]
    $logicalCpu = [System.Environment]::ProcessorCount
    $endAt = [DateTime]::UtcNow.AddSeconds($DurationSeconds)
    $prevCpu = $Process.TotalProcessorTime
    $prevStamp = [DateTime]::UtcNow

    while ([DateTime]::UtcNow -lt $endAt) {
        Start-Sleep -Milliseconds $IntervalMs

        if ($Process.HasExited) {
            break
        }

        $Process.Refresh()
        $nowStamp = [DateTime]::UtcNow
        $nowCpu = $Process.TotalProcessorTime

        $elapsedMs = ($nowStamp - $prevStamp).TotalMilliseconds
        $cpuDeltaMs = ($nowCpu - $prevCpu).TotalMilliseconds
        $cpuPercent = 0.0
        if ($elapsedMs -gt 0.0) {
            $cpuPercent = ($cpuDeltaMs / ($elapsedMs * $logicalCpu)) * 100.0
            if ($cpuPercent -lt 0.0) { $cpuPercent = 0.0 }
        }

        $sample = [pscustomobject]@{
            TimestampUtc   = [DateTime]::UtcNow.ToString("o")
            CpuPercent     = [Math]::Round($cpuPercent, 2)
            WorkingSetMB   = [Math]::Round($Process.WorkingSet64 / 1MB, 2)
            PrivateMB      = [Math]::Round($Process.PrivateMemorySize64 / 1MB, 2)
            PagedMB        = [Math]::Round($Process.PagedMemorySize64 / 1MB, 2)
            Handles        = $Process.HandleCount
            Threads        = $Process.Threads.Count
        }
        $samples.Add($sample) | Out-Null

        $prevCpu = $nowCpu
        $prevStamp = $nowStamp
    }

    return $samples
}

function Measure-Mean {
    param([AllowNull()][double[]]$Values = @())
    if ($null -eq $Values -or $Values.Count -eq 0) { return [double]::NaN }
    return [double](($Values | Measure-Object -Average).Average)
}

function Measure-Max {
    param([AllowNull()][double[]]$Values = @())
    if ($null -eq $Values -or $Values.Count -eq 0) { return [double]::NaN }
    return [double](($Values | Measure-Object -Maximum).Maximum)
}

function Summarize-Samples {
    param([AllowNull()][object[]]$Samples)

    if ($null -eq $Samples) {
        $Samples = @()
    }

    $metricSamples = @(
        $Samples | Where-Object {
            $_ -and
            ($_.PSObject.Properties.Match("CpuPercent").Count -gt 0) -and
            ($_.PSObject.Properties.Match("WorkingSetMB").Count -gt 0) -and
            ($_.PSObject.Properties.Match("PrivateMB").Count -gt 0) -and
            ($_.PSObject.Properties.Match("PagedMB").Count -gt 0) -and
            ($_.PSObject.Properties.Match("Handles").Count -gt 0) -and
            ($_.PSObject.Properties.Match("Threads").Count -gt 0)
        }
    )

    $cpu = @($metricSamples | ForEach-Object { [double]$_.CpuPercent })
    $workingSet = @($metricSamples | ForEach-Object { [double]$_.WorkingSetMB })
    $private = @($metricSamples | ForEach-Object { [double]$_.PrivateMB })
    $paged = @($metricSamples | ForEach-Object { [double]$_.PagedMB })
    $handles = @($metricSamples | ForEach-Object { [double]$_.Handles })
    $threads = @($metricSamples | ForEach-Object { [double]$_.Threads })

    return [pscustomobject]@{
        SampleCount       = $metricSamples.Count
        AvgCpuPercent     = [Math]::Round((Measure-Mean $cpu), 2)
        PeakCpuPercent    = [Math]::Round((Measure-Max $cpu), 2)
        AvgWorkingSetMB   = [Math]::Round((Measure-Mean $workingSet), 2)
        PeakWorkingSetMB  = [Math]::Round((Measure-Max $workingSet), 2)
        AvgPrivateMB      = [Math]::Round((Measure-Mean $private), 2)
        PeakPrivateMB     = [Math]::Round((Measure-Max $private), 2)
        AvgPagedMB        = [Math]::Round((Measure-Mean $paged), 2)
        PeakPagedMB       = [Math]::Round((Measure-Max $paged), 2)
        AvgHandles        = [Math]::Round((Measure-Mean $handles), 2)
        PeakHandles       = [Math]::Round((Measure-Max $handles), 2)
        AvgThreads        = [Math]::Round((Measure-Mean $threads), 2)
        PeakThreads       = [Math]::Round((Measure-Max $threads), 2)
    }
}

function Format-Num {
    param([double]$Value)
    if ([double]::IsNaN($Value)) { return "N/A" }
    return ("{0:N2}" -f $Value)
}

function Get-ExeProcessName {
    param([Parameter(Mandatory = $true)][string]$ExePath)
    return [System.IO.Path]::GetFileNameWithoutExtension($ExePath)
}

function Get-WindowTitleProcessIds {
    param([Parameter(Mandatory = $true)][string]$TitleHint)

    $ids = New-Object System.Collections.Generic.List[int]
    $all = @(Get-Process -ErrorAction SilentlyContinue)
    foreach ($proc in $all) {
        try {
            if ($proc.MainWindowHandle -ne 0 -and $proc.MainWindowTitle -like "*$TitleHint*") {
                $ids.Add([int]$proc.Id) | Out-Null
            }
        }
        catch {
        }
    }
    return @($ids)
}

function Wait-ForWindowBackedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$TitleHint,
        [AllowNull()][int[]]$ExcludeIds = @(),
        [DateTime]$StartedAfterUtc = [DateTime]::UtcNow.AddMinutes(-2),
        [int]$TimeoutMs = 8000
    )

    if ($null -eq $ExcludeIds) {
        $ExcludeIds = @()
    }

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $candidates = New-Object System.Collections.Generic.List[object]
        $all = @(Get-Process -ErrorAction SilentlyContinue)
        foreach ($proc in $all) {
            try {
                if ($proc.Id -in $ExcludeIds) { continue }
                if ($proc.MainWindowHandle -eq 0) { continue }
                if ($proc.MainWindowTitle -notlike "*$TitleHint*") { continue }
                if ($proc.StartTime.ToUniversalTime() -lt $StartedAfterUtc.AddSeconds(-2)) { continue }
                $candidates.Add($proc) | Out-Null
            }
            catch {
            }
        }

        if ($candidates.Count -gt 0) {
            return ($candidates | Sort-Object StartTime -Descending | Select-Object -First 1)
        }

        Start-Sleep -Milliseconds 100
    }

    return $null
}

$legacyExeFull = Resolve-ExistingPath $LegacyExe
$msNotepadFull = Resolve-ExistingPath $MsNotepadExe
$outputRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDir)
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$runId = Get-Date -Format "yyyyMMdd-HHmmss"
$runDir = Join-Path $outputRoot "run-$runId"
New-Item -ItemType Directory -Force -Path $runDir | Out-Null

$corpusRoot = $CorpusDir
if ([string]::IsNullOrWhiteSpace($corpusRoot)) {
    $corpusRoot = Join-Path $outputRoot "corpus-shared"
}
$corpusDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($corpusRoot)
New-Item -ItemType Directory -Force -Path $corpusDir | Out-Null

$scenarios = @(
    [pscustomobject]@{ Name = "Empty"; FilePath = $null; SizeBytes = 0L },
    [pscustomobject]@{ Name = "Open 1MB"; FilePath = (Join-Path $corpusDir "open-1mb.txt"); SizeBytes = 1MB },
    [pscustomobject]@{ Name = "Open 5MB"; FilePath = (Join-Path $corpusDir "open-5mb.txt"); SizeBytes = 5MB },
    [pscustomobject]@{ Name = "Open 20MB"; FilePath = (Join-Path $corpusDir "open-20mb.txt"); SizeBytes = 20MB }
)

foreach ($scenario in $scenarios) {
    if ($scenario.SizeBytes -le 0) { continue }
    if ($NoFileRegeneration -and (Test-Path -LiteralPath $scenario.FilePath)) { continue }
    New-TextCorpusFile -Path $scenario.FilePath -TargetBytes $scenario.SizeBytes
}

$apps = @(
    [pscustomobject]@{
        Name = "Saka Note"
        Path = $legacyExeFull
        ProcessName = Get-ExeProcessName $legacyExeFull
        WindowTitleHint = "Notepad"
    },
    [pscustomobject]@{
        Name = "Microsoft Notepad"
        Path = $msNotepadFull
        ProcessName = Get-ExeProcessName $msNotepadFull
        WindowTitleHint = "Notepad"
    }
)

if (-not $ForceCloseBeforeEachRun) {
    foreach ($app in $apps) {
        $alreadyRunning = @(Get-Process -Name $app.ProcessName -ErrorAction SilentlyContinue)
        if ($alreadyRunning.Count -gt 0) {
            throw ("Process '{0}' is already running. Close it first, or run with -ForceCloseBeforeEachRun for a controlled benchmark environment." -f $app.ProcessName)
        }
    }
}

$legacyGateExitCode = $null
$legacyGateReportCopied = $false
if (-not $SkipLegacyInternalGate) {
    Write-Host "[1/3] Running Saka Note internal benchmark gate..."
    $gateProc = Start-Process -FilePath $legacyExeFull -ArgumentList "--benchmark-ci" -PassThru -Wait
    $legacyGateExitCode = $gateProc.ExitCode

    $benchDir = Join-Path $env:LOCALAPPDATA "SakaNote\benchmarks"
    if (Test-Path -LiteralPath $benchDir) {
        $latestReport = Get-ChildItem -LiteralPath $benchDir -Filter "benchmark-*.txt" |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
        if ($latestReport) {
            Copy-Item -LiteralPath $latestReport.FullName -Destination (Join-Path $runDir "saka-note-internal-benchmark.txt") -Force
            $legacyGateReportCopied = $true
        }
    }
}
else {
    Write-Host "[1/3] Skipping Saka Note internal benchmark gate (--SkipLegacyInternalGate)."
}

$summaryRows = New-Object System.Collections.Generic.List[object]
$sampleRows = New-Object System.Collections.Generic.List[object]

Write-Host "[2/3] Running A/B process benchmark..."
for ($iteration = 1; $iteration -le $Iterations; $iteration++) {
    foreach ($scenario in $scenarios) {
        foreach ($app in $apps) {
            Write-Host ("  - Iteration {0}/{1} | {2} | {3}" -f $iteration, $Iterations, $app.Name, $scenario.Name)

            $arguments = @()
            if ($scenario.FilePath) {
                $arguments += $scenario.FilePath
            }

            if ($ForceCloseBeforeEachRun) {
                Close-ProcessesByName -Name $app.ProcessName
            }

            $process = $null
            $launcherProcess = $null
            $fallbackProcess = $null
            $startupMs = -1.0
            $startupMetric = [double]::NaN
            $samples = @()

            try {
                $baselineIds = @(Get-ProcessIdsByName -Name $app.ProcessName)
                $baselineWindowIds = @(Get-WindowTitleProcessIds -TitleHint $app.WindowTitleHint)
                $launchStartedUtc = [DateTime]::UtcNow
                if ($arguments.Count -gt 0) {
                    $launcherProcess = Start-Process -FilePath $app.Path -ArgumentList $arguments -PassThru
                }
                else {
                    $launcherProcess = Start-Process -FilePath $app.Path -PassThru
                }

                $process = Wait-ForTargetProcess -ProcessName $app.ProcessName -BaselineIds $baselineIds -LauncherProcess $launcherProcess -TimeoutMs 8000
                if (-not $process) {
                    throw ("Unable to find target process for {0} ({1})." -f $app.Name, $app.ProcessName)
                }

                $startupMs = Wait-ForMainWindow -Process $process -TimeoutMs 3000
                if ($startupMs -lt 0.0) {
                    $windowProcess = Wait-ForWindowBackedProcess -TitleHint $app.WindowTitleHint -ExcludeIds $baselineWindowIds -StartedAfterUtc $launchStartedUtc -TimeoutMs 8000
                    if ($windowProcess) {
                        $startupMs = ([DateTime]::UtcNow - $launchStartedUtc).TotalMilliseconds
                        if ($windowProcess.Id -ne $process.Id) {
                            $fallbackProcess = $process
                            $process = $windowProcess
                        }
                    }
                }
                if ($startupMs -ge 0.0) {
                    $startupMetric = [Math]::Round($startupMs, 2)
                }
                if ($WarmupMs -gt 0) {
                    Start-Sleep -Milliseconds $WarmupMs
                }

                # Some app models (notably modern packaged Notepad) may transfer the visible
                # window to a different process after launch. Re-acquire a live window process
                # before sampling if the original launcher exited.
                if ($process.HasExited) {
                    $replacement = Wait-ForWindowBackedProcess -TitleHint $app.WindowTitleHint -ExcludeIds $baselineWindowIds -StartedAfterUtc $launchStartedUtc -TimeoutMs 4000
                    if ($replacement) {
                        $fallbackProcess = $process
                        $process = $replacement
                    }
                }

                $samples = @(Collect-ProcessSamples -Process $process -DurationSeconds $SampleSeconds -IntervalMs $SampleIntervalMs)
                if ($samples.Count -eq 0) {
                    throw ("No runtime samples were captured for {0} in scenario '{1}'. Benchmark result is invalid." -f $app.Name, $scenario.Name)
                }
            }
            finally {
                if ($process) {
                    Stop-StartedProcess -Process $process
                }
                if ($launcherProcess -and (($process -eq $null) -or ($launcherProcess.Id -ne $process.Id))) {
                    Stop-StartedProcess -Process $launcherProcess
                }
                if ($fallbackProcess -and (($process -eq $null) -or ($fallbackProcess.Id -ne $process.Id))) {
                    Stop-StartedProcess -Process $fallbackProcess
                }
            }

            $stats = Summarize-Samples -Samples $samples
            $summaryRows.Add([pscustomobject]@{
                    RunId             = $runId
                    Iteration         = $iteration
                    App               = $app.Name
                    Scenario          = $scenario.Name
                    StartupMs         = $startupMetric
                    SampleCount       = $stats.SampleCount
                    AvgCpuPercent     = $stats.AvgCpuPercent
                    PeakCpuPercent    = $stats.PeakCpuPercent
                    AvgWorkingSetMB   = $stats.AvgWorkingSetMB
                    PeakWorkingSetMB  = $stats.PeakWorkingSetMB
                    AvgPrivateMB      = $stats.AvgPrivateMB
                    PeakPrivateMB     = $stats.PeakPrivateMB
                    AvgPagedMB        = $stats.AvgPagedMB
                    PeakPagedMB       = $stats.PeakPagedMB
                    AvgHandles        = $stats.AvgHandles
                    PeakHandles       = $stats.PeakHandles
                    AvgThreads        = $stats.AvgThreads
                    PeakThreads       = $stats.PeakThreads
                }) | Out-Null

            foreach ($sample in $samples) {
                $sampleRows.Add([pscustomobject]@{
                        RunId         = $runId
                        Iteration     = $iteration
                        App           = $app.Name
                        Scenario      = $scenario.Name
                        TimestampUtc  = $sample.TimestampUtc
                        CpuPercent    = $sample.CpuPercent
                        WorkingSetMB  = $sample.WorkingSetMB
                        PrivateMB     = $sample.PrivateMB
                        PagedMB       = $sample.PagedMB
                        Handles       = $sample.Handles
                        Threads       = $sample.Threads
                    }) | Out-Null
            }
        }
    }
}

$summaryCsv = Join-Path $runDir "summary.csv"
$samplesCsv = Join-Path $runDir "samples.csv"
$metaJson = Join-Path $runDir "run-metadata.json"
$comparisonMd = Join-Path $runDir "comparison.md"

$summaryRows | Sort-Object App, Scenario, Iteration | Export-Csv -LiteralPath $summaryCsv -NoTypeInformation -Encoding UTF8
$sampleRows | Sort-Object App, Scenario, Iteration, TimestampUtc | Export-Csv -LiteralPath $samplesCsv -NoTypeInformation -Encoding UTF8

$grouped = $summaryRows | Group-Object App, Scenario
$aggregateRows = New-Object System.Collections.Generic.List[object]
foreach ($group in $grouped) {
    $first = $group.Group[0]
    $startupValues = @(
        $group.Group |
            ForEach-Object { [double]$_.StartupMs } |
            Where-Object { -not [double]::IsNaN($_) -and $_ -ge 0.0 }
    )
    $aggregateRows.Add([pscustomobject]@{
            App               = $first.App
            Scenario          = $first.Scenario
            StartupMsAvg      = [Math]::Round((Measure-Mean $startupValues), 2)
            AvgCpuPercentAvg  = [Math]::Round((Measure-Mean @($group.Group | ForEach-Object { [double]$_.AvgCpuPercent })), 2)
            PeakCpuPercentMax = [Math]::Round((Measure-Max @($group.Group | ForEach-Object { [double]$_.PeakCpuPercent })), 2)
            PeakWorkingSetMB  = [Math]::Round((Measure-Max @($group.Group | ForEach-Object { [double]$_.PeakWorkingSetMB })), 2)
            PeakPrivateMB     = [Math]::Round((Measure-Max @($group.Group | ForEach-Object { [double]$_.PeakPrivateMB })), 2)
            PeakHandles       = [Math]::Round((Measure-Max @($group.Group | ForEach-Object { [double]$_.PeakHandles })), 2)
            PeakThreads       = [Math]::Round((Measure-Max @($group.Group | ForEach-Object { [double]$_.PeakThreads })), 2)
        }) | Out-Null
}

$aggregateMap = @{}
foreach ($row in $aggregateRows) {
    $aggregateMap["$($row.App)|$($row.Scenario)"] = $row
}

$metrics = @(
    [pscustomobject]@{ Label = "Startup (ms, lower is better)"; Field = "StartupMsAvg" },
    [pscustomobject]@{ Label = "Avg CPU (%, lower is better)"; Field = "AvgCpuPercentAvg" },
    [pscustomobject]@{ Label = "Peak Working Set (MB, lower is better)"; Field = "PeakWorkingSetMB" },
    [pscustomobject]@{ Label = "Peak Private Memory (MB, lower is better)"; Field = "PeakPrivateMB" },
    [pscustomobject]@{ Label = "Peak Handles (lower is better)"; Field = "PeakHandles" },
    [pscustomobject]@{ Label = "Peak Threads (lower is better)"; Field = "PeakThreads" }
)

$mdLines = New-Object System.Collections.Generic.List[string]
$mdLines.Add("# Saka Note vs Microsoft Notepad Benchmark Report") | Out-Null
$mdLines.Add("") | Out-Null
$mdLines.Add("- Run ID: $runId") | Out-Null
$mdLines.Add("- Iterations: $Iterations") | Out-Null
$mdLines.Add("- Sample window: ${SampleSeconds}s") | Out-Null
$mdLines.Add("- Sample interval: ${SampleIntervalMs}ms") | Out-Null
$mdLines.Add("- Warmup before sampling: ${WarmupMs}ms") | Out-Null
$mdLines.Add("- Force-close before each run: $([bool]$ForceCloseBeforeEachRun)") | Out-Null
$mdLines.Add("- Shared corpus directory: '$corpusDir'") | Out-Null
$mdLines.Add("- Saka Note executable: '$legacyExeFull'") | Out-Null
$mdLines.Add("- Microsoft Notepad executable: '$msNotepadFull'") | Out-Null
if ($legacyGateExitCode -ne $null) {
    $gateStatus = if ($legacyGateExitCode -eq 0) { "PASS" } else { "FAIL (exit code $legacyGateExitCode)" }
    $mdLines.Add("- Saka Note internal benchmark gate: $gateStatus") | Out-Null
    if ($legacyGateReportCopied) {
    $mdLines.Add("- Saka Note benchmark report copied to: 'saka-note-internal-benchmark.txt'") | Out-Null
    }
}
$mdLines.Add("") | Out-Null

foreach ($scenario in $scenarios) {
    $mdLines.Add("## $($scenario.Name)") | Out-Null
    $mdLines.Add("") | Out-Null
    $mdLines.Add("| Metric | Saka Note | Microsoft Notepad | Winner |") | Out-Null
    $mdLines.Add("| --- | ---: | ---: | --- |") | Out-Null

    $legacyKey = "Saka Note|$($scenario.Name)"
    $msKey = "Microsoft Notepad|$($scenario.Name)"
    $legacyRow = $aggregateMap[$legacyKey]
    $msRow = $aggregateMap[$msKey]

    foreach ($metric in $metrics) {
        $winner = "N/A"
        $legacyValue = [double]::NaN
        $msValue = [double]::NaN
        if ($legacyRow) { $legacyValue = [double]$legacyRow.($metric.Field) }
        if ($msRow) { $msValue = [double]$msRow.($metric.Field) }

        if (-not [double]::IsNaN($legacyValue) -and -not [double]::IsNaN($msValue)) {
            if ([Math]::Abs($legacyValue - $msValue) -lt 0.01) {
                $winner = "Tie"
            }
            elseif ($legacyValue -lt $msValue) {
                $winner = "Saka Note"
            }
            else {
                $winner = "Microsoft Notepad"
            }
        }

        $mdLines.Add("| $($metric.Label) | $(Format-Num $legacyValue) | $(Format-Num $msValue) | $winner |") | Out-Null
    }

    $mdLines.Add("") | Out-Null
}

Set-Content -LiteralPath $comparisonMd -Value $mdLines -Encoding UTF8

$metadata = [pscustomobject]@{
    RunId                     = $runId
    TimestampUtc              = [DateTime]::UtcNow.ToString("o")
    Iterations                = $Iterations
    SampleSeconds             = $SampleSeconds
    SampleIntervalMs          = $SampleIntervalMs
    WarmupMs                  = $WarmupMs
    LegacyExe                 = $legacyExeFull
    MsNotepadExe              = $msNotepadFull
    CorpusDir                 = $corpusDir
    ForceCloseBeforeEachRun   = [bool]$ForceCloseBeforeEachRun
    LegacyInternalGateSkipped = [bool]$SkipLegacyInternalGate
    LegacyInternalGateExitCode = $legacyGateExitCode
    OutputFiles               = [pscustomobject]@{
        SummaryCsv = $summaryCsv
        SamplesCsv = $samplesCsv
        ComparisonMd = $comparisonMd
    }
}
$metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $metaJson -Encoding UTF8

Write-Host "[3/3] Finished."
Write-Host "Run directory: $runDir"
Write-Host "Summary CSV : $summaryCsv"
Write-Host "Samples CSV : $samplesCsv"
Write-Host "Report MD   : $comparisonMd"
if ($legacyGateExitCode -ne $null) {
    if ($legacyGateExitCode -eq 0) {
        $legacyGateLabel = "PASS"
    }
    else {
        $legacyGateLabel = "FAIL ($legacyGateExitCode)"
    }
    Write-Host ("Saka Note gate : {0}" -f $legacyGateLabel)
}
