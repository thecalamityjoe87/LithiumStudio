# Builds Lithium_Studio-<version>-windows-x64.zip from the Release build.
#   packaging/windows/build-zip.ps1
# Run it from a Developer PowerShell for Visual Studio (so the compiler and
# Ninja are on the PATH), after configuring the build once:
#   cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
$ErrorActionPreference = "Stop"

$root = (Resolve-Path "$PSScriptRoot/../..").Path
$build = Join-Path $root "build-release"
$out = Join-Path $root "build-zip"

cmake --build $build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$version = (Get-Content (Join-Path $build "version.txt")).Trim()   # written by CMake
$artefacts = Join-Path $build "LithiumStudio_artefacts/Release"

# The zip holds one folder: the program, the sound packs it looks for next
# to itself, and the license and credits (as .txt, so they open in Notepad).
# The program needs nothing else: the C++ runtime is linked into it.
$stage = Join-Path $out "Lithium Studio"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force $stage | Out-Null

Copy-Item (Join-Path $artefacts "Lithium Studio.exe") $stage
Copy-Item -Recurse (Join-Path $artefacts "Sounds") $stage
Copy-Item (Join-Path $root "LICENSE") (Join-Path $stage "LICENSE.txt")
Copy-Item (Join-Path $root "CREDITS") (Join-Path $stage "CREDITS.txt")

Compress-Archive -Path $stage -DestinationPath (Join-Path $out "Lithium_Studio-$version-windows-x64.zip")
