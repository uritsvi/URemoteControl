#requires -Version 5.0
<#
.SYNOPSIS
    Build (optional) and run the full URemoteControl stack locally in DEBUG mode:
    the Go relay server plus the Controlled and Controller C clients, both on this
    machine.

.DESCRIPTION
    Debug mode (config.ini -> debug_mode=1) makes this safe to run on a single
    machine: the controller does not capture or forward your input and does not
    trap the cursor, the controlled side never applies remote input, and the
    controller window opens windowed (non full screen). End-to-end encryption is
    active whenever config.ini has a non-empty e2e_key.

    The script starts the server, waits for it to listen, launches both clients,
    then watches the server log until it prints "All clients are connected".

.PARAMETER ServerAddress
    Address the clients dial. Default 127.0.0.1.

.PARAMETER ServerPort
    TCP port for the relay server. Default 47800 (a high, non privileged port).

.PARAMETER BuildFirst
    Rebuild the C apps before running.

.PARAMETER KeepOpen
    After a successful connection, keep monitoring until a process exits or
    Ctrl+C is pressed. Without it the script returns once connectivity is
    confirmed, leaving the processes running.
#>
param(
    [string]$ServerAddress = "127.0.0.1",
    [string]$ServerPort = "47800",
    [switch]$BuildFirst,
    [switch]$KeepOpen,
    [int]$ConnectTimeoutSec = 40
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot  = Resolve-Path (Join-Path $PSScriptRoot "..")
$binDir    = Join-Path $repoRoot "bin"
$serverDir = Join-Path $repoRoot "Server"
$logDir    = Join-Path $repoRoot "logs"

if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
    throw "Go was not found. Install Go and ensure it is in PATH."
}
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }

$serverOut = Join-Path $logDir "go-server.log"
$serverErr = Join-Path $logDir "go-server.err.log"

if ($BuildFirst) {
    & (Join-Path $PSScriptRoot "build-c-apps.ps1") -Configuration Debug
}

$controllerExe = Join-Path $binDir "ControllerApp.exe"
$controlledExe = Join-Path $binDir "ControlledApp.exe"
if (-not (Test-Path $controllerExe)) { throw "Missing '$controllerExe'. Run with -BuildFirst." }
if (-not (Test-Path $controlledExe)) { throw "Missing '$controlledExe'. Run with -BuildFirst." }

function Test-Alive { param($proc) return ($null -ne (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue)) }

# Kill any stale client instances from a previous run.
foreach ($name in @("ControllerApp", "ControlledApp")) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | ForEach-Object {
        & taskkill /PID $_.Id /T /F 2>&1 | Out-Null
    }
}

Remove-Item $serverOut, $serverErr -ErrorAction SilentlyContinue

# Bind the relay to the same address the clients dial. For local runs this is
# 127.0.0.1 (loopback), which is exempt from the Windows Firewall, so Windows
# does NOT prompt to "allow this app to communicate on networks".
$env:UREMOTE_LISTEN_HOST = $ServerAddress

Write-Host "Starting Go relay server on ${ServerAddress}:$ServerPort (log: $serverOut) ..."
# -WindowStyle Hidden (not -NoNewWindow) so the long-lived server does not
# inherit this script's console handles and keep them open after we return.
$go = Start-Process -FilePath "go" -ArgumentList @("run", ".", $ServerPort) `
    -WorkingDirectory $serverDir -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr

# Wait for the server's own "listening" log line rather than opening a probe
# socket: the relay would otherwise treat the probe as a (malformed) client.
Write-Host "Waiting for the server to listen (allows time to compile) ..."
$deadline = (Get-Date).AddSeconds(60)
$listening = $false
while ((Get-Date) -lt $deadline) {
    if ((Test-Path $serverOut) -and ((Get-Content $serverOut -Raw -ErrorAction SilentlyContinue) -match "Start listening on")) {
        $listening = $true; break
    }
    if (-not (Test-Alive $go)) { break }
    Start-Sleep -Milliseconds 500
}
if (-not $listening) {
    Get-Content $serverOut, $serverErr -ErrorAction SilentlyContinue
    throw "Go server did not start listening on port $ServerPort."
}
# Small grace period so the listen socket is actually accepting.
Start-Sleep -Seconds 1
Write-Host "Server is listening."

Write-Host "Starting ControlledApp ($ServerAddress $ServerPort) ..."
$controlled = Start-Process -FilePath $controlledExe -WorkingDirectory $binDir `
    -ArgumentList @($ServerAddress, $ServerPort) -PassThru

Start-Sleep -Seconds 2

Write-Host "Starting ControllerApp ($ServerAddress $ServerPort) ..."
$controller = Start-Process -FilePath $controllerExe -WorkingDirectory $binDir `
    -ArgumentList @($ServerAddress, $ServerPort) -PassThru

Write-Host "Waiting up to $ConnectTimeoutSec s for the handshake ..."
$connected = $false
$deadline = (Get-Date).AddSeconds($ConnectTimeoutSec)
while ((Get-Date) -lt $deadline) {
    if (Test-Path $serverOut) {
        if ((Get-Content $serverOut -Raw -ErrorAction SilentlyContinue) -match "All clients are connected") {
            $connected = $true; break
        }
    }
    if (-not (Test-Alive $controlled)) { Write-Host "ControlledApp exited early."; break }
    if (-not (Test-Alive $controller)) { Write-Host "ControllerApp exited early."; break }
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "==================== Go server log ===================="
Get-Content $serverOut -ErrorAction SilentlyContinue
Get-Content $serverErr -ErrorAction SilentlyContinue
Write-Host "======================================================="
Write-Host ("Go server    PID {0}  alive={1}" -f $go.Id, (Test-Alive $go))
Write-Host ("ControlledApp PID {0}  alive={1}" -f $controlled.Id, (Test-Alive $controlled))
Write-Host ("ControllerApp PID {0}  alive={1}" -f $controller.Id, (Test-Alive $controller))

if ($connected -and (Test-Alive $controlled) -and (Test-Alive $controller)) {
    Write-Host ""
    Write-Host "SUCCESS: all clients connected. The stack is running." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "FAILURE: the stack did not fully connect." -ForegroundColor Red
    if (-not $KeepOpen) { exit 1 }
}

if ($KeepOpen) {
    Write-Host "Monitoring. Press Ctrl+C to stop (then run scripts\stop-debug.ps1)."
    while ((Test-Alive $go) -and (Test-Alive $controlled) -and (Test-Alive $controller)) {
        Start-Sleep -Seconds 2
    }
    Write-Host "A process exited; stopping the rest."
    & (Join-Path $PSScriptRoot "stop-debug.ps1")
}
