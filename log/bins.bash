#!/usr/bin/env bash
set -euo pipefail

# 구간 크기 (기본 10). 필요 시 BIN=20 ./script.sh 형태로 덮어쓰기 가능
BIN="${BIN:-1000}"

# 기본 입력 목록: pol_pcp3, pol_pcp5, nopol_pcp3, nopol_pcp5
# 인자를 주면 해당 인자만 처리 (예: ./script.sh ./log/pol_pcp3.csv)
if [ "$#" -gt 0 ]; then
  INPUTS=("$@")
else
  INPUTS=(pol_pcp3 pol_pcp5 nopol_pcp3 nopol_pcp5)
fi

process_one() {
  local in="$1"

  # 확장자 유연 처리: 존재하면 그대로, 없으면 .csv 시도
  if [ -f "$in" ]; then
    :
  elif [ -f "${in}.csv" ]; then
    in="${in}.csv"
  else
    echo "입력 파일을 찾을 수 없음: $1" >&2
    return 0
  fi

  local dir base ext out
  dir="$(dirname -- "$in")"
  base="$(basename -- "$in")"
  if [[ "$base" == *.* ]]; then
    ext=".${base##*.}"
    out="${dir}/bins_${base%.*}${ext}"
  else
    ext=""
    out="${dir}/bins_${base}${ext:-.csv}"
  fi

  # 집계 맵
  declare -A sum_delay=()
  declare -A sum_jitter=()
  declare -A count=()

  # 입력 파싱 및 집계
  # 라인 예시: [VLAN] PCP=5 VID=200,33334,55555,14,1681,206
  while IFS= read -r line; do
    [[ "$line" == *"VID="* ]] || continue
    payload="${line#*VID=}"
    IFS=',' read -r vid src dst seq delay jitter <<< "$payload"

    [[ "$seq" =~ ^[0-9]+$ ]] || continue
    [[ "$delay" =~ ^[0-9]+$ ]] || continue
    [[ "$jitter" =~ ^[0-9]+$ ]] || continue

    # 1~BIN → bin=0, 11~20 → bin=1 ...
    bin=$(( (seq - 1) / BIN ))
    key_start=$(( bin * BIN + 1 ))
    key_end=$(( (bin + 1) * BIN ))
    key="${key_start}-${key_end}"

    sum_delay["$key"]=$(( ${sum_delay["$key"]:-0} + delay ))
    sum_jitter["$key"]=$(( ${sum_jitter["$key"]:-0} + jitter ))
    count["$key"]=$(( ${count["$key"]:-0} + 1 ))
  done < "$in"

  # 출력 파일 작성
  # 헤더: bin,delay_avg,jitter_avg,n
  {
    echo "bin,delay_avg,jitter_avg,n"

    # 키 정렬(구간 시작값 기준 오름차순)
    # key 형태 "S-E"에서 S만 뽑아 정렬
    mapfile -t keys < <(
      for k in "${!count[@]}"; do
        echo "$k"
      done | awk -F- '{print $1","$0}' | sort -n -t, -k1,1 | cut -d, -f2
    )

    for k in "${keys[@]}"; do
      n=${count[$k]}
      sd=${sum_delay[$k]}
      sj=${sum_jitter[$k]}
      # 평균은 소수점 1자리
      avg_d=$(awk -v s="$sd" -v n="$n" 'BEGIN{ if(n>0) printf("%.1f", s/n); else print "0.0" }')
      avg_j=$(awk -v s="$sj" -v n="$n" 'BEGIN{ if(n>0) printf("%.1f", s/n); else print "0.0" }')
      echo "${k},${avg_d},${avg_j},${n}"
    done
  } > "$out"

  echo "완료: ${in} → ${out}"
}

for f in "${INPUTS[@]}"; do
  process_one "$f"
done
