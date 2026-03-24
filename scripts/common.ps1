Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-GoExecutable {
    $candidates = @()

    try {
        $cmd = Get-Command go -ErrorAction Stop
        if ($cmd.Path) {
            $candidates += $cmd.Path
        }
    } catch {
    }

    $candidates += @(
        'C:\Program Files\Go\bin\go.exe',
        'C:\Go\bin\go.exe'
    )

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    throw "go.exe not found. Install Go or add it to PATH."
}

function Initialize-GoEnvironment {
    param(
        [string]$ProjectRoot
    )

    $script:GOCmd = Get-GoExecutable
    $env:GOCACHE = Join-Path $ProjectRoot '.gocache'
    $env:GOMODCACHE = Join-Path $ProjectRoot '.gomodcache'
    $env:GOFLAGS = '-buildvcs=false'

    New-Item -ItemType Directory -Force -Path $env:GOCACHE | Out-Null
    New-Item -ItemType Directory -Force -Path $env:GOMODCACHE | Out-Null
}

function Invoke-Go {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Args
    )

    & $script:GOCmd @Args
    if ($LASTEXITCODE -ne 0) {
        throw "Go command failed: go $($Args -join ' ')"
    }
}
