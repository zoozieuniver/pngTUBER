#!/bin/bash

echo "------------------------------------------"
echo "   PNGTuber CLI - Universal Installer    "
echo "------------------------------------------"

# --- 1. ПЕРЕВІРКА ЗАЛЕЖНОСТЕЙ ---
echo "🔍 Detecting package manager..."

if command -v apt &> /dev/null; then
    sudo apt update && sudo apt install -y build-essential libsdl2-dev libsdl2-image-dev
elif command -v dnf &> /dev/null; then
    sudo dnf install -y gcc-c++ SDL2-devel SDL2_image-devel
elif command -v pacman &> /dev/null; then
    sudo pacman -S --needed base-devel sdl2 sdl2_image
elif command -v zypper &> /dev/null; then
    sudo zypper install -y gcc-c++ libSDL2-devel libSDL2_image-devel
fi

# --- 2. СТВОРЕННЯ СТРУКТУРИ ПАПОК ---
echo "📂 Creating directory structure..."
mkdir -p assets/errors assets/icons presets/default
mkdir -p ~/.local/share/applications ~/.local/share/icons

# --- 3. РОЗПАКУВАННЯ РЕСУРСІВ (Твої Base64 дані) ---
echo "📦 Extracting embedded assets..."

# Головний код
echo "$(cat <<EOF
$(cat b64_main.txt)
EOF
)" | base64 -d > main.cpp

# Бібліотека звуку
echo "$(cat <<EOF
$(cat b64_miniaudio.txt)
EOF
)" | base64 -d > miniaudio.h

# Картинки персонажа
echo "$(cat <<EOF
$(cat b64_idle.txt)
EOF
)" | base64 -d > presets/default/idle.png

echo "$(cat <<EOF
$(cat b64_talk.txt)
EOF
)" | base64 -d > presets/default/talk.png

# Fallback картинки (Помилки)
echo "$(cat <<EOF
$(cat b64_err_idle.txt)
EOF
)" | base64 -d > assets/errors/error_idle.png

echo "$(cat <<EOF
$(cat b64_err_talk.txt)
EOF
)" | base64 -d > assets/errors/error_talk.png

# Іконка програми (я використав idle як заглушку, якщо b64_icon не було окремо)
cp presets/default/idle.png assets/icons/app_icon.png
cp assets/icons/app_icon.png ~/.local/share/icons/pngtuber_icon.png

# --- 4. КОМПІЛЯЦІЯ ---
echo "⚙️ Compiling PNGTuber CLI..."
g++ main.cpp -o PNGTuber -lSDL2 -lSDL2_image -lpthread -ldl
chmod +x PNGTuber

# --- 5. КОНФІГУРАЦІЯ ---
if [ ! -f config.txt ]; then
    echo "threshold=0.05" > config.txt
    echo "mic_name=" >> config.txt
    echo "shake_intensity=5" >> config.txt
    echo "preset=default" >> config.txt
fi

# --- 6. СИСТЕМНА ІНТЕГРАЦІЯ ---
echo "🚀 Creating system menu shortcut..."
cat <<EOF > ~/.local/share/applications/pngtuber.desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=PNGTuber CLI
Comment=Simple PNGTuber with mic input and shake effects
Exec=$(pwd)/PNGTuber
Icon=$HOME/.local/share/icons/pngtuber_icon.png
Terminal=true
Categories=Utility;Video;
EOF

# --- 7. ПРИБИРАННЯ ---
echo "🧹 Cleaning up source files..."
rm main.cpp miniaudio.h

echo "------------------------------------------"
echo "✨ SUCCESS! PNGTuber CLI is installed. ✨"
echo "Run it via: ./PNGTuber"
echo "------------------------------------------"