Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Initialize-ConsoleEncoding {
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    [Console]::InputEncoding = $utf8
    [Console]::OutputEncoding = $utf8
    $OutputEncoding = $utf8
}

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

    Initialize-ConsoleEncoding
    $script:GOCmd = Get-GoExecutable
    $env:GOCACHE = Join-Path $ProjectRoot '.gocache'
    $env:GOMODCACHE = Join-Path $ProjectRoot '.gomodcache'
    $env:GOFLAGS = '-buildvcs=false'
    if (-not $env:GOPROXY) {
        $env:GOPROXY = 'https://goproxy.cn,direct'
    }

    New-Item -ItemType Directory -Force -Path $env:GOCACHE | Out-Null
    New-Item -ItemType Directory -Force -Path $env:GOMODCACHE | Out-Null
}

function Get-PortFromAddress {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Address
    )

    if ($Address -match ':(\d+)$') {
        return [int]$Matches[1]
    }

    throw "Unable to parse port from address: $Address"
}

function Assert-PortAvailable {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Port
    )

    $listener = $null
    try {
        $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, $Port)
        $listener.Start()
    } catch {
        $conn = $null
        try {
            $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction Stop | Select-Object -First 1
        } catch {
        }

        if ($conn) {
            $owner = ""
            try {
                $process = Get-Process -Id $conn.OwningProcess -ErrorAction Stop
                $owner = ", process: $($process.ProcessName) (PID $($process.Id))"
            } catch {
                $owner = ", PID: $($conn.OwningProcess)"
            }
            throw "Port $Port is already in use$owner. Stop it first, or change HTTP_ADDR and try again."
        }

        throw "Port $Port is already in use. Stop it first, or change HTTP_ADDR."
    } finally {
        if ($listener) {
            $listener.Stop()
        }
    }
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
