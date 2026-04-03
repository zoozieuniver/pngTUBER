#!/bin/bash

# Шляхи (використовуємо абсолютні для стабільності)
REAL_HOME=$(eval echo "~$USER")
INSTALL_DIR="$REAL_HOME/.local/share/pngtuber-cli"
REPO_DIR="$INSTALL_DIR/source"
BIN_DIR="$REAL_HOME/.local/bin"
LAUNCHER="$BIN_DIR/pngtuber"
BINARY_ENGINE="$INSTALL_DIR/pngtuber-bin" # Нова назва для бінарника
DESKTOP_PATH="$REAL_HOME/.local/share/applications/pngtuber.desktop"

echo "🚀 Starting Clean Installation..."

# 1. Створення папок
mkdir -p "$INSTALL_DIR" "$BIN_DIR" "$REAL_HOME/.local/share/applications"

# 2. Встановлення залежностей (додано перевірку на Arch/CachyOS)
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --noconfirm gcc git sdl2 sdl2_image desktop-file-utils
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel desktop-file-utils
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update && sudo apt-get install -y g++ git libsdl2-dev libsdl2-image-dev desktop-file-utils
fi

# 3. Клонування/Оновлення сирців
if [ -d "$REPO_DIR/.git" ]; then
    cd "$REPO_DIR" && git pull
else
    rm -rf "$REPO_DIR"
    git clone https://github.com/zoozieuniver/pngTUBER.git "$REPO_DIR"
fi

# 4. Компіляція
cd "$REPO_DIR"
g++ main.cpp -o "$BINARY_ENGINE" -lSDL2 -lSDL2_image -lpthread -ldl

if [ $? -eq 0 ]; then
    echo "✅ Engine compiled: $BINARY_ENGINE"
    # Копіюємо ресурси та чистимо тимчасові файли (якщо g++ створив .o файли)
    cp -r presets assets "$INSTALL_DIR/" 2>/dev/null
    find . -maxdepth 1 -type f -name "*.o" -delete 
else
    echo "❌ Compilation failed!" && exit 1
fi

# 5. Створення лаунчера (використовуємо 'EOF' в лапках, щоб Bash нічого не підміняв завчасно)
cat << 'EOF' > "$LAUNCHER"
#!/bin/bash
INSTALL_DIR="$HOME/.local/share/pngtuber-cli"
REPO_DIR="$INSTALL_DIR/source"
EXE_PATH="$INSTALL_DIR/pngtuber-bin"

# Тиха перевірка оновлень
if ping -q -c 1 -W 1 google.com >/dev/null 2>&1; then
    cd "$REPO_DIR" || exit
    git fetch >/dev/null 2>&1
    
    LOCAL=$(git rev-parse HEAD)
    REMOTE=$(git rev-parse @{u})

    if [ "$LOCAL" != "$REMOTE" ]; then
        echo "📥 New update found! Updating..."
        git pull >/dev/null 2>&1
        g++ main.cpp -o "$EXE_PATH" -lSDL2 -lSDL2_image -lpthread -ldl
        echo "✅ Updated successfully!"
    fi
fi

# Запуск двигуна
if [ -f "$EXE_PATH" ]; then
    exec "$EXE_PATH" "$@"
else
    echo "Error: Binary not found at $EXE_PATH"
    exit 1
fi
EOF

chmod +x "$LAUNCHER" "$BINARY_ENGINE"

# 6. Ярлик та оновлення БД
cat <<EOF > "$DESKTOP_PATH"
[Desktop Entry]
Version=1.0
Type=Application
Name=PNGTuber CLI
Exec=$LAUNCHER
Icon=$INSTALL_DIR/assets/icons/app_icon.png
Path=$INSTALL_DIR
Terminal=true
Categories=AudioVideo;Utility;
EOF

update-desktop-database "$REAL_HOME/.local/share/applications/" >/dev/null 2>&1
echo "✨ Done! Try running 'pngtuber' in your terminal."