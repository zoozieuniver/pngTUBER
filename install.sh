#!/bin/bash

echo "------------------------------------------"
echo "   PNGTuber CLI - Autonomic Installer    "
echo "------------------------------------------"

# 1. ПЕРЕВІРКА ЗАЛЕЖНОСТЕЙ
echo "🔍 Searching for dependencies..."
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --noconfirm gcc git sdl2 sdl2_image
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update && sudo apt-get install -y g++ git libsdl2-dev libsdl2-image-dev
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel
fi

# 2. ПІДТЯГУВАННЯ ФАЙЛІВ ОНЛАЙН (Крок 1)
echo "🌐 Downloading source files from GitHub..."
REPO_URL="https://github.com/zoozieuniver/pngTUBER.git"
TEMP_DIR="$HOME/pngtuber_build_tmp"

# Видаляємо стару папку, якщо вона була, і клонуємо заново
rm -rf "$TEMP_DIR"
git clone "$REPO_URL" "$TEMP_DIR"
cd "$TEMP_DIR" || { echo "❌ Error: Could not download files"; exit 1; }

# 3. ВСТАНОВЛЕННЯ ПРОГРАМИ НА ОСНОВІ ФАЙЛІВ (Крок 2)
echo "⚙️ Compiling and installing..."
INSTALL_DIR="$HOME/.local/share/pngtuber"
BIN_DIR="$HOME/.local/bin"

mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"

# Компіляція (використовуємо файли, що щойно скачали)
g++ main.cpp -o pngtuber -lSDL2 -lSDL2_image -lpthread -ldl

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    # Переносимо бінарник та ресурси
    mv pngtuber "$BIN_DIR/"
    cp -r presets "$INSTALL_DIR/"
    cp -r assets "$INSTALL_DIR/"
    chmod +x "$BIN_DIR/pngtuber"
else
    echo "❌ Error: Compilation failed!"
    exit 1
fi

# Створення ярлика для меню
cat <<EOF > ~/.local/share/applications/pngtuber.desktop
[Desktop Entry]
Name=PNGTuber CLI
Exec=$BIN_DIR/pngtuber
Path=$INSTALL_DIR
Icon=$INSTALL_DIR/assets/icons/app_icon.png
Terminal=true
Type=Application
Categories=AudioVideo;
EOF

# 4. ВИДАЛЕННЯ ТОГО, ЩО КАЧАЛО (Крок 3)
echo "🧹 Cleaning up source files..."
cd "$HOME"
rm -rf "$TEMP_DIR"

echo "------------------------------------------"
echo "✨ DONE! PNGTuber installed to $BIN_DIR"
echo "Source files have been removed."
echo "------------------------------------------"