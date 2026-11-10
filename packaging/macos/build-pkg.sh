#!/usr/bin/env bash
# Builds the macOS installer, dist/Lithium_Studio-<version>-macos.pkg, in the style
# of Lithium Drums' installer: welcome, licence, install to Applications.
#   packaging/macos/build-pkg.sh
# Run it after build-dmg.sh, which builds the universal (Apple Silicon and
# Intel) app this packs. Needs Xcode's command line tools (pkgbuild,
# productbuild).
#
# Unsigned, the package works, but macOS warns that it's from an unidentified
# developer. To sign and notarize (needs an Apple Developer account), set
# LITHIUM_SIGN_APP and LITHIUM_NOTARY_PROFILE for build-dmg.sh (which signs
# the app), and before running this:
#   LITHIUM_SIGN_INSTALLER="Developer ID Installer: Isaac Joseph (TEAMID)"
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
app="$root/build-dmg/dmg/Lithium Studio.app"
id="io.github.thecalamityjoe87.LithiumStudio"

if [[ ! -d "$app" ]]; then
    echo "No app at $app: run packaging/macos/build-dmg.sh first." >&2
    exit 1
fi

version="$(cat "$root/build-macos-arm64/version.txt")"   # written by CMake
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# The app's own package, installed to /Applications. Bundle relocation is
# off: otherwise Installer "upgrades" a copy it finds anywhere on the disk
# (an old build, say) instead of installing to Applications.
mkdir -p "$work/root" "$work/pkgs" "$work/resources"
ditto "$app" "$work/root/Lithium Studio.app"
pkgbuild --analyze --root "$work/root" "$work/app.plist" > /dev/null
plutil -replace BundleIsRelocatable -bool NO "$work/app.plist" 2> /dev/null || true
pkgbuild --root "$work/root" --component-plist "$work/app.plist" \
    --identifier "$id.app" --version "$version" --install-location /Applications \
    "$work/pkgs/app.pkg" > /dev/null

# The pages, the licence and the background (drawn by tools/make_installer_art.py).
cp "$here/resources/"* "$work/resources/"
cp "$root/LICENSE" "$work/resources/LICENSE.txt"
sed "s/@VERSION@/$version/g" "$here/distribution.xml" > "$work/distribution.xml"

mkdir -p "$root/dist"
pkg="$root/dist/Lithium_Studio-$version-macos.pkg"
sign_args=()
[[ -n "${LITHIUM_SIGN_INSTALLER:-}" ]] && sign_args=(--sign "$LITHIUM_SIGN_INSTALLER" --timestamp)

productbuild --distribution "$work/distribution.xml" --resources "$work/resources" \
    --package-path "$work/pkgs" ${sign_args[@]+"${sign_args[@]}"} "$pkg"

if [[ -n "${LITHIUM_NOTARY_PROFILE:-}" ]]; then
    xcrun notarytool submit "$pkg" --keychain-profile "$LITHIUM_NOTARY_PROFILE" --wait
    xcrun stapler staple "$pkg"
fi

echo "Created $pkg"
