#!/bin/bash

echo "------------------------------------------"
echo "   PNGTuber CLI - Universal Installer    "
echo "------------------------------------------"

# --- 1. ПЕРЕВІРКА ЗАЛЕЖНОСТЕЙ (КРОС-ДИСТРИБУТИВ) ---
echo "🔍 Detecting package manager..."

if command -v apt &> /dev/null; then
    echo "📦 Debian/Ubuntu/Mint detected. Installing via apt..."
    sudo apt update && sudo apt install -y build-essential libsdl2-dev libsdl2-image-dev
elif command -v dnf &> /dev/null; then
    echo "📦 Fedora/RHEL detected. Installing via dnf..."
    sudo dnf install -y gcc-c++ SDL2-devel SDL2_image-devel
elif command -v pacman &> /dev/null; then
    echo "📦 Arch/Manjaro detected. Installing via pacman..."
    sudo pacman -S --needed base-devel sdl2 sdl2_image
elif command -v zypper &> /dev/null; then
    echo "📦 OpenSUSE detected. Installing via zypper..."
    sudo zypper install -y gcc-c++ libSDL2-devel libSDL2_image-devel
else
    echo "⚠️ Unknown distribution. Please install SDL2 and SDL2_image manually."
fi

# --- 2. СТВОРЕННЯ СТРУКТУРИ ПАПОК ---
echo "📂 Creating directory structure..."
mkdir -p assets/errors
mkdir -p assets/icons
mkdir -p presets/default
mkdir -p ~/.local/share/applications
mkdir -p ~/.local/share/icons

# --- 3. РОЗПАКУВАННЯ РЕСУРСІВ (BASE64) ---
echo "📦 Extracting embedded assets..."

# Заміни ці плейсхолдери на свої реальні Base64 рядки
echo "BASE64_CODE_HERE" | base64 -d > main.cpp
echo "BASE64_AUDIO_H_HERE" | base64 -d > miniaudio.h

# Розпакування картинок та іконки
echo "BASE64_IDLE_PNG" | base64 -d > presets/default/idle.png
echo "BASE64_TALK_PNG" | base64 -d > presets/default/talk.png
echo "BASE64_ERR_IDLE" | base64 -d > assets/errors/error_idle.png
echo "BASE64_ERR_TALK" | base64 -d > assets/errors/error_talk.png
echo "BASE64_ICON_PNG" | base64 -d > assets/icons/app_icon.png

# Реєстрація іконки в системі
cp assets/icons/app_icon.png ~/.local/share/icons/pngtuber_icon.png

# --- 4. КОМПІЛЯЦІЯ ---
echo "⚙️ Compiling PNGTuber CLI..."
# Компілюємо з підтримкою потоків та динамічних бібліотек
g++ main.cpp -o PNGTuber -lSDL2 -lSDL2_image -lpthread -ldl
chmod +x PNGTuber

# --- 5. КОНФІГУРАЦІЯ ---
# Створюємо дефолтний конфіг, якщо його ще немає
if [ ! -f config.txt ]; then
    echo "preset=default" > config.txt
    echo "threshold=100" >> config.txt
    echo "shake_intensity=5" >> config.txt
fi

# --- 6. СИСТЕМНА ІНТЕГРАЦІЯ (DESKTOP FILE) ---
echo "🚀 Creating system menu shortcut..."
cat <<EOF > ~/.local/share/applications/pngtuber.desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=PNGTuber CLI
Comment=Simple PNGTuber with shake and fallback effects
Exec=$(pwd)/PNGTuber
Icon=$HOME/.local/share/icons/pngtuber_icon.png
Terminal=true
Categories=Utility;Video;AudioVideo;
EOF

# --- 7. ПРИБИРАННЯ ---
echo "🧹 Cleaning up source files..."
rm main.cpp miniaudio.h

echo "------------------------------------------"
echo "✨ SUCCESS! PNGTuber CLI is installed. ✨"
echo "You can find it in your app menu or run ./PNGTuber"
echo "------------------------------------------"