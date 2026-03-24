param(
    [switch]$NoWorker
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

Write-Host "Using go: $script:GOCmd"
Write-Host "GOCACHE: $env:GOCACHE"
Write-Host "GOMODCACHE: $env:GOMODCACHE"

if (-not $NoWorker) {
    $workerJob = Start-Job -ScriptBlock {
        param($ProjectRoot, $GoCmd, $GoCache, $GoModCache, $GoFlags)
        Set-Location $ProjectRoot
        $env:GOCACHE = $GoCache
        $env:GOMODCACHE = $GoModCache
        $env:GOFLAGS = $GoFlags
        & $GoCmd run ./cmd/worker
    } -ArgumentList $ProjectRoot, $script:GOCmd, $env:GOCACHE, $env:GOMODCACHE, $env:GOFLAGS

    Write-Host "Worker job started: $($workerJob.Id)"
}

try {
    Invoke-Go -Args @('run', './cmd/api')
} finally {
    if ($workerJob) {
        Stop-Job $workerJob -ErrorAction SilentlyContinue | Out-Null
        Remove-Job $workerJob -Force -ErrorAction SilentlyContinue | Out-Null
    }
}
