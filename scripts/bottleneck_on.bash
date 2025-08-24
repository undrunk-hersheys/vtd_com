#!/usr/bin/env bash
set -euo pipefail

[[ $EUID -ne 0 ]] && { echo "Run as root"; exit 1; }

DEV="${1:-tap1}"
IFB="${IFB_NAME:-ifb0}"
RATE="${RATE:-1mbit}"
BURST="${BURST:-32k}"
LATENCY="${LATENCY:-400ms}"

echo "[*] Enabling bottleneck on $DEV via $IFB (rate=$RATE, burst=$BURST, latency=$LATENCY)"

# 모듈/IFB 준비
modprobe ifb 2>/dev/null || true
ip link add "$IFB" type ifb 2>/dev/null || true
ip link set "$IFB" up

# ingress 훅 설치(여러 번 호출 안전)
tc qdisc add dev "$DEV" ingress 2>/dev/null || true

# 기존에 같은 redirect가 있으면 중복 추가 방지
if ! tc filter show dev "$DEV" ingress | grep -q "mirred egress redirect dev $IFB"; then
  tc filter add dev "$DEV" ingress matchall action mirred egress redirect dev "$IFB"
fi

# IFB에 병목(TBF) 설정/갱신
tc qdisc replace dev "$IFB" root tbf rate "$RATE" burst "$BURST" latency "$LATENCY"

echo "[+] Bottleneck ON"
tc qdisc show dev "$DEV" || true
tc qdisc show dev "$IFB" || true
