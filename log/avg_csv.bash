#!/usr/bin/env bash
set -euo pipefail

# 입력 파일 세트: 미지정 시 pol/nopol 모두 포함
FILES=("$@")
if [ ${#FILES[@]} -eq 0 ]; then
    FILES=(./log/pol_pcp*.csv ./log/nopol_pcp*.csv)
fi

# PCP별 파일 매핑
declare -A POLFILES
declare -A NOPOLFILES
declare -A PCPSEEN

for f in "${FILES[@]}"; do
    [ -f "$f" ] || continue
    if [[ $f =~ pcp([0-9]+)\.csv$ ]]; then
        p="${BASH_REMATCH[1]}"
        PCPSEEN["$p"]=1
        if [[ "$f" == *"/nopol_pcp"* ]]; then
            NOPOLFILES["$p"]="$f"
        else
            POLFILES["$p"]="$f"
        fi
    fi
done

# 통계 계산 함수: delay, jitter를 한 번에 계산
stats() {
    local file="$1"
    awk '
        {
            line=$0
            sub(/^.*VID=/, "", line)
            n = split(line, a, ",")
            if (n>=6) {
                if (a[5] ~ /^[0-9]+$/) {
                    d=a[5]+0
                    dsum+=d; if(d>dmax) dmax=d; dn++
                }
                if (a[6] ~ /^[0-9]+$/) {
                    j=a[6]+0
                    jsum+=j; if(j>jmax) jmax=j; jn++
                }
            }
        }
        END {
            if (dn)
                printf("delay avg=%.1f max=%d (n=%d)\n", dsum/dn, dmax, dn);
            else
                print "delay: no data";
            if (jn)
                printf("jitter avg=%.1f max=%d (n=%d)\n", jsum/jn, jmax, jn);
            else
                print "jitter: no data";
        }
    ' "$file"
}

# PCP 정렬 목록 생성
mapfile -t PCP_LIST < <(printf "%s\n" "${!PCPSEEN[@]}" | sort -n)

if [ ${#PCP_LIST[@]} -eq 0 ]; then
    echo "매칭되는 파일이 없습니다."
    exit 0
fi

# 1) nopol 묶음 전체
echo "==== nopol ===="
for p in "${PCP_LIST[@]}"; do
    nopf="${NOPOLFILES[$p]:-}"
    if [ -n "${nopf}" ] && [ -f "$nopf" ]; then
        echo " PCP $p - (${nopf}) "
        stats "$nopf"
    else
        echo "no file"
    fi
    echo
done

# 2) pol 묶음 전체
echo "==== pol ===="
for p in "${PCP_LIST[@]}"; do
    polf="${POLFILES[$p]:-}"
    if [ -n "${polf}" ] && [ -f "$polf" ]; then
        echo " PCP $p - (${polf})"
        stats "$polf"
    else
        echo "no file"
    fi
    echo
done
