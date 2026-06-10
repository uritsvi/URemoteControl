param(
    [switch]$InstallOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$pythonScript = Join-Path $PSScriptRoot "run-python-server.ps1"
$goScript = Join-Path $PSScriptRoot "run-go-server.ps1"
$serverProcesses = @()

function Stop-ServerProcesses {
    foreach ($proc in $serverProcesses) {
        if ($null -eq $proc) {
            continue
        }

        try {
            $runningProc = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
            if ($null -eq $runningProc) {
                continue
            }

            Write-Host "Stopping process tree for PID $($proc.Id) ..."
            & taskkill /PID $proc.Id /T /F | Out-Null
        }
        catch {
            Write-Warning "Failed to stop PID $($proc.Id): $($_.Exception.Message)"
        }
    }
}

if (-not (Test-Path $pythonScript)) {
    throw "Missing script: $pythonScript"
}

if (-not (Test-Path $goScript)) {
    throw "Missing script: $goScript"
}

Write-Host "Installing all server dependencies ..."
& $pythonScript -InstallOnly
& $goScript -InstallOnly

if ($InstallOnly) {
    Write-Host "All dependencies installed."
    exit 0
}

try {
    Write-Host "Launching Python server ..."
    $pythonProcess = Start-Process -FilePath "powershell.exe" -ArgumentList @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "`"$pythonScript`""
    ) -PassThru
    $serverProcesses += $pythonProcess

    Write-Host "Launching Go server ..."
    $goProcess = Start-Process -FilePath "powershell.exe" -ArgumentList @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "`"$goScript`""
    ) -PassThru
    $serverProcesses += $goProcess

    Write-Host "Both servers launched. Press Ctrl+C (Stop) to terminate all server processes."

    while ($true) {
        Start-Sleep -Seconds 1
        $running = 0
        foreach ($proc in $serverProcesses) {
            if (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue) {
                $running++
            }
        }

        if ($running -eq 0) {
            break
        }
    }
}
finally {
    Stop-ServerProcesses
}
