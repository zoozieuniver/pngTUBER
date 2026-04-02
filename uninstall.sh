#!/bin/bash

echo "🗑️ Starting PNGTuber uninstallation..."

INSTALL_DIR="$HOME/.local/share/pngtuber"
BIN_DIR="$HOME/.local/bin"
DESKTOP_FILE="$HOME/.local/share/applications/pngtuber.desktop"

# Видаляємо виконуваний файл
if [ -f "$BIN_DIR/pngtuber" ]; then
    rm "$BIN_DIR/pngtuber"
    echo "🧹 Removed executable from $BIN_DIR"
fi

# Видаляємо папку з ресурсами (картинками)
if [ -d "$INSTALL_DIR" ]; then
    rm -r "$INSTALL_DIR"
    echo "🧹 Removed assets from $INSTALL_DIR"
fi

# Видаляємо ярлик
if [ -f "$DESKTOP_FILE" ]; then
    rm "$DESKTOP_FILE"
    echo "🧹 Removed desktop shortcut"
fi

echo "========================================="
echo "✅ Uninstallation complete!"
echo "PNGTuber has been completely removed from your system."
echo "========================================="