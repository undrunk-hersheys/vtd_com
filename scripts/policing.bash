#!/usr/bin/env bash
set -euo pipefail

IF=tap1

echo "[*] attach clsact (idempotent)"
sudo tc qdisc add dev "$IF" clsact 2>/dev/null || true

echo "[*] clear existing ingress filters"
sudo tc filter del dev "$IF" ingress 2>/dev/null || true

# ───────────────────────────────────────────────────────────────────
# 스트림 A: VLAN 100, PCP 3, UDP dst 44444 → 1 Mbit policing
# 먼저 매칭되도록 낮은 pref 사용
sudo tc filter add dev "$IF" ingress pref 100 protocol 802.1Q \
  flower \
    vlan_id 100 \
    vlan_prio 3 \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port 44444 \
  action police rate 1mbit burst 20000 conform-exceed drop

# 스트림 B: VLAN 200, PCP 5, UDP dst 55555 → 50 kbit policing
sudo tc filter add dev "$IF" ingress pref 200 protocol 802.1Q \
  flower \
    vlan_id 200 \
    vlan_prio 5 \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port 55555 \
  action police rate 50kbit burst 10000 conform-exceed drop

# 나머지는 전부 드롭 (가장 마지막에 매칭되게 큰 pref)
sudo tc filter add dev "$IF" ingress pref 65000 protocol all matchall action drop
# ───────────────────────────────────────────────────────────────────

echo
echo "==== ingress filters on $IF ===="
sudo tc -s filter show dev "$IF" ingress
