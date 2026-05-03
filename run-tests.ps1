$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

. "$PSScriptRoot\_setup-env.ps1"

Write-Host "=== Configure tests ===" -ForegroundColor Cyan
cmake --preset tests
if ($LASTEXITCODE -ne 0) { Write-Host "Configure FAILED" -ForegroundColor Red; exit 1 }

Write-Host "=== Build tests ===" -ForegroundColor Cyan
cmake --build build/tests
if ($LASTEXITCODE -ne 0) { Write-Host "Build FAILED" -ForegroundColor Red; exit 1 }

Write-Host "=== Run tests ===" -ForegroundColor Cyan
ctest --test-dir build/tests --output-on-failure
exit $LASTEXITCODE
