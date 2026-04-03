#!/bin/bash

# --- НАЛАШТУВАННЯ ШЛЯХІВ ---
# Використовуємо прямий шлях до домашньої папки для надійності
REAL_HOME=$(eval echo "~$USER")
INSTALL_DIR="$REAL_HOME/.local/share/pngtuber-cli"
REPO_DIR="$INSTALL_DIR/source"
BIN_DIR="$REAL_HOME/.local/bin"
LAUNCHER="$BIN_DIR/pngtuber"
DESKTOP_PATH="$REAL_HOME/.local/share/applications/pngtuber.desktop"
REPO_URL="https://github.com/zoozieuniver/pngTUBER.git"

echo -e "\e[36m------------------------------------------"
echo "   PNGTuber CLI - Universal Installer    "
echo -e "------------------------------------------\e[0m"

# 1. ПЕРЕВІРКА ПАПОК
# Створюємо необхідні директорії, якщо їх ще немає
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$REAL_HOME/.local/share/applications"

# 2. ВСТАНОВЛЕННЯ ЗАЛЕЖНОСТЕЙ
echo "🛠️ Checking dependencies..."
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --noconfirm gcc git sdl2 sdl2_image desktop-file-utils
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel desktop-file-utils
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update && sudo apt-get install -y g++ git libsdl2-dev libsdl2-image-dev desktop-file-utils
fi

# 3. ЗАВАНТАЖЕННЯ ТА КОМПІЛЯЦІЯ
echo "🌐 Downloading source from GitHub..."
rm -rf "$REPO_DIR"
git clone "$REPO_URL" "$REPO_DIR"

cd "$REPO_DIR" || exit
echo "⚙️ Compiling program..."
g++ main.cpp -o "$INSTALL_DIR/PNGTuber" -lSDL2 -lSDL2_image -lpthread -ldl

if [ $? -eq 0 ]; then
    echo -e "\e[32m✅ Compilation successful!\e[0m"
    cp -r presets "$INSTALL_DIR/" 2>/dev/null
    cp -r assets "$INSTALL_DIR/" 2>/dev/null
else
    echo -e "\e[31m❌ Compilation failed!\e[0m"
    exit 1
fi

# 4. СТВОРЕННЯ РОЗУМНОГО ЛАУНЧЕРА
echo "🚀 Creating launcher with auto-update..."
cat << EOF > "$LAUNCHER"
#!/bin/bash
REPO_DIR="$REPO_DIR"
EXE_PATH="$INSTALL_DIR/PNGTuber"

show_spinner() {
    local pid=\$1
    local delay=0.1
    local spinstr='|/-'
    while kill -0 "\$pid" 2>/dev/null; do
        local temp=\${spinstr#?}
        printf "\r [%c] Checking for updates... " "\$spinstr"
        spinstr=\$temp\${spinstr%"\$temp"}
        sleep \$delay
    done
    printf "\r"
}

if ping -q -c 1 -W 1 google.com >/dev/null 2>&1; then
    cd "\$REPO_DIR" || exit
    git fetch >/dev/null 2>&1 &
    show_spinner \$!
    
    if [ "\$(git rev-parse HEAD)" != "\$(git rev-parse @{u})" ]; then
        echo -e "\e[33m📥 Update found! Downloading changes...\e[0m"
        git pull >/dev/null 2>&1
        rm -f "\$EXE_PATH"
        echo -e "\e[36m⚙️ Recompiling source code...\e[0m"
        g++ main.cpp -o "\$EXE_PATH" -lSDL2 -lSDL2_image -lpthread -ldl
        echo -e "\e[32m✅ Update successfully installed!\e[0m"
        echo -e "\e[35m👉 Please press Enter to reboot application...\e[0m"
        read
        exec "\$0" "\$@"
    fi
fi

\$EXE_PATH
EOF
chmod +x "$LAUNCHER"

# 5. РЕЄСТРАЦІЯ ЯРЛИКА
echo "🌟 Registering desktop shortcut..."
cat <<EOF > "$DESKTOP_PATH"
[Desktop Entry]
Version=1.0
Type=Application
Name=PNGTuber CLI
Comment=Simple PNGTuber tool
Exec=$LAUNCHER
Icon=$INSTALL_DIR/assets/icons/app_icon.png
Path=$INSTALL_DIR
Terminal=true
Categories=AudioVideo;Utility;
EOF

chmod +x "$DESKTOP_PATH"

# 6. ВАЛІДАЦІЯ ТА ОНОВЛЕННЯ КЕШУ
if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$DESKTOP_PATH"
fi

update-desktop-database "$REAL_HOME/.local/share/applications/" >/dev/null 2>&1

# Специфічне оновлення для KDE (Nobara)
if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 >/dev/null 2>&1
elif command -v kbuildsycoca5 >/dev/null 2>&1; then
    kbuildsycoca5 >/dev/null 2>&1
fi

echo -e "\e[32m========================================="
echo "✅ Installation complete!"
echo "Check your app menu for 'PNGTuber CLI'."
echo -e "=========================================\e[0m"