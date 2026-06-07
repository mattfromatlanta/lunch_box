#!/bin/bash
# Builds, signs, notarizes, and packages Lunch Box for distribution.
#
# Prerequisites:
#   - "Developer ID Application" certificate installed in your keychain
#   - Apple ID app-specific password: https://appleid.apple.com → App-Specific Passwords
#   - xcrun notarytool credentials stored:
#       xcrun notarytool store-credentials "lunch_box" \
#         --apple-id YOUR_APPLE_ID \
#         --team-id YOUR_TEAM_ID \
#         --password APP_SPECIFIC_PASSWORD
#
# Usage:
#   ./make_release.sh

set -e

APP_NAME="Lunch Box"
APP_BUNDLE="build/lunch_box_artefacts/${APP_NAME}.app"
NOTARYTOOL_PROFILE="lunch_box"   # name you used with store-credentials

# ── Resolve version from CMakeLists.txt ────────────────────────────────────────
VERSION=$(grep -m1 'project(lunch_box VERSION' CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
if [[ -z "$VERSION" ]]; then
    echo "Error: could not read version from CMakeLists.txt"
    exit 1
fi
echo "Version: $VERSION"

# ── Find Developer ID certificate ──────────────────────────────────────────────
SIGN_IDENTITY=$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | sed 's/.*"\(Developer ID Application[^"]*\)".*/\1/')
if [[ -z "$SIGN_IDENTITY" ]]; then
    echo ""
    echo "Error: No 'Developer ID Application' certificate found in your keychain."
    echo ""
    echo "To fix:"
    echo "  1. Visit https://developer.apple.com/account/resources/certificates/list"
    echo "  2. Create a 'Developer ID Application' certificate"
    echo "  3. Download and double-click to install it"
    echo "  4. Re-run this script"
    exit 1
fi
echo "Signing identity: $SIGN_IDENTITY"

# ── Build ───────────────────────────────────────────────────────────────────────
echo ""
echo "Building..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null
make -j$(sysctl -n hw.logicalcpu)
cd ..

# ── Sign ────────────────────────────────────────────────────────────────────────
echo ""
echo "Signing..."
codesign --deep --force --options runtime \
    --sign "$SIGN_IDENTITY" \
    "$APP_BUNDLE"

codesign --verify --deep --strict "$APP_BUNDLE"
echo "Signature verified."

# ── Zip for notarization ────────────────────────────────────────────────────────
ZIP_NAME="LunchBox-${VERSION}.zip"
echo ""
echo "Creating ${ZIP_NAME}..."
ditto -c -k --keepParent "$APP_BUNDLE" "$ZIP_NAME"

# ── Notarize ────────────────────────────────────────────────────────────────────
echo ""
echo "Submitting for notarization (this takes a minute or two)..."
xcrun notarytool submit "$ZIP_NAME" \
    --keychain-profile "$NOTARYTOOL_PROFILE" \
    --wait

# ── Staple ──────────────────────────────────────────────────────────────────────
echo ""
echo "Stapling notarization ticket..."
xcrun stapler staple "$APP_BUNDLE"

# ── Repack with stapled app ─────────────────────────────────────────────────────
echo ""
echo "Repacking..."
rm "$ZIP_NAME"
ditto -c -k --keepParent "$APP_BUNDLE" "$ZIP_NAME"

echo ""
echo "Done! Distributable: ${ZIP_NAME}"
