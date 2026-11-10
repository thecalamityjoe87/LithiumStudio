#!/usr/bin/env bash
# Builds Lithium_Studio-<version>-macos.dmg, with one app that runs on both
# Apple Silicon and Intel Macs.
#   packaging/macos/build-dmg.sh
# Needs Xcode (or its command line tools), CMake and Ninja.
#
# Unsigned (the default), the app gets an ad-hoc signature, and macOS asks
# before opening it the first time. To sign with a Developer ID and notarize
# (needs an Apple Developer account), set before running:
#   LITHIUM_SIGN_APP="Developer ID Application: Isaac Joseph (TEAMID)"
#   LITHIUM_NOTARY_PROFILE=<profile saved with: xcrun notarytool store-credentials>
# build-pkg.sh then packs the same app into the installer.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build-dmg"
app_name="Lithium Studio.app"
exe_path="Contents/MacOS/Lithium Studio"

# sfizz can't be built for both architectures at once (see CMakeLists.txt),
# so each gets a Release build of its own, and lipo joins the two programs.
for arch in arm64 x86_64; do
    cmake -S "$root" -B "$root/build-macos-$arch" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$arch"
    cmake --build "$root/build-macos-$arch"
done

version="$(cat "$root/build-macos-arm64/version.txt")"   # written by CMake
arm_app="$root/build-macos-arm64/LithiumStudio_artefacts/Release/$app_name"
intel_app="$root/build-macos-x86_64/LithiumStudio_artefacts/Release/$app_name"

rm -rf "$out"
mkdir -p "$out/dmg"
app="$out/dmg/$app_name"

# Everything but the program is the same in both builds (Info.plist, icon,
# sounds), so the Apple Silicon one is the starting point.
ditto "$arm_app" "$app"
lipo -create "$arm_app/$exe_path" "$intel_app/$exe_path" -output "$app/$exe_path"

# Apple Silicon Macs don't run a program without a signature, and lipo's
# output has none. With a Developer ID, the hardened runtime goes on, with
# the entitlements a studio needs (audio input, other developers' plug-ins).
# Without one, an ad-hoc signature ("-"): macOS then asks before opening the
# app the first time (System Settings > Privacy & Security > Open Anyway).
if [[ -n "${LITHIUM_SIGN_APP:-}" ]]; then
    codesign --force --deep --timestamp --options runtime \
        --entitlements "$root/packaging/macos/entitlements.plist" --sign "$LITHIUM_SIGN_APP" "$app"
else
    codesign --force --sign - "$app"
fi

# The disk image shows the app next to a link to Applications, to drag it onto.
ln -s /Applications "$out/dmg/Applications"
dmg="$out/Lithium_Studio-$version-macos.dmg"
hdiutil create -volname "Lithium Studio" -srcfolder "$out/dmg" -format UDZO -ov "$dmg"

if [[ -n "${LITHIUM_SIGN_APP:-}" ]]; then
    codesign --force --timestamp --sign "$LITHIUM_SIGN_APP" "$dmg"
fi

if [[ -n "${LITHIUM_NOTARY_PROFILE:-}" ]]; then
    xcrun notarytool submit "$dmg" --keychain-profile "$LITHIUM_NOTARY_PROFILE" --wait
    xcrun stapler staple "$dmg"
fi
