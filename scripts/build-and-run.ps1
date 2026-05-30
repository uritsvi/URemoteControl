#requires -Version 5.0
<#
.SYNOPSIS
    Build AND run the whole URemoteControl stack with one command:
    the Go relay server + the Controlled and Controller C clients.

.DESCRIPTION
    Steps, in order:
      1. (optional) Clean the native build outputs for the chosen configuration.
      2. Stop any stale instances from a previous run (calls stop-debug.ps1).
      3. Build the native C/C++ apps (Platform, Helpers, ScreenDelta,
         ControlledApp, ControllerApp, ConsoleAppLuncher, TestScreenDelta).
      4. Compile-check the Go relay server.
      5. Launch the relay + both clients and wait until they connect
         (delegates to run-debug.ps1, which runs the relay via `go run`).

    Runs in debug mode (config.ini -> debug_mode=1), which is safe on a single
    machine: input is neither captured nor applied and the controller opens in a
    normal window. End-to-end encryption is active whenever config.ini has a
    non-empty e2e_key.

.PARAMETER Configuration
    Debug (default) or Release. The C clients and their static libs are built in
    this configuration. Tip: pass -Clean the first time you switch configuration
    (the static libs are written to a shared lib\ folder, so a stale lib from the
    other configuration must be rebuilt).

.PARAMETER Clean
    Force a full rebuild: delete the chosen configuration's intermediate objects
    and the shared static libs before building.

.PARAMETER IncludeDotNet
    Also build the .NET projects (URemoteControlGUI / service). Not needed to run
    the core stack.

.PARAMETER BuildOnly
    Build (and compile-check the relay) but do not launch anything.

.PARAMETER KeepOpen
    After a successful connection, keep monitoring until a process exits or you
    press Ctrl+C (then run stop-debug.ps1). Without it the script returns once
    connectivity is confirmed, leaving everything running.

.PARAMETER ServerAddress
    Address the clients dial / the relay binds. Default 127.0.0.1 (loopback).

.PARAMETER ServerPort
    TCP port for the relay. Default 47800.

.EXAMPLE
    .\scripts\build-and-run.ps1
    # Debug build, run the stack, return once connected.

.EXAMPLE
    .\scripts\build-and-run.ps1 -Configuration Release -Clean -KeepOpen
    # Clean Release build, run, and keep monitoring until something exits.
#>
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Clean,
    [switch]$IncludeDotNet,
    [switch]$BuildOnly,
    [switch]$KeepOpen,
    [string]$ServerAddress = "127.0.0.1",
    [string]$ServerPort = "47800"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptsDir = $PSScriptRoot
$repoRoot   = Resolve-Path (Join-Path $scriptsDir "..")
$serverDir  = Join-Path $repoRoot "Server"

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }

Write-Host "URemoteControl - build & run ($Configuration)" -ForegroundColor Green

# 1. Clean -----------------------------------------------------------------------
# The static libs (Platform/Helpers/ScreenDelta) are written to a shared lib\
# folder for BOTH Debug and Release, so a lib left over from the other
# configuration would otherwise be linked into the exe (a Debug lib in a Release
# build pulls in msvcrtd and fails to link). Always remove them so each run
# rebuilds the libs for the requested configuration.
Write-Step "Refreshing static libs for a consistent $Configuration build ..."
foreach ($lib in @("Platform\lib\Platform.lib", "Helpers\lib\Helpers.lib", "ScreenDelta\lib\ScreenDelta.lib")) {
    $p = Join-Path $repoRoot $lib
    if (Test-Path $p) { Remove-Item $p -Force -ErrorAction SilentlyContinue }
}

if ($Clean) {
    Write-Step "Clean: deleting intermediate objects for $Configuration ..."
    Get-ChildItem -Path $repoRoot -Directory -Recurse -Filter "x64" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch "vcpkg_installed" } |
        ForEach-Object {
            $cfgDir = Join-Path $_.FullName $Configuration
            if (Test-Path $cfgDir) { Remove-Item $cfgDir -Recurse -Force -ErrorAction SilentlyContinue }
        }
}

# 2. Stop stale instances -------------------------------------------------------
Write-Step "Stopping any stale instances from a previous run ..."
& (Join-Path $scriptsDir "stop-debug.ps1") | Out-Null

# 3. Build the native C/C++ apps ------------------------------------------------
Write-Step "Building C/C++ apps ..."
$buildArgs = @{ Configuration = $Configuration }
if ($IncludeDotNet) { $buildArgs.IncludeDotNet = $true }
& (Join-Path $scriptsDir "build-c-apps.ps1") @buildArgs

# 4. Compile-check the Go relay server -----------------------------------------
Write-Step "Building Go relay server ..."
if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
    throw "Go was not found in PATH. Install Go (the relay server is written in Go)."
}
Push-Location $serverDir
try {
    & go build ./...
    if ($LASTEXITCODE -ne 0) { throw "Go build failed with exit code $LASTEXITCODE." }
}
finally {
    Pop-Location
}
Write-Host "Go relay server compiles cleanly."

if ($BuildOnly) {
    Write-Host "`nBuild succeeded ($Configuration). -BuildOnly set, not launching." -ForegroundColor Green
    return
}

# 5. Launch the whole stack -----------------------------------------------------
Write-Step "Launching the stack (relay + both clients) ..."
$runArgs = @{ ServerAddress = $ServerAddress; ServerPort = $ServerPort }
if ($KeepOpen) { $runArgs.KeepOpen = $true }
& (Join-Path $scriptsDir "run-debug.ps1") @runArgs

Write-Host "`nTip: stop everything with  scripts\stop-debug.ps1" -ForegroundColor DarkGray
