$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

. "$PSScriptRoot\_setup-env.ps1"

# CommonLibSSE-NG lives as a git submodule. If the marker header is
# missing, init/update before configuring.
$clsHeader = Join-Path $PSScriptRoot "extern\CommonLibSSE-NG\include\REL\Relocation.h"
if (-not (Test-Path $clsHeader)) {
    Write-Host "=== Initializing CommonLibSSE-NG submodule ===" -ForegroundColor Cyan
    git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Submodule init FAILED" -ForegroundColor Red
        exit 1
    }
}

Write-Host "=== CMake Configure ===" -ForegroundColor Cyan
cmake --preset release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Configure FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "=== Build ===" -ForegroundColor Cyan
cmake --build build/release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "=== Build succeeded ===" -ForegroundColor Green
