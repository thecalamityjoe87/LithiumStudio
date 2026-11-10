#!/usr/bin/env bash
# Builds Lithium_Studio-<version>-x86_64.AppImage from the Release build.
#   packaging/appimage/build-appimage.sh
# Downloads linuxdeploy and appimagetool into build-appimage/tools on first run.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build-appimage"
tools="$out/tools"
appdir="$out/AppDir"
app_id="io.github.thecalamityjoe87.LithiumStudio"

mkdir -p "$tools"
fetch() {  # fetch <name> <url>
    [[ -x "$tools/$1" ]] || { curl -fsSL -o "$tools/$1" "$2" && chmod +x "$tools/$1"; }
}
fetch linuxdeploy  https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fetch appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage

# Run the tools without needing FUSE.
export APPIMAGE_EXTRACT_AND_RUN=1

cmake --build "$root/build-release" -j3
version="$(cat "$root/build-release/version.txt")"   # written by CMake

rm -rf "$appdir"
DESTDIR="$appdir" cmake --install "$root/build-release" --prefix /usr

# Copies the shared libraries the binary needs into the AppDir.
"$tools/linuxdeploy" --appdir "$appdir" \
    --desktop-file "$appdir/usr/share/applications/$app_id.desktop" \
    --icon-file "$root/resources/icons/$app_id.svg" \
    --executable "$appdir/usr/bin/lithium-studio"

# appimagetool makes .DirIcon (the AppImage's own icon) from a PNG named after
# the desktop file's Icon= at the top of the AppDir; linuxdeploy only put the
# SVG there.
cp "$appdir/usr/share/icons/hicolor/256x256/apps/$app_id.png" "$appdir/$app_id.png"

# --no-appstream: appimagetool only looks for the old <id>.appdata.xml name, not
# the <id>.metainfo.xml we install, and validating it would need Ubuntu 22.04's
# appstreamcli (0.15), which predates tags the file uses (<developer>). The
# metadata still ships in usr/share/metainfo; check it with
# appstreamcli validate packaging/linux/<id>.metainfo.xml
ARCH=x86_64 VERSION="$version" "$tools/appimagetool" --no-appstream "$appdir" \
    "$out/Lithium_Studio-$version-x86_64.AppImage"
