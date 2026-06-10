param(
    [switch]$InstallOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$httpServerDir = Join-Path $repoRoot "HTTPServer"
$requirementsFile = Join-Path $httpServerDir "requirements.txt"

if (-not (Test-Path $requirementsFile)) {
    throw "Missing requirements file at '$requirementsFile'."
}

$pythonExe = $null
$pythonPrefix = @()

if (Get-Command py -ErrorAction SilentlyContinue) {
    $pythonExe = "py"
    $pythonPrefix = @("-3")
}
elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $pythonExe = "python"
}
else {
    throw "Python was not found. Install Python 3.9+ and ensure it is in PATH."
}

Write-Host "Installing Python dependencies from $requirementsFile ..."
& $pythonExe @pythonPrefix -m pip install --upgrade pip
& $pythonExe @pythonPrefix -m pip install -r $requirementsFile

if ($InstallOnly) {
    Write-Host "Python dependencies installed."
    exit 0
}

Push-Location $httpServerDir
try {
    Write-Host "Starting Python HTTP server from $httpServerDir ..."
    & $pythonExe @pythonPrefix "main.py"
}
finally {
    Pop-Location
}
