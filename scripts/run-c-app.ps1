param(
    [ValidateSet("controller", "controlled", "launcher")]
    [string]$App = "controller",
    [string]$ServerAddress = "127.0.0.1",
    [string]$ServerPort = "80",
    [ValidateSet("1", "2")]
    [string]$LauncherAppType = "1",
    [switch]$BuildFirst,
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$NewWindow
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$binDir = Join-Path $repoRoot "bin"

if (-not (Test-Path $binDir)) {
    throw "Missing bin directory at '$binDir'."
}

if ($BuildFirst) {
    $buildScript = Join-Path $PSScriptRoot "build-c-apps.ps1"
    & $buildScript -Configuration $Configuration
}

$exeName = switch ($App) {
    "controller" { "ControllerApp.exe" }
    "controlled" { "ControlledApp.exe" }
    "launcher" { "ConsoleAppLauncher.exe" }
}

$exePath = Join-Path $binDir $exeName
if (-not (Test-Path $exePath)) {
    throw "Executable not found at '$exePath'. Build first or verify your output path."
}

$arguments = @($ServerAddress, $ServerPort)
if ($App -eq "launcher") {
    # Launcher accepts app-type first: 1 controller, 2 controlled.
    $arguments = @($LauncherAppType, $ServerAddress, $ServerPort)
}

Write-Host "Starting $exeName from $binDir ..."

if ($NewWindow) {
    Start-Process -FilePath $exePath -WorkingDirectory $binDir -ArgumentList $arguments
}
else {
    Push-Location $binDir
    try {
        & $exePath @arguments
    }
    finally {
        Pop-Location
    }
}
