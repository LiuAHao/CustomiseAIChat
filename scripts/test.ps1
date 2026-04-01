$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $ProjectRoot 'scripts\common.ps1')

Initialize-ConsoleEncoding

powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'scripts\test-all.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'Automated test suite failed'
}

$stage0Checklist = (Get-ChildItem (Join-Path $ProjectRoot 'docs') -File -Filter '*.md' | Select-Object -First 1 -ExpandProperty FullName)
if (-not $stage0Checklist) {
    $stage0Checklist = Join-Path $ProjectRoot 'docs'
}

Write-Host ""
Write-Host "Testing guide: TESTING.md"
Write-Host "Stage 0 checklist: $stage0Checklist"
