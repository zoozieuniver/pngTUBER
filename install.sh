#!/bin/bash

echo "📦 Starting PNGTuber installation..."

# 1. Check if source files exist in the current directory
if [ ! -f "main.cpp" ] || [ ! -f "idle.png" ] || [ ! -f "talk.png" ] || [ ! -f "miniaudio.h" ]; then
    echo "❌ Error: Required files (main.cpp, miniaudio.h, idle.png, talk.png) not found!"
    echo "Please run this script from the folder containing these files."
    exit 1
fi

# 2. Smart dependency installation
echo "🛠️ Checking package manager..."

if command -v dnf >/dev/null 2>&1; then
    echo "Fedora/Nobara detected. Installing via dnf..."
    sudo dnf install -y gcc-c++ SDL2-devel SDL2_image-devel
elif command -v apt-get >/dev/null 2>&1; then
    echo "Ubuntu/Debian/Mint detected. Installing via apt..."
    sudo apt-get update
    sudo apt-get install -y g++ libsdl2-dev libsdl2-image-dev
elif command -v pacman >/dev/null 2>&1; then
    echo "Arch/Manjaro detected. Installing via pacman..."
    sudo pacman -S --noconfirm gcc sdl2 sdl2_image
else
    echo "❌ Unknown Linux distribution. Please install C++ compiler, SDL2 and SDL2_image manually."
    exit 1
fi

# 4. Set up directories (User-level installation, no root needed here)
echo "📂 Setting up directories..."
INSTALL_DIR="$HOME/.local/share/pngtuber"
BIN_DIR="$HOME/.local/bin"

mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"

# 5. Copy files to their new home
echo "🚚 Moving files..."
cp pngtuber "$BIN_DIR/"
cp idle.png talk.png "$INSTALL_DIR/"

# Make sure the binary is executable
chmod +x "$BIN_DIR/pngtuber"

# 6. Create Desktop Entry (Shortcut)
echo "🌟 Creating desktop shortcut..."
DESKTOP_FILE="$HOME/.local/share/applications/pngtuber.desktop"

cat <<EOF > "$DESKTOP_FILE"
[Desktop Entry]
Name=PNGTuber
Comment=Control panel and avatar for PNGTuber
Exec=$BIN_DIR/pngtuber
Path=$INSTALL_DIR
Terminal=true
Type=Application
Categories=AudioVideo;
EOF

chmod +x "$DESKTOP_FILE"

echo "========================================="
echo "✅ Installation complete!"
echo "You can now find 'PNGTuber' in your application menu."
echo "Note: The terminal will open automatically to show the control menu."
echo "========================================="