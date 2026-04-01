$ProjectRoot = Split-Path -Parent $PSScriptRoot

Write-Host "== Unit tests =="
powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'scripts\test-unit.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'Unit tests failed'
}

Write-Host ""
Write-Host "== Integration tests =="
powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'scripts\test-integration.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'Integration tests failed'
}

Write-Host ""
Write-Host "All automated tests passed."
