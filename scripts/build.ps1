$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

New-Item -ItemType Directory -Force -Path (Join-Path $ProjectRoot 'bin') | Out-Null

Write-Host "Building api binary..."
Invoke-Go -Args @('build', '-o', 'bin/api.exe', './cmd/api')

Write-Host "Building worker binary..."
Invoke-Go -Args @('build', '-o', 'bin/worker.exe', './cmd/worker')

Write-Host "Artifacts written to bin/"
