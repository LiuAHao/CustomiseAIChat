param(
    [string]$PostgresDsn = ""
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

if ([string]::IsNullOrWhiteSpace($PostgresDsn)) {
    if (-not [string]::IsNullOrWhiteSpace($env:POSTGRES_DSN)) {
        $PostgresDsn = $env:POSTGRES_DSN
    } else {
        $PostgresDsn = "postgres://agent:agent@127.0.0.1:5432/agent_platform?sslmode=disable"
    }
}

$env:POSTGRES_INTEGRATION = '1'
$env:POSTGRES_TEST_DSN = $PostgresDsn
$env:POSTGRES_DSN = $PostgresDsn

Write-Host "Running PostgreSQL integration tests with DSN: $PostgresDsn"
Invoke-Go -Args @('test', './tests/integration/...', '-run', 'TestPostgres')
