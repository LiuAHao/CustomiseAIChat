$ProjectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'common.ps1')

Initialize-GoEnvironment -ProjectRoot $ProjectRoot
Set-Location $ProjectRoot

$baseUrl = 'http://127.0.0.1:8081'

Write-Host 'Checking health endpoint...'
$health = Invoke-WebRequest "$baseUrl/healthz" -UseBasicParsing -TimeoutSec 5
if ($health.StatusCode -ne 200) {
    throw "Health check failed with status $($health.StatusCode)"
}
Write-Host "Health OK: $($health.Content)"

$body = @{
    email    = 'admin@example.com'
    password = 'ChangeMe123!'
} | ConvertTo-Json

Write-Host 'Checking login endpoint...'
$login = Invoke-WebRequest "$baseUrl/api/v1/auth/login" -Method POST -ContentType 'application/json' -Body $body -UseBasicParsing -TimeoutSec 5
if ($login.StatusCode -ne 200) {
    throw "Login failed with status $($login.StatusCode)"
}

$result = $login.Content | ConvertFrom-Json
if (-not $result.token) {
    throw 'Login succeeded but token is empty'
}

Write-Host "Login OK: token acquired for $($result.user.email)"

$headers = @{ Authorization = "Bearer $($result.token)" }

Write-Host 'Checking current user endpoint...'
$me = Invoke-WebRequest "$baseUrl/api/v1/me" -Headers $headers -UseBasicParsing -TimeoutSec 5
if ($me.StatusCode -ne 200) {
    throw "GET /me failed with status $($me.StatusCode)"
}

Write-Host 'Smoke test passed.'
