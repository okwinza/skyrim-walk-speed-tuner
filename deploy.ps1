$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$buildDll          = Join-Path $PSScriptRoot "build\release\WalkSpeedTuner.dll"
$releaseTree       = Join-Path $PSScriptRoot "release\SKSE"
$releasePluginsDir = Join-Path $releaseTree "Plugins"
$releaseDll        = Join-Path $releasePluginsDir "WalkSpeedTuner.dll"

Write-Host "=== Building ===" -ForegroundColor Cyan
& "$PSScriptRoot\build.ps1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed; deploy aborted." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $buildDll)) {
    Write-Host "Built DLL missing at $buildDll" -ForegroundColor Red
    exit 1
}

Write-Host "=== Staging release/ ===" -ForegroundColor Cyan
New-Item -ItemType Directory -Path $releasePluginsDir -Force | Out-Null
Copy-Item $buildDll $releaseDll -Force
Write-Host "Staged $releaseDll" -ForegroundColor Green

# Optional MO2 / loose-file deploy: set $env:SKYRIM_MODS_FOLDER to your MO2
# `mods` directory and we'll copy the release tree into a WalkSpeedTuner mod
# folder under it. Skipped silently if unset.
if ($env:SKYRIM_MODS_FOLDER -and (Test-Path $env:SKYRIM_MODS_FOLDER)) {
    $modSkse = Join-Path $env:SKYRIM_MODS_FOLDER "WalkSpeedTuner\SKSE"
    Write-Host "=== Deploying to $modSkse ===" -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $modSkse -Force | Out-Null
    try {
        Copy-Item -Path "$releaseTree\*" -Destination $modSkse -Recurse -Force -ErrorAction Stop
        Write-Host "=== Deploy succeeded -> $modSkse ===" -ForegroundColor Green
    } catch {
        Write-Host "=== Deploy FAILED ===" -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
        Write-Host "Likely cause: Skyrim or MO2 has the DLL locked. Quit the game and re-run." -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "SKYRIM_MODS_FOLDER not set; skipping MO2 deploy." -ForegroundColor Yellow
    Write-Host "  Built DLL is at $releaseDll" -ForegroundColor Yellow
}
