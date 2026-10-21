#!/usr/bin/env bash
# =====================================================================================
# CYD Marauder - cloud / Linux build.
# Installs arduino-cli + the ESP32 core, compiles the sketch against the bundled libs,
# and merges a single flashable image (flash at 0x0 over USB, OR let the device pull it
# via Device > Update Firmware > WiFi Update once attached to a GitHub Release).
#
# Usage:
#   bash build.sh            # just build  -> marauder_cyd_v1_0x0.bin
#   bash build.sh --release  # build AND publish a GitHub release with the .bin (needs gh auth)
#
# Network note (cloud / restricted sandboxes): allow these hosts so the core and
# esptool can download:  github.com, raw.githubusercontent.com, objects.githubusercontent.com,
# espressif.github.io, downloads.arduino.cc, dl.espressif.com, files.pythonhosted.org, pypi.org
# =====================================================================================
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$ROOT/bin"; mkdir -p "$BIN_DIR"
export PATH="$BIN_DIR:$PATH"

FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs,FlashSize=4M,PSRAM=disabled,CPUFreq=240"
CORE_VER="2.0.17"
OUT="$ROOT/marauder_cyd_v1_0x0.bin"

echo "==> arduino-cli"
if ! command -v arduino-cli >/dev/null 2>&1; then
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$BIN_DIR" sh
fi

echo "==> ESP32 core $CORE_VER"
arduino-cli config init --overwrite >/dev/null 2>&1 || true
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json >/dev/null 2>&1 || true
arduino-cli core update-index
arduino-cli core install "esp32:esp32@$CORE_VER"

echo "==> esptool"
python3 -m pip install --quiet --user esptool 2>/dev/null || pip3 install --quiet esptool 2>/dev/null || true

echo "==> compile"
# NOTE: the --allow-multiple-definition linker flag is REQUIRED (this codebase defines
# functions/globals directly in headers included by many .cpp). Linux has no 260-char path
# limit, so the Windows "short path" workaround is unnecessary here.
arduino-cli compile --fqbn "$FQBN" \
  --libraries "$ROOT/source/libs" \
  --build-property "compiler.c.elf.extra_flags=-Wl,--allow-multiple-definition" \
  --build-path "$ROOT/build" \
  "$ROOT/source/esp32_marauder"

echo "==> merge to a single 0x0 image"
CORE_DIR="$HOME/.arduino15/packages/esp32/hardware/esp32/$CORE_VER"
BOOTAPP="$CORE_DIR/tools/partitions/boot_app0.bin"
python3 -m esptool --chip esp32 merge-bin -o "$OUT" \
  --flash-mode dio --flash-freq 40m --flash-size 4MB \
  0x1000  "$ROOT/build/esp32_marauder.ino.bootloader.bin" \
  0x8000  "$ROOT/build/esp32_marauder.ino.partitions.bin" \
  0xe000  "$BOOTAPP" \
  0x10000 "$ROOT/build/esp32_marauder.ino.bin"
echo "==> built: $OUT"

if [ "${1:-}" = "--release" ]; then
  TAG="cloud-$(date +%Y%m%d-%H%M%S)"
  echo "==> publishing GitHub release $TAG"
  # Uses the REST API so it works even where tag pushes are blocked (e.g. the cloud sandbox).
  gh release create "$TAG" "$OUT" --title "$TAG" --notes "Automated cloud build" \
    || gh api -X POST "repos/{owner}/{repo}/releases" -f tag_name="$TAG" -f name="$TAG" >/dev/null
  echo "==> released $TAG (attach $OUT in the release UI if the API path was used)"
fi
