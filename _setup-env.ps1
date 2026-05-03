# Dot-sourced by build.ps1, deploy.ps1, run-tests.ps1.
#
# Order of operations:
#   1. Load .env then .env.local (later overrides earlier; neither overrides
#      vars already set in the shell — shell-level env wins)
#   2. Resolve VCVARSALL via vswhere if still unset
#   3. Resolve VCPKG_ROOT via ./vcpkg/ if still unset
#   4. Import MSVC environment into this process

# --- Load .env files (dotenv-flow style) ---
function Import-DotEnv {
    param([string]$Path)
    if (-not (Test-Path $Path)) { return }
    foreach ($line in Get-Content $Path) {
        $trimmed = $line.Trim()
        if ($trimmed -eq '' -or $trimmed.StartsWith('#')) { continue }
        if ($trimmed -match '^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$') {
            $key = $matches[1]
            $val = $matches[2].Trim()
            # Strip surrounding quotes if present
            if (($val.StartsWith('"') -and $val.EndsWith('"')) -or
                ($val.StartsWith("'") -and $val.EndsWith("'"))) {
                $val = $val.Substring(1, $val.Length - 2)
            }
            # Don't override vars already set in the shell (matches dotenv
            # standard — shell env wins).
            if (-not [System.Environment]::GetEnvironmentVariable($key, 'Process')) {
                [System.Environment]::SetEnvironmentVariable($key, $val, 'Process')
            }
        }
    }
}

# Import in cascade order: .env first (so .env.local overrides it).
Import-DotEnv (Join-Path $PSScriptRoot ".env")
Import-DotEnv (Join-Path $PSScriptRoot ".env.local")

# --- Locate vcvarsall ---
if (-not $env:VCVARSALL -or -not (Test-Path $env:VCVARSALL)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -prerelease -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($vsPath) {
            $candidate = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
            if (Test-Path $candidate) { $env:VCVARSALL = $candidate }
        }
    }
}
if (-not $env:VCVARSALL -or -not (Test-Path $env:VCVARSALL)) {
    Write-Host "vcvarsall.bat not found." -ForegroundColor Red
    Write-Host "  Install MSVC C++ tools, or set VCVARSALL in .env.local." -ForegroundColor Yellow
    exit 1
}

# --- Locate vcpkg ---
if (-not $env:VCPKG_ROOT) {
    $localVcpkg = Join-Path $PSScriptRoot "vcpkg"
    if (Test-Path (Join-Path $localVcpkg "vcpkg.exe")) {
        $env:VCPKG_ROOT = $localVcpkg
    }
}
if (-not $env:VCPKG_ROOT -or -not (Test-Path $env:VCPKG_ROOT)) {
    Write-Host "vcpkg not found." -ForegroundColor Red
    Write-Host "  Either: set VCPKG_ROOT in .env.local" -ForegroundColor Yellow
    Write-Host "  Or:     git clone https://github.com/microsoft/vcpkg.git ./vcpkg; ./vcpkg/bootstrap-vcpkg.bat" -ForegroundColor Yellow
    exit 1
}

# --- Import MSVC environment ---
$envBlock = cmd.exe /c "`"$env:VCVARSALL`" x64 >nul 2>&1 && set" 2>&1
foreach ($line in $envBlock) {
    if ($line -match "^(.+?)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}
