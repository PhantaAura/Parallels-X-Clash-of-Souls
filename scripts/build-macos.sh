#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-macos"
DIST="$ROOT/dist"
STAGE="$BUILD/dmg-stage"
APP_NAME="Parallels X - Clash of Souls.app"
DMG_NAME="Parallels-X-Clash-of-Souls.dmg"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "build-macos.sh must run on macOS because it uses the Apple SDK and hdiutil." >&2
  exit 2
fi

rm -rf "$BUILD" "$STAGE"
mkdir -p "$DIST"
rm -f "$DIST/$DMG_NAME"

cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build "$BUILD" --target ParallelsX --config Release -j "$(sysctl -n hw.logicalcpu)"

APP_PATH="$BUILD/$APP_NAME"
if [[ ! -d "$APP_PATH" ]]; then
  echo "Expected app bundle not found: $APP_PATH" >&2
  exit 3
fi

# Ad-hoc signing keeps local/test builds easy to launch. Release signing/notarization is a later storefront step.
codesign --force --deep --sign - "$APP_PATH"

mkdir -p "$STAGE"
cp -R "$APP_PATH" "$STAGE/"
ln -s /Applications "$STAGE/Applications"

hdiutil create \
  -volname "Parallels X Clash of Souls" \
  -srcfolder "$STAGE" \
  -ov \
  -format UDZO \
  "$DIST/$DMG_NAME"

shasum -a 256 "$DIST/$DMG_NAME" > "$DIST/$DMG_NAME.sha256"
echo "Built: $DIST/$DMG_NAME"
