#!/bin/bash

# --- PATHS ---
INSTALL_DIR="$HOME/.local/share/pngtuber-cli"
REPO_DIR="$INSTALL_DIR/source"
BIN_DIR="$HOME/.local/bin"
LAUNCHER="$BIN_DIR/pngtuber"
REPO_URL="https://github.com/zoozieuniver/pngTUBER.git"

echo -e "\e[36m------------------------------------------"
echo "   PNGTuber CLI - Universal Installer    "
echo -e "------------------------------------------\e[0m"

# 1. DEPENDENCIES
echo "🛠️ Checking dependencies..."
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --noconfirm gcc git sdl2 sdl2_image
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update && sudo apt-get install -y g++ git libsdl2-dev libsdl2-image-dev
fi

# 2. SETUP DIRECTORIES & CLONE
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"

echo "🌐 Downloading source from GitHub..."
rm -rf "$REPO_DIR"
git clone "$REPO_URL" "$REPO_DIR"

# 3. INITIAL BUILD
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

# 4. CREATING SMART LAUNCHER (Updated with Enter to reboot)
echo "🚀 Creating launcher with auto-update and manual reboot..."
cat << 'EOF' > "$LAUNCHER"
#!/bin/bash
REPO_DIR="$HOME/.local/share/pngtuber-cli/source"
EXE_PATH="$HOME/.local/share/pngtuber-cli/PNGTuber"

show_spinner() {
    local pid=$1
    local delay=0.1
    local spinstr='|/-'
    while kill -0 "$pid" 2>/dev/null; do
        local temp=${spinstr#?}
        printf "\r [%c] Checking for updates... " "$spinstr"
        spinstr=$temp${spinstr%"$temp"}
        sleep $delay
    done
    printf "\r"
}

if ping -q -c 1 -W 1 google.com >/dev/null 2>&1; then
    cd "$REPO_DIR" || exit
    git fetch >/dev/null 2>&1 &
    show_spinner $!
    
    if [ "$(git rev-parse HEAD)" != "$(git rev-parse @{u})" ]; then
        echo -e "\e[33m📥 Update found! Downloading changes...\e[0m"
        git pull >/dev/null 2>&1
        
        # Safe removal of the old binary
        rm -f "$EXE_PATH"
        
        echo -e "\e[36m⚙️ Recompiling source code...\e[0m"
        g++ main.cpp -o "$EXE_PATH" -lSDL2 -lSDL2_image -lpthread -ldl
        
        echo -e "\e[32m✅ Update successfully installed!\e[0m"
        echo -e "\e[35m👉 Please press Enter to reboot application...\e[0m"
        read # Чекаємо на натискання Enter
        
        exec "$0" "$@" # Перезапуск скрипта
    fi
fi

$EXE_PATH
EOF
chmod +x "$LAUNCHER"