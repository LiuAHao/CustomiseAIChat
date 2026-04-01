param(
    [switch]$NoWorker
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

$httpAddr = $env:HTTP_ADDR
if ([string]::IsNullOrWhiteSpace($httpAddr)) {
    $httpAddr = ':8081'
}
$httpPort = Get-PortFromAddress -Address $httpAddr
Assert-PortAvailable -Port $httpPort

Write-Host "Using go: $script:GOCmd"
Write-Host "GOCACHE: $env:GOCACHE"
Write-Host "GOMODCACHE: $env:GOMODCACHE"
Write-Host "HTTP port: $httpPort"

if (-not $NoWorker) {
    $workerJob = Start-Job -ScriptBlock {
        param($ProjectRoot, $GoCmd, $GoCache, $GoModCache, $GoFlags)
        $utf8 = [System.Text.UTF8Encoding]::new($false)
        [Console]::InputEncoding = $utf8
        [Console]::OutputEncoding = $utf8
        $OutputEncoding = $utf8
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
