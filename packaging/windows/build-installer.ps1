# Builds the Windows installer, dist/Lithium_Studio-<version>-windows-setup.exe
# (and the zip, in build-zip).
#   packaging/windows/build-installer.ps1
# Run it like build-zip.ps1 (from a Developer PowerShell for Visual Studio,
# after configuring build-release once). Needs Inno Setup 6 or 7
# (choco install innosetup, or https://jrsoftware.org/isinfo.php).
$ErrorActionPreference = "Stop"

$root = (Resolve-Path "$PSScriptRoot/../..").Path
$out = Join-Path $root "build-zip"   # where build-zip.ps1 stages the app
$dist = Join-Path $root "dist"       # where the finished installers go
New-Item -ItemType Directory -Force $dist | Out-Null

# Builds the app and stages it in build-zip/Lithium Studio (and makes the zip).
& (Join-Path $PSScriptRoot "build-zip.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$version = (Get-Content (Join-Path $root "build-release/version.txt")).Trim()   # written by CMake

# Inno Setup's compiler: on the PATH, or where its installer puts it.
$iscc = (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source
if (-not $iscc) {
    $iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
              "${env:ProgramFiles(x86)}\Inno Setup 7\ISCC.exe", "$env:ProgramFiles\Inno Setup 7\ISCC.exe") |
             Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $iscc) { throw "Inno Setup's ISCC.exe wasn't found. Install Inno Setup 6 or 7 first." }

& $iscc "/DAppVersion=$version" "/DStaging=$(Join-Path $out 'Lithium Studio')" "/DOutputDir=$dist" `
        (Join-Path $PSScriptRoot "LithiumStudio.iss")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
