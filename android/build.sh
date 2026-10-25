#!/bin/bash
# Construit sde.apk sans Android Studio : aapt, javac, dx, zipalign, apksigner.
# Usage : ./build.sh <versionCode> <versionName> <firmware de la page embarquée> <keystore> <mot de passe>
set -e
cd "$(dirname "$0")"
CODE=$1; NAME=$2; FW=$3; KS=$4; PASS=$5
JAR=/usr/lib/android-sdk/platforms/android-23/android.jar
rm -rf build && mkdir -p build/gen/com/sde/station build/classes
# Page embarquée (utilisée tant que l'appli n'a pas vu la station) : celle de app/, allégée comme dans le firmware.
node -e 'const fs=require("fs");fs.writeFileSync("assets/page.html",fs.readFileSync("../app/sde-analyse.html","utf8").split("\n").map(l=>l.trim()).filter(l=>l&&!l.startsWith("//")).join("\n"))'

cat > build/gen/com/sde/station/BuildInfo.java <<J
package com.sde.station;
final class BuildInfo {
    static final String VERSION = "$NAME";
    static final String PAGE_FW = "$FW";   // firmware d'où vient assets/page.html
}
J
javac -nowarn -source 8 -target 8 -bootclasspath $JAR -d build/classes $(find src build/gen -name '*.java') 2>&1 | grep -v "^warning\|^Note\|^1 warning" || true
dalvik-exchange --dex --min-sdk-version=23 --output=build/classes.dex build/classes
aapt package -f -M AndroidManifest.xml -S res -A assets -I $JAR -F build/base.apk -0 arsc \
  --min-sdk-version 23 --target-sdk-version 34 --version-code $CODE --version-name $NAME
(cd build && aapt add -f base.apk classes.dex >/dev/null)
zipalign -p -f 4 build/base.apk build/aligned.apk
apksigner sign --ks "$KS" --ks-pass pass:$PASS --key-pass pass:$PASS --out build/sde.apk build/aligned.apk
apksigner verify --print-certs build/sde.apk | head -2
ls -l build/sde.apk
