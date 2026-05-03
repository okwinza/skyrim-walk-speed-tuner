$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

. "$PSScriptRoot\_setup-env.ps1"

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
