$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

Write-Host "Running go test ./..."
Invoke-Go -Args @('test', './...')

Write-Host ""
Write-Host "Testing guide: TESTING.md"
Write-Host "Stage 0 checklist: docs/阶段0测试验收清单.md"
