#!/usr/bin/env bash
set -euo pipefail

# ===== Settings =====
IF="${1:-tap1}"

# Stream A: VLAN 100, PCP 3, UDP dst 44444 → 100 kbit policing
RATE_A="${RATE_A:-50000kbit}"
BURST_A="${BURST_A:-2000}"   # bytes
DSTPORT_A="${DSTPORT_A:-44444}"
VID_A="${VID_A:-100}"
PCP_A="${PCP_A:-3}"

# Stream B: VLAN 200, PCP 5, UDP dst 55555 → 50 kbit policing
RATE_B="${RATE_B:-10000kbit}"
BURST_B="${BURST_B:-2000}"
DSTPORT_B="${DSTPORT_B:-55555}"
VID_B="${VID_B:-200}"
PCP_B="${PCP_B:-5}"

# Other traffics on drop (true/false)
DROP_OTHERS="${DROP_OTHERS:-true}"

echo "[*] Device: $IF"

# clsact attached (ingress/egress hook)
echo "[*] attach clsact (idempotent)"
sudo tc qdisc add dev "$IF" clsact 2>/dev/null || true

# delete previous filter
echo "[*] clear existing filters (ingress/egress)"
sudo tc filter del dev "$IF" ingress 2>/dev/null || true
sudo tc filter del dev "$IF" egress  2>/dev/null || true

# ───────────────────────────────────────────────────────────────────
# EGRESS 폴리싱: 초과 시 즉시 drop
# 주의: egress에 붙여야 TAP에서 사용자 프로세스 read() 방향 트래픽이 매칭됨
# mtu를 명시하여 토큰버킷 계산을 예측 가능하게 함
# ───────────────────────────────────────────────────────────────────

echo "[*] add EGRESS police for Stream A (VID=$VID_A, PCP=$PCP_A, UDP dport=$DSTPORT_A)"
sudo tc filter add dev "$IF" egress pref 100 protocol 802.1Q \
  flower \
    vlan_id "$VID_A" \
    vlan_prio "$PCP_A" \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port "$DSTPORT_A" \
  action police rate "$RATE_A" burst "$BURST_A" mtu 1500 conform-exceed drop

echo "[*] add EGRESS police for Stream B (VID=$VID_B, PCP=$PCP_B, UDP dport=$DSTPORT_B)"
sudo tc filter add dev "$IF" egress pref 200 protocol 802.1Q \
  flower \
    vlan_id "$VID_B" \
    vlan_prio "$PCP_B" \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port "$DSTPORT_B" \
  action police rate "$RATE_B" burst "$BURST_B" mtu 1500 conform-exceed drop

# select: drop every other
if [[ "$DROP_OTHERS" == "true" ]]; then
  echo "[*] drop everything else on EGRESS (pref 65000)"
  sudo tc filter add dev "$IF" egress pref 65000 protocol all matchall action drop
fi

echo
echo "==== EGRESS filters on $IF ===="
sudo tc -s filter show dev "$IF" egress

echo
echo "==== Qdisc stats on $IF ===="
sudo tc -s qdisc show dev "$IF"

# cat <<'EOF'
# Usage:
#   IF=tap1 RATE_A=50000kbit BURST_A=2000 DROP_OTHERS=true ./polishing.bash

# Notes:
#   - EGRESS policing only, clsact is attached idempotently.
#   - Change VLAN/PCP/ports via env vars: VID_A, PCP_A, DSTPORT_A, ...
# EOF
