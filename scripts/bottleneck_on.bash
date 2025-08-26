#!/usr/bin/env bash
set -euo pipefail

# ---- Config ----
IF_EGRESS="${1:-tap1}"
RATE="${RATE:-600kbit}"
BURST="${BURST:-16k}"
LATENCY="${LATENCY:-50ms}"
DELAY="${DELAY:-2ms}"
JITTER="${JITTER:-0.5ms}"

echo "[*] Cleaning existing qdiscs on ${IF_EGRESS}"
sudo tc qdisc del dev "${IF_EGRESS}" root 2>/dev/null || true

echo "[*] Applying bottleneck on ${IF_EGRESS} (TBF ${RATE}, burst ${BURST}, latency ${LATENCY})"
sudo tc qdisc replace dev "${IF_EGRESS}" root handle 1: tbf rate "${RATE}" burst "${BURST}" latency "${LATENCY}"

echo "[*] Adding netem under TBF (delay ${DELAY} ± ${JITTER})"
sudo tc qdisc replace dev "${IF_EGRESS}" parent 1:1 handle 10: netem delay "${DELAY}" "${JITTER}"

# Optional: sender 쪽만 pfifo로 통일 (tap1 root는 건드리지 않음)
if ip link show tap0 &>/dev/null; then
  sudo tc qdisc replace dev tap0 root pfifo limit 1000 || true
fi

echo
echo "[*] qdisc status for ${IF_EGRESS}:"
sudo tc -s qdisc show dev "${IF_EGRESS}"
