#!/bin/bash

echo "------------------------------------------"
echo "   PNGTuber CLI - Universal Installer    "
echo "------------------------------------------"

# --- 1. SEARCH FOR DEPENDENCIES ---
echo "🔍 Searching for dependencies..."

# Перевірка та встановлення пакетів залежно від дистрибутива
if command -v pacman &> /dev/null; then
    sudo pacman -S --needed base-devel git sdl2 sdl2_image
elif command -v apt &> /dev/null; then
    sudo apt update && sudo apt install -y build-essential git libsdl2-dev libsdl2-image-dev
elif command -v dnf &> /dev/null; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel
else
    echo "⚠️ Unknown distribution. Please ensure git, g++, SDL2 and SDL2_image are installed."
fi

# --- 2. CLONING REPOSITORY ---
echo "📦 Downloading source code from GitHub..."
# Видаляємо стару папку, якщо вона була, щоб уникнути конфліктів
rm -rf pngTUBER
git clone https://github.com/zoozieuniver/pngTUBER.git
cd pngTUBER || exit

# --- 3. COMPILING ---
echo "⚙️ Compiling PNGTuber CLI..."
# Компілюємо програму з твого репозиторію
if g++ main.cpp -o PNGTuber -lSDL2 -lSDL2_image -lpthread -ldl; then
    echo "✅ Compilation successful!"
    chmod +x PNGTuber
else
    echo "❌ ERROR: Compilation failed. Please check the error messages above."
    exit 1
fi

# --- 4. SYSTEM INTEGRATION ---
echo "🚀 Creating system menu shortcut..."
mkdir -p ~/.local/share/applications
mkdir -p ~/.local/share/icons

# Копіюємо іконку в системну папку
cp assets/icons/app_icon.png ~/.local/share/icons/pngtuber_icon.png

# Створюємо .desktop файл
cat <<EOF > ~/.local/share/applications/pngtuber.desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=PNGTuber CLI
Comment=Simple PNGTuber for Linux
Exec=$(pwd)/PNGTuber
Icon=$HOME/.local/share/icons/pngtuber_icon.png
Terminal=true
Categories=Utility;Video;AudioVideo;
EOF

echo "------------------------------------------"
echo "✨ SUCCESS! PNGTuber CLI is installed. ✨"
echo "You can find it in your App Menu or run: ./PNGTuber"
echo "------------------------------------------"