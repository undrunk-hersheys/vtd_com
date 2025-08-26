#!/usr/bin/env bash
set -euo pipefail

IF_EGRESS="${1:-tap1}"

echo "[*] Removing bottleneck qdiscs from ${IF_EGRESS}"
sudo tc qdisc del dev "${IF_EGRESS}" root 2>/dev/null || true
sudo tc qdisc del dev "${IF_EGRESS}" ingress 2>/dev/null || true

# Optional: sender 쪽은 pfifo로 정리
if ip link show tap0 &>/dev/null; then
  sudo tc qdisc replace dev tap0 root pfifo limit 1000 || true
fi

echo
echo "[*] qdisc status after cleanup for ${IF_EGRESS}:"
sudo tc -s qdisc show dev "${IF_EGRESS}"
