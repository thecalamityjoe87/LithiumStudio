#!/usr/bin/env bash
# Builds dist/Lithium_Studio-<version>-linux-x86_64.tar.gz: the graphical installer
# ("Install Lithium Studio", see tools/installer) with the files it installs
# beside it in payload/, as Lithium Drums' Linux download is laid out.
#   packaging/linux/build-installer.sh
# Run it after configuring the Release build once:
#   cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
build="$root/build-release"
out="$root/build-installer"   # the staging folder
dist="$root/dist"              # where the finished installers go
app_id="io.github.thecalamityjoe87.LithiumStudio"

cmake --build "$build" -j2 --target LithiumStudio LithiumStudioInstaller
version="$(cat "$build/version.txt")"   # written by CMake

name="Lithium_Studio-$version-linux-$(uname -m)"
stage="$out/$name"
payload="$stage/payload"
rm -rf "$out"
mkdir -p "$payload/app" "$payload/icons"

# What the installer installs (see tools/installer/Installation.h).
cp "$build/LithiumStudio_artefacts/Release/Lithium Studio" "$payload/app/"
cp -R "$root/resources/sounds" "$payload/app/Sounds"
cp "$root/packaging/linux/icons/"icon-*.png "$payload/icons/"
cp "$root/resources/icons/$app_id.svg" "$payload/icons/icon.svg"
cp "$root/packaging/linux/$app_id.xml" "$payload/"
cp "$root/LICENSE" "$root/CREDITS" "$payload/"

# The installer, the licence, and how to use it.
cp "$build/LithiumStudioInstaller_artefacts/Release/Install Lithium Studio" "$stage/"
cp "$root/LICENSE" "$stage/"
cat > "$stage/README.txt" <<README
Lithium Studio $version, by Isaac Joseph (Calamity Audio)

To install, double-click "Install Lithium Studio".

It installs Lithium Studio into your home folder (no password needed), adds
it to your app menu, and lets you open sessions (.lisession) from your file
manager. Run it again to update or remove Lithium Studio.

From a terminal:  ./"Install Lithium Studio" --install   (or --remove, --help)

Lithium Studio is free software under the GNU GPLv3 or later; see LICENSE.
README

mkdir -p "$dist"
tar -C "$out" -czf "$dist/$name.tar.gz" "$name"
echo "Created $dist/$name.tar.gz"
