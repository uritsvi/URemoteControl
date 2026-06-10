param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$IncludeDotNet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$solutionPath = Join-Path $repoRoot "URemoteControl.sln"

if (-not (Test-Path $solutionPath)) {
    throw "Solution file not found at '$solutionPath'."
}

function Get-MsBuildPath {
    if (Get-Command msbuild -ErrorAction SilentlyContinue) {
        return "msbuild"
    }

    $vswhere = Join-Path "${env:ProgramFiles(x86)}" "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "Could not locate msbuild. Install Visual Studio Build Tools and ensure msbuild is available."
    }

    $installationPath = & $vswhere -latest -requires Microsoft.Component.MSBuild -property installationPath
    if ([string]::IsNullOrWhiteSpace($installationPath)) {
        throw "Could not locate a Visual Studio installation with MSBuild."
    }

    $msbuildPath = Join-Path $installationPath "MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuildPath)) {
        throw "MSBuild.exe was not found under '$installationPath'."
    }

    return $msbuildPath
}

$msbuild = Get-MsBuildPath

Write-Host "Building C/C++ apps using '$msbuild' ..."

$targets = @(
    "Platform",
    "Helpers",
    "ScreenDelta",
    "ControlledApp",
    "ControllerApp",
    "ConsoleAppLuncher",
    "TestScreenDelta"
)

$buildArgs = @(
    $solutionPath,
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform"
)

if (-not $IncludeDotNet) {
    $buildArgs += "/t:$($targets -join ';')"
}

& $msbuild @buildArgs

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

if ($IncludeDotNet) {
    Write-Host "Build succeeded for full solution (native + .NET) with Configuration=$Configuration Platform=$Platform."
}
else {
    Write-Host "Build succeeded for native C/C++ targets with Configuration=$Configuration Platform=$Platform."
}
