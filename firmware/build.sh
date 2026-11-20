#!/bin/bash
# Compile le firmware avec arduino-cli (cœur ESP32 2.0.17) et prépare les deux binaires :
#   dist/sde-firmware-<version>-ota.bin     mise à jour par Wi-Fi (appli ou /api/update)
#   dist/sde-firmware-<version>-merged.bin  image complète à flasher par USB à l'adresse 0x0
# Prérequis : arduino-cli, Node.js, esptool (pip install esptool), voir README.
set -euo pipefail
cd "$(dirname "$0")"
VER=$(cat VERSION)
CORE=2.0.17
FQBN=esp32:esp32:esp32
URL=https://espressif.github.io/arduino-esp32/package_esp32_index.json

if [ "${1:-}" = "--setup" ]; then
  arduino-cli core update-index --additional-urls $URL
  arduino-cli core install esp32:esp32@$CORE --additional-urls $URL
  arduino-cli lib install "Adafruit BME680 Library" "Adafruit Unified Sensor" "Adafruit BusIO" \
    ArduinoJson@7.2.1 "Adafruit NeoPixel@1.12.3"
fi

# L'appli (dossier app/) est embarquée compressée dans src/web/assets.h
node tools/gen_assets.js

# Croquis Arduino : un .ino vide + le dossier src/
rm -rf build && mkdir -p build/sketch/sde && touch build/sketch/sde/sde.ino
cp -r src build/sketch/sde/src
# Table de partitions V2 (programme de 1,5 Mo), prise en compte par arduino-cli ;
# upload.maximum_size relève la limite de taille de la carte « esp32 » (1,25 Mo par défaut).
cp partitions.csv build/sketch/sde/
arduino-cli compile --fqbn $FQBN --output-dir build/out \
  --build-property "compiler.cpp.extra_flags=-DSDE_FW_VERSION=\"$VER\"" \
  --build-property "upload.maximum_size=1572864" build/sketch/sde

mkdir -p dist
cp build/out/sde.ino.bin dist/sde-firmware-$VER-ota.bin
# La partition app0/app1 fait 1 572 864 octets (partitions.csv) : un fichier plus gros serait refusé par la mise à jour.
# Une station encore en table V1 (1 310 720 octets) doit d'abord être flashée par USB.
SIZE=$(stat -c %s build/out/sde.ino.bin)
if [ "$SIZE" -gt 1572864 ]; then echo "error: firmware trop gros ($SIZE > 1572864 octets)"; exit 1; fi
echo "Firmware : $SIZE / 1572864 octets ($((1572864 - SIZE)) libres)"
if [ "$SIZE" -gt 1310720 ]; then echo "warning: plus gros que la table V1 : flash USB obligatoire sur une station en 0.9.x"; fi
python3 -m esptool --chip esp32 merge_bin -o dist/sde-firmware-$VER-merged.bin \
  --flash_mode keep --flash_freq keep --flash_size keep \
  0x1000 build/out/sde.ino.bootloader.bin 0x8000 build/out/sde.ino.partitions.bin \
  0xe000 "$(arduino-cli config get directories.data)/packages/esp32/hardware/esp32/$CORE/tools/partitions/boot_app0.bin" \
  0x10000 build/out/sde.ino.bin
ls -l dist
