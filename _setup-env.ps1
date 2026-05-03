# Dot-sourced by build.ps1, deploy.ps1, run-tests.ps1.
# Resolves MSVC vcvarsall + vcpkg, then imports MSVC env into the calling
# PowerShell process. Honors $env:VCVARSALL and $env:VCPKG_ROOT if set;
# otherwise discovers via vswhere and ./vcpkg/.

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
    Write-Host "  Install MSVC C++ tools, or set \$env:VCVARSALL to the vcvarsall.bat path." -ForegroundColor Yellow
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
    Write-Host "  Either: \$env:VCPKG_ROOT = 'C:\path\to\vcpkg'" -ForegroundColor Yellow
    Write-Host "  Or: git clone https://github.com/microsoft/vcpkg.git ./vcpkg; ./vcpkg/bootstrap-vcpkg.bat" -ForegroundColor Yellow
    exit 1
}

# --- Import MSVC environment ---
$envBlock = cmd.exe /c "`"$env:VCVARSALL`" x64 >nul 2>&1 && set" 2>&1
foreach ($line in $envBlock) {
    if ($line -match "^(.+?)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}
