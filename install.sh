#!/bin/bash

# --- ПАРАМЕТРИ ---
REPO_URL="https://github.com/zoozieuniver/pngTUBER.git"
INSTALL_DIR="$HOME/.local/share/pngtuber-cli"
BIN_DIR="$HOME/.local/bin"
TEMP_DIR="$HOME/pngtuber_tmp_build"

echo "------------------------------------------"
echo "   PNGTuber CLI - Full Setup & Clean     "
echo "------------------------------------------"

# 1. ЗАЛЕЖНОСТІ (Arch/CachyOS)
echo "🛠️ Installing dependencies..."
sudo pacman -S --needed base-devel git sdl2 sdl2_image --noconfirm

# 2. КЛОНУВАННЯ (Крок 1: Підтягуємо файли)
echo "🌐 Cloning from GitHub..."
rm -rf "$TEMP_DIR"
git clone "$REPO_URL" "$TEMP_DIR"
cd "$TEMP_DIR" || exit

# 3. ВСТАНОВЛЕННЯ (Крок 2: Будуємо структуру)
echo "📂 Creating permanent folders..."
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"

echo "⚙️ Compiling..."
# Компілюємо прямо в папку призначення
g++ main.cpp -o "$INSTALL_DIR/PNGTuber" -lSDL2 -lSDL2_image -lpthread -ldl

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    # Копіюємо всі ресурси в папку "дому"
    cp -r presets "$INSTALL_DIR/"
    cp -r assets "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/PNGTuber"
    
    # Створюємо посилання в bin, щоб можна було запускати командою 'pngtuber'
    ln -sf "$INSTALL_DIR/PNGTuber" "$BIN_DIR/pngtuber"
else
    echo "❌ ERROR: Compilation failed!"
    exit 1
fi

# 4. СТВОРЕННЯ ЯРЛИКА
echo "🌟 Creating Desktop shortcut..."
cat <<EOF > ~/.local/share/applications/pngtuber.desktop
[Desktop Entry]
Name=PNGTuber CLI
Exec=$INSTALL_DIR/PNGTuber
Path=$INSTALL_DIR
Icon=$INSTALL_DIR/assets/icons/app_icon.png
Terminal=true
Type=Application
Categories=Utility;AudioVideo;
EOF

# --- 5. ОЧИЩЕННЯ (Крок 3: Видаляємо все, що качали) ---
echo "🧹 Cleaning up temporary files..."

# Повертаємось у папку, де лежить сам інсталятор
cd "$(dirname "$0")"

# Видаляємо тимчасову папку, яку створив git clone
if [ -d "$TEMP_DIR" ]; then
    rm -rf "$TEMP_DIR"
    echo "✅ Temporary folder $TEMP_DIR removed."
else
    # Якщо TEMP_DIR не спрацював як змінна, видаляємо за назвою
    rm -rf pngtuber_tmp_build
    echo "✅ Source files cleaned up."
fi

# (Опціонально) Видалити сам інсталятор після завершення
# rm -- "$0" 

echo "------------------------------------------"
echo "✨ ALL DONE! System is clean."
echo "------------------------------------------"