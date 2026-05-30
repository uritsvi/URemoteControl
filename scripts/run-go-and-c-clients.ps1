param(
    [string]$ServerAddress = "127.0.0.1",
    [string]$ServerPort = "80",
    [switch]$BuildFirst,
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [int]$MaxAttempts = 20
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$goScript  = Join-Path $PSScriptRoot "run-go-server.ps1"
$binDir    = Join-Path $repoRoot "bin"

if (-not (Test-Path $goScript)) { throw "Missing script: $goScript" }
if (-not (Test-Path $binDir))   { throw "Missing bin directory at '$binDir'." }

if ($BuildFirst) {
    $buildScript = Join-Path $PSScriptRoot "build-c-apps.ps1"
    & $buildScript -Configuration $Configuration
}

$controllerExe = Join-Path $binDir "ControllerApp.exe"
$controlledExe = Join-Path $binDir "ControlledApp.exe"

if (-not (Test-Path $controllerExe)) {
    throw "Executable not found at '$controllerExe'. Build first or verify your output path."
}
if (-not (Test-Path $controlledExe)) {
    throw "Executable not found at '$controlledExe'. Build first or verify your output path."
}

# ── helpers ──────────────────────────────────────────────────────────────────

function Test-TcpPortOpen {
    param([string]$HostName, [int]$Port, [int]$TimeoutMs = 500)
    try {
        $client  = New-Object System.Net.Sockets.TcpClient
        $connect = $client.BeginConnect($HostName, $Port, $null, $null)
        if (-not $connect.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
            $client.Close(); return $false
        }
        $client.EndConnect($connect)
        $client.Close()
        return $true
    }
    catch { return $false }
}

function Stop-ProcessTree {
    param([System.Diagnostics.Process]$proc)
    if ($null -eq $proc) { return }
    try {
        if (-not (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue)) { return }
        Write-Host "  Killing PID $($proc.Id) ..."
        & taskkill /PID $proc.Id /T /F 2>&1 | Out-Null
    }
    catch { Write-Warning "  Could not kill PID $($proc.Id): $($_.Exception.Message)" }
}

function Kill-StaleClientInstances {
    foreach ($name in @("ControllerApp", "ControlledApp")) {
        $procs = Get-Process -Name $name -ErrorAction SilentlyContinue
        foreach ($p in $procs) {
            Write-Host "  Killing stale $name (PID $($p.Id)) ..."
            & taskkill /PID $p.Id /T /F 2>&1 | Out-Null
        }
    }
}

function Is-ProcessAlive {
    param([System.Diagnostics.Process]$proc)
    if ($null -eq $proc) { return $false }
    return $null -ne (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue)
}

# ── main retry loop ───────────────────────────────────────────────────────────

$goShell        = $null
$controlledProc = $null
$controllerProc = $null

function Stop-All {
    Stop-ProcessTree $controlledProc
    Stop-ProcessTree $controllerProc
    Stop-ProcessTree $goShell
}

$attempt = 0

try {
    while ($attempt -lt $MaxAttempts) {
        $attempt++
        Write-Host ""
        Write-Host "=== Attempt $attempt / $MaxAttempts ==="

        # Kill any leftover instances from the previous attempt.
        Kill-StaleClientInstances
        Stop-All

        $goShell        = $null
        $controlledProc = $null
        $controllerProc = $null

        # ── Start Go server ───────────────────────────────────────────────────
        Write-Host "Starting Go server on port $ServerPort ..."
        $goShell = Start-Process -FilePath "powershell.exe" -ArgumentList @(
            "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", $goScript, $ServerPort
        ) -PassThru

        # Wait for the TCP listener to be open (up to 60 s to allow compilation).
        Write-Host "Waiting for Go server to open port $ServerPort ..."
        $deadline  = (Get-Date).AddSeconds(60)
        $listening = $false
        while ((Get-Date) -lt $deadline) {
            if (-not (Is-ProcessAlive $goShell)) {
                Write-Host "  Go server process exited during startup – retrying."
                break
            }
            if (Test-TcpPortOpen -HostName $ServerAddress -Port ([int]$ServerPort)) {
                $listening = $true
                break
            }
            Start-Sleep -Milliseconds 500
        }

        if (-not $listening) {
            Write-Host "  Go server did not open port – retrying."
            Stop-All
            Start-Sleep -Seconds 2
            continue
        }

        Write-Host "  Go server is listening."

        # ── Start Controlled app ──────────────────────────────────────────────
        Write-Host "Starting ControlledApp ($ServerAddress $ServerPort) ..."
        $controlledProc = Start-Process -FilePath $controlledExe `
            -WorkingDirectory $binDir `
            -ArgumentList @($ServerAddress, $ServerPort) `
            -PassThru

        # ── Wait, then start Controller app ───────────────────────────────────
        Write-Host "Waiting 5 s before starting ControllerApp ..."
        for ($i = 0; $i -lt 5; $i++) {
            Start-Sleep -Seconds 1
            if (-not (Is-ProcessAlive $goShell)) {
                Write-Host "  Go server exited while waiting – retrying."
                break
            }
            if (-not (Is-ProcessAlive $controlledProc)) {
                Write-Host "  ControlledApp exited while waiting – retrying."
                break
            }
        }

        if (-not (Is-ProcessAlive $goShell) -or -not (Is-ProcessAlive $controlledProc)) {
            Stop-All
            Start-Sleep -Seconds 2
            continue
        }

        Write-Host "Starting ControllerApp ($ServerAddress $ServerPort) ..."
        $controllerProc = Start-Process -FilePath $controllerExe `
            -WorkingDirectory $binDir `
            -ArgumentList @($ServerAddress, $ServerPort) `
            -PassThru

        # ── Verify all three are alive after 8 s (enough for handshake) ───────
        Write-Host "Waiting 8 s to confirm all processes are connected ..."
        $stable = $true
        for ($i = 0; $i -lt 8; $i++) {
            Start-Sleep -Seconds 1
            if (-not (Is-ProcessAlive $goShell)) {
                Write-Host "  Go server exited – retrying."
                $stable = $false; break
            }
            if (-not (Is-ProcessAlive $controlledProc)) {
                Write-Host "  ControlledApp exited – retrying."
                $stable = $false; break
            }
            if (-not (Is-ProcessAlive $controllerProc)) {
                Write-Host "  ControllerApp exited – retrying."
                $stable = $false; break
            }
        }

        if (-not $stable) {
            Stop-All
            Start-Sleep -Seconds 2
            continue
        }

        # ── All three are running – stay alive until Stop is pressed ──────────
        Write-Host ""
        Write-Host "All processes are running and connected."
        Write-Host "Press Ctrl+C or Stop in the IDE to shut down everything."

        while ($true) {
            Start-Sleep -Seconds 2

            $goAlive         = Is-ProcessAlive $goShell
            $controlledAlive = Is-ProcessAlive $controlledProc
            $controllerAlive = Is-ProcessAlive $controllerProc

            if (-not $goAlive)         { Write-Host "Go server exited unexpectedly – retrying."; break }
            if (-not $controlledAlive) { Write-Host "ControlledApp exited unexpectedly – retrying."; break }
            if (-not $controllerAlive) { Write-Host "ControllerApp exited unexpectedly – retrying."; break }
        }

        Stop-All
        Start-Sleep -Seconds 2
    }

    Write-Host "Reached maximum attempts ($MaxAttempts). Giving up."
}
finally {
    Stop-All
}
