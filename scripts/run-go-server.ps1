param(
    [switch]$InstallOnly,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$GoArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$goServerDir = Join-Path $repoRoot "Server"

if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
    throw "Go was not found. Install Go and ensure it is in PATH."
}

Push-Location $goServerDir
try {
    Write-Host "Downloading Go module dependencies in $goServerDir ..."
    go mod download

    if ($InstallOnly) {
        Write-Host "Go dependencies installed."
        exit 0
    }

    Write-Host "Starting Go server from $goServerDir ..."
    go run . @GoArgs
}
finally {
    Pop-Location
}
