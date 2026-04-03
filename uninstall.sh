#!/bin/bash

INSTALL_DIR="$HOME/.local/share/pngtuber-cli"
BIN_DIR="$HOME/.local/bin"
DESKTOP_FILE="$HOME/.local/share/applications/pngtuber.desktop"

echo "------------------------------------------"
echo "   PNGTuber CLI - Complete Uninstall     "
echo "------------------------------------------"

# 1. Remove assets, source and binary
if [ -d "$INSTALL_DIR" ]; then
    echo "📂 Removing all files from $INSTALL_DIR..."
    rm -rf "$INSTALL_DIR"
    echo "✅ Done."
fi

# 2. Remove launcher
if [ -f "$BIN_DIR/pngtuber" ]; then
    echo "⚙️ Removing launcher from $BIN_DIR..."
    rm -f "$BIN_DIR/pngtuber"
    echo "✅ Done."
fi

# 3. Remove desktop shortcut
if [ -f "$DESKTOP_FILE" ]; then
    echo "🌟 Removing desktop shortcut..."
    rm -f "$DESKTOP_FILE"
    
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database ~/.local/share/applications/
    fi
    echo "✅ Done."
fi

echo "========================================="
echo "✨ System is clean!"
echo "========================================="