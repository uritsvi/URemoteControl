#requires -Version 5.0
<#
.SYNOPSIS
    Verify that the running URemoteControl stack is healthy: the Go relay server
    and both C clients are alive, none is blocked on an error dialog, and the
    server logs contain no panic / error.

.DESCRIPTION
    The C clients report fatal errors with a blocking MessageBox (debug builds),
    so "process alive" alone is not proof of health. This script also enumerates
    each client's top-level windows and fails if it finds a dialog window
    (class #32770 = a MessageBox / error popup).

.PARAMETER SettleSeconds
    How long to let the stack run before judging it healthy. Default 25 s — long
    enough to stream many full-screen frames (the case that previously crashed on
    incompressible content).
#>
param(
    [int]$SettleSeconds = 25
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot  = Resolve-Path (Join-Path $PSScriptRoot "..")
$logDir    = Join-Path $repoRoot "logs"
$serverOut = Join-Path $logDir "go-server.log"
$serverErr = Join-Path $logDir "go-server.err.log"

Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
public class WinEnum {
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc cb, IntPtr p);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr h, StringBuilder s, int m);
    [DllImport("user32.dll")] static extern int GetWindowText(IntPtr h, StringBuilder s, int m);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    delegate bool EnumWindowsProc(IntPtr h, IntPtr p);
    public static List<string> ForPids(HashSet<uint> pids) {
        var o = new List<string>();
        EnumWindows((h,l)=>{
            uint pid; GetWindowThreadProcessId(h, out pid);
            if (pids.Contains(pid) && IsWindowVisible(h)) {
                var c = new StringBuilder(256); GetClassName(h, c, 256);
                var t = new StringBuilder(512); GetWindowText(h, t, 512);
                o.Add(pid + "|" + c.ToString() + "|" + t.ToString());
            }
            return true;
        }, IntPtr.Zero);
        return o;
    }
}
"@

if ($SettleSeconds -gt 0) {
    Write-Host "Letting the stack run for $SettleSeconds s ..."
    Start-Sleep -Seconds $SettleSeconds
}

$ok = $true

# 1) Process liveness + uptime.
$go         = Get-Process go            -ErrorAction SilentlyContinue | Select-Object -First 1
$controlled = Get-Process ControlledApp -ErrorAction SilentlyContinue | Select-Object -First 1
$controller = Get-Process ControllerApp -ErrorAction SilentlyContinue | Select-Object -First 1

function Report($label, $p) {
    if ($null -eq $p) { Write-Host ("  [FAIL] {0}: not running" -f $label); return $false }
    $up = [int]((Get-Date) - $p.StartTime).TotalSeconds
    Write-Host ("  [ OK ] {0}: PID {1}, up {2}s" -f $label, $p.Id, $up)
    return $true
}

Write-Host "Processes:"
if (-not (Report "Go server   " $go))         { $ok = $false }
if (-not (Report "ControlledApp" $controlled)) { $ok = $false }
if (-not (Report "ControllerApp" $controller)) { $ok = $false }

# 2) No error dialog (MessageBox, class #32770) owned by the clients.
$pids = New-Object 'System.Collections.Generic.HashSet[uint32]'
foreach ($p in @($controlled, $controller)) { if ($p) { [void]$pids.Add([uint32]$p.Id) } }

Write-Host "Client windows:"
$dialogs = @()
if ($pids.Count -gt 0) {
    foreach ($w in [WinEnum]::ForPids($pids)) {
        $parts = $w -split '\|', 3
        Write-Host ("  pid {0}  class '{1}'  title '{2}'" -f $parts[0], $parts[1], $parts[2])
        if ($parts[1] -eq '#32770') { $dialogs += $w }
    }
}
if ($dialogs.Count -gt 0) {
    Write-Host ("  [FAIL] error dialog(s) detected: {0}" -f ($dialogs -join '  ;  '))
    $ok = $false
} else {
    Write-Host "  [ OK ] no error dialogs."
}

# 3) Server logs clean.
Write-Host "Server log scan:"
$serverText = ""
if (Test-Path $serverOut) { $serverText = Get-Content $serverOut -Raw -ErrorAction SilentlyContinue }
if ($null -eq $serverText) { $serverText = "" }
$serverErrText = ""
if (Test-Path $serverErr) { $serverErrText = Get-Content $serverErr -Raw -ErrorAction SilentlyContinue }
if ($null -eq $serverErrText) { $serverErrText = "" }

if ($serverText -match "All clients are connected") {
    Write-Host "  [ OK ] 'All clients are connected' present."
} else {
    Write-Host "  [FAIL] handshake line missing."; $ok = $false
}
if ($serverText -match "panic" -or $serverErrText -match "panic" -or $serverErrText.Trim().Length -gt 0) {
    Write-Host "  [FAIL] server reported a panic / error:"
    if ($serverErrText.Trim().Length -gt 0) { Write-Host $serverErrText }
    $ok = $false
} else {
    Write-Host "  [ OK ] no panic, error log empty."
}

Write-Host ""
if ($ok) {
    Write-Host "VERIFY: HEALTHY - server and both clients running with no errors." -ForegroundColor Green
    exit 0
} else {
    Write-Host "VERIFY: PROBLEM detected (see above)." -ForegroundColor Red
    exit 1
}
