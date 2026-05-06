#!/usr/bin/env bash
set -uo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build/release"
BIN="$BUILD_DIR/TreeProgram"
SUPP="$BUILD_DIR/qt5.supp"
EXAMPLES="$PROJECT_DIR/Примеры JSON"

FOLDERS=(
    "Добавление листьев"
    "Вставка в середину"
    "Освобождение корня"
    "Освобождение среднего"
    "Освобождение листьев"
    "С функциями"
)

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

qmake "$PROJECT_DIR/Course_work.pro" CONFIG+=release
make -j"$(nproc)" 2>&1 || exit 1

cd "$PROJECT_DIR"

for folder in "${FOLDERS[@]}"; do
    for variant in ok leak; do
        json="$EXAMPLES/$folder/$variant.json"
        [[ -f "$json" ]] || continue

        echo "=== $folder / $variant ==="

        "$BIN" "$json"

        echo "--- valgrind ---"
        valgrind --leak-check=full --show-leak-kinds=all \
            --track-origins=yes \
            --suppressions="$SUPP" \
            "$BIN" "$json"

        echo ""
    done
done
