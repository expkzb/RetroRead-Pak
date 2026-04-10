#!/bin/sh

resolve_path() {
    target="$1"
    if command -v readlink >/dev/null 2>&1; then
        resolved="$(readlink -f "$target" 2>/dev/null)"
        if [ -n "$resolved" ]; then
            printf '%s\n' "$resolved"
            return 0
        fi
    fi
    case "$target" in
        /*) printf '%s\n' "$target" ;;
        *) printf '%s/%s\n' "$(pwd)" "$target" ;;
    esac
}

SCRIPT_PATH="$(resolve_path "$0")"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$SCRIPT_PATH")" && pwd)
APP_ROOT="$SCRIPT_DIR"

export NEXTREADING_HOME="${NEXTREADING_HOME:-$APP_ROOT}"
export NEXTREADING_CACHE_PATH="${NEXTREADING_CACHE_PATH:-$APP_ROOT/BooksCache}"
export NEXTREADING_SAVES_PATH="${NEXTREADING_SAVES_PATH:-$APP_ROOT/Saves/RetroRead}"
export NEXTREADING_ASSETS_PATH="${NEXTREADING_ASSETS_PATH:-$APP_ROOT/assets}"
export NEXTREADING_SCREEN_WIDTH="${NEXTREADING_SCREEN_WIDTH:-1024}"
export NEXTREADING_SCREEN_HEIGHT="${NEXTREADING_SCREEN_HEIGHT:-768}"
export LD_LIBRARY_PATH="$APP_ROOT/lib:${LD_LIBRARY_PATH:-}"
export SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1
export SDL_GAMECONTROLLER_USE_BUTTON_LABELS=0
export NEXTREADING_INPUT_MODE="${NEXTREADING_INPUT_MODE:-joystick}"
# TrimUI Brick default gamepad mapping (can be overridden by env).
export NEXTREADING_BTN_A="${NEXTREADING_BTN_A:-1}"
export NEXTREADING_BTN_B="${NEXTREADING_BTN_B:-0}"
export NEXTREADING_BTN_X="${NEXTREADING_BTN_X:-3}"
export NEXTREADING_BTN_Y="${NEXTREADING_BTN_Y:-2}"
export NEXTREADING_BTN_L1="${NEXTREADING_BTN_L1:-4}"
export NEXTREADING_BTN_R1="${NEXTREADING_BTN_R1:-5}"
export NEXTREADING_BTN_SELECT="${NEXTREADING_BTN_SELECT:-6}"
export NEXTREADING_BTN_START="${NEXTREADING_BTN_START:-7}"

BOOK_CANDIDATES="
$APP_ROOT/Books
/mnt/SDCARD/Books
/mnt/SDCARD/Roms/Books
/mnt/SDCARD/ROMS/Books
"

if [ -n "${NEXTREADING_BOOKS_PATH:-}" ]; then
    BOOK_CANDIDATES="$NEXTREADING_BOOKS_PATH
$BOOK_CANDIDATES"
fi

SELECTED_BOOKS_PATH="$APP_ROOT/Books"
for candidate in $BOOK_CANDIDATES; do
    if [ -d "$candidate" ]; then
        if find "$candidate" -type f | grep -Eiq '\.epub$'; then
            SELECTED_BOOKS_PATH="$candidate"
            break
        fi
        SELECTED_BOOKS_PATH="$candidate"
    fi
done

export NEXTREADING_BOOKS_PATH="$SELECTED_BOOKS_PATH"

mkdir -p "$NEXTREADING_BOOKS_PATH" "$NEXTREADING_CACHE_PATH" "$NEXTREADING_SAVES_PATH"
echo "NEXTREADING_SCRIPT_PATH=$SCRIPT_PATH" >&2
echo "NEXTREADING_APP_ROOT=$APP_ROOT" >&2
echo "NEXTREADING_PWD=$(pwd)" >&2
echo "NEXTREADING_BOOKS_PATH=$NEXTREADING_BOOKS_PATH" >&2
BOOK_COUNT="$(find "$NEXTREADING_BOOKS_PATH" -type f | grep -Ei '\.epub$' | wc -l | tr -d '[:space:]')"
echo "NEXTREADING_BOOK_COUNT=$BOOK_COUNT" >&2
echo "NEXTREADING_INPUT_MODE=$NEXTREADING_INPUT_MODE" >&2
echo "NEXTREADING_BTN_MAP=A:$NEXTREADING_BTN_A B:$NEXTREADING_BTN_B X:$NEXTREADING_BTN_X Y:$NEXTREADING_BTN_Y L1:$NEXTREADING_BTN_L1 R1:$NEXTREADING_BTN_R1 SELECT:$NEXTREADING_BTN_SELECT START:$NEXTREADING_BTN_START" >&2

if [ -x "$APP_ROOT/nextreading" ]; then
    exec "$APP_ROOT/nextreading" "$@"
fi

echo "RetroRead binary not found: $APP_ROOT/nextreading" >&2
exit 1
