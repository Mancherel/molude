#!/bin/bash
set -e

echo "=== Molude Linux Build ==="

# Check dependencies
for cmd in cmake g++; do
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: $cmd not found. Install it first."
        exit 1
    fi
done

# Check for required dev libraries
echo "Checking dependencies..."
MISSING=""
for pkg in libasound2-dev libfreetype-dev libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libwebkit2gtk-4.1-dev; do
    if ! dpkg -s "$pkg" &> /dev/null 2>&1; then
        MISSING="$MISSING $pkg"
    fi
done

if [ -n "$MISSING" ]; then
    echo ""
    echo "Missing packages:$MISSING"
    echo ""
    echo "Install with:"
    echo "  sudo apt install$MISSING"
    echo ""
    exit 1
fi

# Init submodule if needed
if [ ! -f JUCE/CMakeLists.txt ]; then
    echo "Initializing JUCE submodule..."
    git submodule update --init
fi

echo "Configuring..."
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

echo "Building (this may take a few minutes)..."
cmake --build build --config Release -j$(nproc)

echo ""
echo "=== Build complete! ==="
echo ""
echo "VST3 plugin:"
echo "  build/Molude_artefacts/Release/VST3/Molude.vst3"
echo ""
echo "Standalone app:"
echo "  build/Molude_artefacts/Release/Standalone/Molude"
echo ""
echo "To install the VST3:"
echo "  mkdir -p ~/.vst3"
echo "  cp -r build/Molude_artefacts/Release/VST3/Molude.vst3 ~/.vst3/"
echo ""
