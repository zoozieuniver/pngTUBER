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

# 2. Встановлення залежностей (додано перевірку на Arch/CachyOS/Gentoo)
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --noconfirm gcc git sdl2 sdl2_image desktop-file-utils
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ git SDL2-devel SDL2_image-devel desktop-file-utils
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update && sudo apt-get install -y g++ git libsdl2-dev libsdl2-image-dev desktop-file-utils
elif command -v emerge >/dev/null 2>&1; then
    if command -v sudo >/dev/null 2>&1; then
        sudo emerge -n dev-vcs/git media-libs/libsdl2 media-libs/sdl2-image dev-util/desktop-file-utils
    else
        echo "🔑 Потрібні права root для встановлення залежностей (використовуємо su):"
        su -c "emerge -n dev-vcs/git media-libs/libsdl2 media-libs/sdl2-image dev-util/desktop-file-utils"
    fi
fi

# 3. Клонування/Оновлення сирців
if [ -f "main.cpp" ] && [ -f "config.cpp" ]; then
    echo "📦 Copying local source files to $REPO_DIR..."
    mkdir -p "$REPO_DIR"
    cp -rf ./* "$REPO_DIR/"
else
    if [ -d "$REPO_DIR/.git" ]; then
        cd "$REPO_DIR" && git pull
    else
        rm -rf "$REPO_DIR"
        git clone https://github.com/zoozieuniver/pngTUBER.git "$REPO_DIR"
    fi
fi

# 3.1 Клонування ImGui якщо його немає
if [ ! -d "$REPO_DIR/imgui" ]; then
    echo "📥 Cloning Dear ImGui..."
    git clone --depth 1 -b v1.90.9 https://github.com/ocornut/imgui.git "$REPO_DIR/imgui"
fi

# 4. Компіляція
cd "$REPO_DIR"
g++ main.cpp config.cpp audio.cpp avatar.cpp gui.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp \
    imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_sdlrenderer2.cpp \
    -o "$BINARY_ENGINE" -Iimgui -Iimgui/backends -I/usr/include/SDL2 -lSDL2 -lSDL2_image -lpthread -ldl

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

# Тиха перевірка оновлень в фоновому режимі, щоб не затримувати запуск
(
    if ping -q -c 1 -W 1 google.com >/dev/null 2>&1; then
        cd "$REPO_DIR" || exit
        git fetch >/dev/null 2>&1
        
        # Перевіряємо, чи є нові коміти у віддаленому репозиторії
        UPSTREAM_CHANGES=$(git log HEAD..@{u} --oneline 2>/dev/null)

        if [ -n "$UPSTREAM_CHANGES" ]; then
            if command -v notify-send >/dev/null 2>&1; then
                notify-send "PNGTuber" "📥 Знайдено оновлення! Встановлення у фоні..."
            fi
            echo "📥 New update found! Updating in background..."
            git pull >/dev/null 2>&1
            if [ ! -d "imgui" ]; then
                git clone --depth 1 -b v1.90.9 https://github.com/ocornut/imgui.git imgui >/dev/null 2>&1
            fi
            g++ main.cpp config.cpp audio.cpp avatar.cpp gui.cpp \
                imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp \
                imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_sdlrenderer2.cpp \
                -o "$EXE_PATH" -Iimgui -Iimgui/backends -I/usr/include/SDL2 -lSDL2 -lSDL2_image -lpthread -ldl >/dev/null 2>&1
            
            if command -v notify-send >/dev/null 2>&1; then
                notify-send "PNGTuber" "✅ Оновлено успішно! Перезапустіть програму, щоб застосувати зміни."
            fi
            echo "✅ Updated successfully! Please restart the app to apply changes."
        fi
    fi
) &

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
Name=PNGTuber
Exec=$LAUNCHER
Icon=$INSTALL_DIR/assets/icons/app_icon.png
Path=$INSTALL_DIR
Terminal=false
Categories=AudioVideo;Utility;
EOF

update-desktop-database "$REAL_HOME/.local/share/applications/" >/dev/null 2>&1
echo "✨ Done! Try running 'pngtuber' in your terminal (make sure ~/.local/bin is in your PATH)."