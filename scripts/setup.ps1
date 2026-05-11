[CmdletBinding()]
param(
    [string]$VcpkgDir = (Join-Path $env:USERPROFILE 'vcpkg'),
    [switch]$SkipWinget
)

$ErrorActionPreference = 'Stop'

function Write-Section {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Test-Command {
    param([string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Set-PersistentEnv {
    param(
        [string]$Name,
        [string]$Value
    )

    [Environment]::SetEnvironmentVariable($Name, $Value, 'User')
    Set-Item -Path "Env:$Name" -Value $Value
    Write-Host "    $Name = $Value" -ForegroundColor DarkGray
}

function Install-Winget {
    param(
        [string]$Id,
        [string]$Friendly
    )

    if ($SkipWinget) {
        Write-Host "    (skipping winget install of $Friendly because -SkipWinget was passed)" -ForegroundColor Yellow
        return
    }

    if (-not (Test-Command 'winget')) {
        Write-Host "    winget not available; please install $Friendly manually." -ForegroundColor Yellow
        return
    }

    Write-Host "    Installing $Friendly via winget ($Id)..." -ForegroundColor DarkGray
    & winget install --id $Id --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne -1978335189) {
        throw "winget install $Id failed with exit code $LASTEXITCODE"
    }
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Magenta
Write-Host "   Graphyne - Windows setup" -ForegroundColor Magenta
Write-Host "======================================" -ForegroundColor Magenta

Write-Section "Checking base toolchain"

if (-not (Test-Command 'git')) {
    Install-Winget 'Git.Git' 'Git'
} else {
    Write-Host "    Git already installed ($(git --version))"
}

if (-not (Test-Command 'cmake')) {
    Install-Winget 'Kitware.CMake' 'CMake'
} else {
    Write-Host "    CMake already installed ($(cmake --version | Select-Object -First 1))"
}

if (-not (Test-Command 'ninja')) {
    Install-Winget 'Ninja-build.Ninja' 'Ninja'
} else {
    Write-Host "    Ninja already installed ($(ninja --version))"
}

Write-Section "Checking Vulkan SDK"

if (-not $env:VULKAN_SDK -or -not (Test-Path $env:VULKAN_SDK)) {
    Install-Winget 'KhronosGroup.VulkanSDK' 'Vulkan SDK'
    $machineSdk = [Environment]::GetEnvironmentVariable('VULKAN_SDK', 'Machine')
    if ($machineSdk) {
        $env:VULKAN_SDK = $machineSdk
        Write-Host "    Picked up VULKAN_SDK = $machineSdk"
    } else {
        Write-Host "    WARNING: VULKAN_SDK not set. Re-open your shell so the installer's env vars take effect." -ForegroundColor Yellow
    }
} else {
    Write-Host "    VULKAN_SDK = $env:VULKAN_SDK"
}

Write-Section "Checking vcpkg"

if (-not (Test-Path (Join-Path $VcpkgDir '.git'))) {
    Write-Host "    Cloning vcpkg into $VcpkgDir ..."
    if (Test-Path $VcpkgDir) {
        throw "$VcpkgDir exists but doesn't contain a git repo. Remove it or pass -VcpkgDir with another path."
    }

    git clone --depth 1 https://github.com/microsoft/vcpkg.git $VcpkgDir
} else {
    Write-Host "    vcpkg already cloned in $VcpkgDir"
}

$bootstrap = Join-Path $VcpkgDir 'bootstrap-vcpkg.bat'
$vcpkgExe = Join-Path $VcpkgDir 'vcpkg.exe'
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "    Bootstrapping vcpkg..."
    & $bootstrap -disableMetrics
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg bootstrap failed"
    }
} else {
    Write-Host "    vcpkg.exe already present"
}

Set-PersistentEnv 'VCPKG_ROOT' $VcpkgDir
if (-not ($env:PATH -split ';' | Where-Object { $_ -eq $VcpkgDir })) {
    $env:PATH = $VcpkgDir + ';' + $env:PATH
}

Write-Section "Installing vcpkg manifest dependencies"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Push-Location $repoRoot
try {
    if (-not (Test-Path 'vcpkg.json')) {
        throw "vcpkg.json not found at $repoRoot - are you running this from the repo root?"
    }

    Write-Host "    Running 'vcpkg install --triplet x64-windows' in manifest mode..."
    & $vcpkgExe install --triplet x64-windows
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install failed"
    }
}
finally {
    Pop-Location
}

Write-Section "Setup complete"
Write-Host ""
Write-Host "    Restart your shell (or run 'refreshenv') so VCPKG_ROOT / VULKAN_SDK" -ForegroundColor Green
Write-Host "    are visible in new processes. Then you can configure and build:" -ForegroundColor Green
Write-Host ""
Write-Host "        cmake --preset default" -ForegroundColor Green
Write-Host "        cmake --build --preset default" -ForegroundColor Green
Write-Host ""
Write-Host "    Or use the convenience batch script:" -ForegroundColor Green
Write-Host ""
Write-Host "        build.bat" -ForegroundColor Green
