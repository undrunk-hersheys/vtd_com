#!/usr/bin/env bash
set -euo pipefail

IF=tap1

# 0) clsact 달기 (있으면 패스)
sudo tc qdisc add dev "$IF" clsact 2>/dev/null || true

# 1) 기존 ingress 필터/체인 정리
sudo tc filter del dev "$IF" ingress 2>/dev/null || true

# 
# 스트림 A: VLAN 100, PCP 3, UDP dst 44444 → 1 Mbit policing
sudo tc filter add dev "$IF" ingress protocol 802.1Q \
  flower \
    vlan_id 100 \
    vlan_prio 3 \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port 44444 \
  action police rate 1mbit burst 10000 conform-exceed drop \
  action pass

# 스트림 B: VLAN 200, PCP 5, UDP dst 55555 → 500 kbit policing
sudo tc filter add dev "$IF" ingress protocol 802.1Q \
  flower \
    vlan_id 200 \
    vlan_prio 5 \
    vlan_ethtype ipv4 \
    ip_proto udp \
    dst_port 55555 \
  action police rate 500kbit burst 5000 conform-exceed drop \
  action pass


sudo tc filter add dev "$IF" ingress pref 10000 protocol all matchall action drop

echo
echo "==== ingress filters on $IF ===="
sudo tc -s filter show dev "$IF" ingress
