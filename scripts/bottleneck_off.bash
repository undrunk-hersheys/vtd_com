#!/usr/bin/env bash
set -euo pipefail

[[ $EUID -ne 0 ]] && { echo "Run as root"; exit 1; }

DEV="${1:-tap1}"
IFB="${IFB_NAME:-ifb0}"

echo "[*] Disabling bottleneck on $DEV and cleaning $IFB"

# ingress qdisc 제거(필터 포함 정리)
tc qdisc del dev "$DEV" ingress 2>/dev/null || true

# IFB의 qdisc 제거 및 링크 삭제
tc qdisc del dev "$IFB" root 2>/dev/null || true
ip link set "$IFB" down 2>/dev/null || true
ip link del "$IFB" 2>/dev/null || true

echo "[+] Bottleneck OFF"
tc qdisc show dev "$DEV" || true
