$ErrorActionPreference = "Continue"

Set-Location "S:\Nolvus\Instances\Nolvus Ascension\MODS\dev\WalkSpeedTuner"

$projectRoot = (Get-Location).Path
$buildDll    = Join-Path $projectRoot "build\release\WalkSpeedTuner.dll"
$releaseTree = Join-Path $projectRoot "release\SKSE"
$releaseDll  = Join-Path $releaseTree "Plugins\WalkSpeedTuner.dll"
$modSkse     = "S:\Nolvus\Instances\Nolvus Ascension\MODS\mods\WalkSpeedTuner\SKSE"

# 1. Build (delegates to build.ps1 — same vcvars/vcpkg setup)
Write-Host "=== Building ===" -ForegroundColor Cyan
& "$projectRoot\build.ps1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed; deploy aborted." -ForegroundColor Red
    exit 1
}

# 2. Refresh release tree with the freshly-built DLL
Write-Host "=== Staging release ===" -ForegroundColor Cyan
if (-not (Test-Path $buildDll)) {
    Write-Host "Built DLL missing at $buildDll" -ForegroundColor Red
    exit 1
}
$releasePluginsDir = Split-Path $releaseDll -Parent
if (-not (Test-Path $releasePluginsDir)) {
    New-Item -ItemType Directory -Path $releasePluginsDir -Force | Out-Null
}
Copy-Item $buildDll $releaseDll -Force
Write-Host "Staged $releaseDll"

# 3. Copy release tree contents into the MO2 mod folder
Write-Host "=== Deploying to MO2 ===" -ForegroundColor Cyan
if (-not (Test-Path $modSkse)) {
    New-Item -ItemType Directory -Path $modSkse -Force | Out-Null
}
try {
    Copy-Item -Path "$releaseTree\*" -Destination $modSkse -Recurse -Force -ErrorAction Stop
    Write-Host "=== Deploy succeeded -> $modSkse ===" -ForegroundColor Green
} catch {
    Write-Host "=== Deploy FAILED ===" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Likely cause: Skyrim or MO2 has the DLL locked. Quit the game and re-run ./deploy.ps1." -ForegroundColor Yellow
    exit 1
}
