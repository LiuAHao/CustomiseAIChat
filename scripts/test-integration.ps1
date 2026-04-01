$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

Write-Host "Running integration tests..."
Invoke-Go -Args @('test', './tests/integration/...')
Write-Host "Tip: run .\\scripts\\test-postgres.ps1 to verify the PostgreSQL path against a real database."
