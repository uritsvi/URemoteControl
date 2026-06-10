#requires -Version 5.0
<#
.SYNOPSIS
    Stop everything started by run-debug.ps1: the Controller / Controlled C
    clients and the Go relay server.
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = "SilentlyContinue"

# Stop the C clients by image name.
foreach ($name in @("ControllerApp", "ControlledApp")) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "Stopping $name (PID $($_.Id)) ..."
        & taskkill /PID $_.Id /T /F 2>&1 | Out-Null
    }
}

# Stop the Go server: the `go run` launcher ("go") and the compiled child it
# spawns. `go run .` builds the module into a temp exe named after the module
# ("Server.exe", run from %LocalAppData%\go-build\...), so kill that too.
foreach ($name in @("go", "Server", "main")) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "Stopping $name (PID $($_.Id)) ..."
        & taskkill /PID $_.Id /T /F 2>&1 | Out-Null
    }
}

# Belt and suspenders: kill any leftover go-build child by image path.
Get-Process -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -like '*go-build*' } |
    ForEach-Object {
        Write-Host "Stopping go-build child (PID $($_.Id)) ..."
        & taskkill /PID $_.Id /T /F 2>&1 | Out-Null
    }

Write-Host "Done."
