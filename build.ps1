$ErrorActionPreference = "Continue"

# Source MSVC environment
$vcvars = "D:\Software\VS Build\VC\Auxiliary\Build\vcvarsall.bat"
$env:VCPKG_ROOT = "D:\Software\VS Build\VC\vcpkg"

# vcvarsall must run in cmd, capture its env and import into PowerShell
$envBlock = cmd.exe /c "`"$vcvars`" x64 >nul 2>&1 && set" 2>&1
foreach ($line in $envBlock) {
    if ($line -match "^(.+?)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

Set-Location "S:\Nolvus\Instances\Nolvus Ascension\MODS\dev\WalkSpeedTuner"

Write-Host "=== CMake Configure ===" -ForegroundColor Cyan
$configOut = cmake --preset release 2>&1
$configOut | Where-Object { $_ -notmatch "^cmake :" -and $_ -notmatch "NativeCommandError" } | ForEach-Object { $_ }
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configure FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "=== Building ===" -ForegroundColor Cyan
cmake --build build/release 2>&1 | ForEach-Object { $_ }
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "=== Build succeeded ===" -ForegroundColor Green
