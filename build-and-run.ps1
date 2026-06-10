# Thin convenience wrapper so you can run  .\build-and-run.ps1  from the repo
# root. Forwards all arguments to scripts\build-and-run.ps1.
#   .\build-and-run.ps1                       # Debug build + run
#   .\build-and-run.ps1 -Configuration Release -Clean -KeepOpen
#   .\build-and-run.ps1 -BuildOnly
& (Join-Path $PSScriptRoot "scripts\build-and-run.ps1") @args
