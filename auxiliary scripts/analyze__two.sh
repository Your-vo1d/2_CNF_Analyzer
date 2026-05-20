#!/bin/bash

analyze_file() {
    local src="$1"
    local name="${src%.c}" 
    local log="result_${name}.log"
    local html="${name}.html"

    if [[ ! -f "$src" ]]; then
        echo "Файл $src не найден, пропускаем."
        return 1
    fi

    echo "Анализируем: $src"

    echo "bear -- gcc $src -o $name"
    bear -- gcc "$src" -o "$name"
    if [[ $? -ne 0 ]]; then
        echo "Ошибка: bear не смог скомпилировать $src"
        return 1
    fi

    echo "pvs-studio-analyzer analyze -f compile_commands.json -o $log"
    pvs-studio-analyzer analyze -f compile_commands.json -o "$log" --disableLicenseExpirationCheck
    if [[ $? -ne 0 ]]; then
        echo "Ошибка: pvs-studio-analyzer завершился с ошибкой для $src"
        return 1
    fi

    echo "plog-converter -a GA:1,2 -t html -o $html $log"
    plog-converter -a GA:1,2 -t html -o "$html" "$log"
    if [[ $? -eq 0 ]]; then
        echo "HTML-отчёт создан: $html"
    else
        echo "Ошибка при конвертации лога для $src"
        return 1
    fi


    echo ""
    return 0
}

# Анализируем два нужных файла
analyze_file "leak.c"
analyze_file "ok.c"

rm -f result_*.log compile_commands.json
echo "Работа завершена. Результаты: leak.html и ok.html"