# Go Agent Platform Dev Guide

## Quick Start

Windows PowerShell:

```powershell
.\scripts\test.ps1
.\scripts\dev.ps1
```

Only start the API:

```powershell
.\scripts\dev.ps1 -NoWorker
```

Only start the worker:

```powershell
.\scripts\worker.ps1
```

Build binaries:

```powershell
.\scripts\build.ps1
```

## What The Scripts Standardize

- Detect `go.exe`
- Create and use local `.gocache` and `.gomodcache`
- Set `GOFLAGS=-buildvcs=false`
- Avoid dependency on whether the current shell refreshed PATH
- Avoid permission issues with the default system cache directories

## Default Credentials

- email: `admin@example.com`
- password: `ChangeMe123!`
