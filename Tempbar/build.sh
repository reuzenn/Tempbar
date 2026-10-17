#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

echo "==> Compiling native SMC in-process bridge..."
clang++ -std=c++17 -O3 -c smc_bridge.cpp -o smc_bridge.o

echo "==> Compiling standalone macos-temp CLI helper..."
clang++ -std=c++17 -O3 -framework IOKit -framework CoreFoundation macos.cpp -o macos-temp

echo "==> Compiling Swift application bundle..."
swiftc -O -parse-as-library \
    -import-objc-header TempbarBridge.h \
    Models.swift Localization.swift HardwareMonitor.swift PopoverContentView.swift TempbarApp.swift \
    smc_bridge.o \
    -lc++ \
    -framework IOKit -framework CoreFoundation \
    -o Tempbar

echo "==> Packaging application bundle..."
APP_BUNDLE="Tempbar.app"
rm -rf "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"

cp Tempbar "$APP_BUNDLE/Contents/MacOS/"
cp macos-temp "$APP_BUNDLE/Contents/MacOS/"
cp macos-temp "$APP_BUNDLE/Contents/Resources/"
cp Info.plist "$APP_BUNDLE/Contents/"

chmod +x "$APP_BUNDLE/Contents/MacOS/Tempbar"
chmod +x "$APP_BUNDLE/Contents/MacOS/macos-temp"

# Clean intermediate object file
rm -f smc_bridge.o

echo "==> Build complete: $APP_BUNDLE"
