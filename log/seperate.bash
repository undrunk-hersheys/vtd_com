#!/usr/bin/env bash

process_file() {
    local input="$1"
    local prefix="$2"

    # 기존 파일 삭제
    rm -f "./log/${prefix}_pcp"*.csv

    while IFS= read -r line; do
        if [[ $line =~ PCP=([0-9]+) ]]; then
            pcp="${BASH_REMATCH[1]}"
        else
            continue
        fi

        out="./log/${prefix}_pcp${pcp}.csv"
        echo "$line" >> "$out"
    done < "$input"
}

# nopol 처리
process_file "./log/nopol.csv" "nopol"

# pol 처리
process_file "./log/pol.csv" "pol"
