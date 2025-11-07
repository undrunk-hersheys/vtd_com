#!/usr/bin/env bash
set -euo pipefail

# ---- names ----
TAP0="tap0"
TAP1="tap1"
BR="br0"
USER_NAME=$(whoami)

# ---- fixed MACs (choose any locally-administered) ----
MAC0="de:ad:be:ef:00:01"   # tap0
MAC1="de:ad:be:ef:00:02"   # tap1

# ---- MTU ----
MTU_VAL=1500 # default 1500

echo "[*] cleanup"
sudo ip link del "$TAP0" 2>/dev/null || true
sudo ip link del "$TAP1" 2>/dev/null || true
sudo ip link del "$BR"   2>/dev/null || true

echo "[*] create TAPs"
# sudo ip tuntap add dev "$TAP0" mode tap user "$USER_NAME"
# sudo ip tuntap add dev "$TAP1" mode tap user "$USER_NAME"
# echo "[*] add multi queue"
sudo ip tuntap add dev "$TAP0" mode tap user "$USER_NAME" multi_queue
sudo ip tuntap add dev "$TAP1" mode tap user "$USER_NAME" multi_queue

echo "[*] set fixed MACs"
sudo ip link set dev "$TAP0" address "$MAC0"
sudo ip link set dev "$TAP1" address "$MAC1"

echo "[*] set MTU for TAPs"
sudo ip link set dev "$TAP0" mtu "$MTU_VAL"
sudo ip link set dev "$TAP1" mtu "$MTU_VAL"

echo "[*] bring TAPs up (no IPs; pure L2 test)"
sudo ip addr flush dev "$TAP0" || true
sudo ip addr flush dev "$TAP1" || true
sudo ip link set "$TAP0" up
sudo ip link set "$TAP1" up

echo "[*] create bridge"
sudo ip link add name "$BR" type bridge \
    vlan_filtering 0 stp_state 0 ageing_time 300
sudo ip link set "$BR" up

echo "[*] enslave TAPs to bridge"
sudo ip link set "$TAP0" master "$BR"
sudo ip link set "$TAP1" master "$BR"

# learning/flood/hairpin fix
sudo bridge link set dev "$TAP0" learning off hairpin off flood off mcast_flood off
sudo bridge link set dev "$TAP1" learning off hairpin off flood off mcast_flood off

# flush remaining learning entries (not static)
sudo bridge fdb flush br "$BR" self 2>/dev/null || true

# static FDB set
sudo bridge fdb replace de:ad:be:ef:00:01 dev "$TAP0" master static
sudo bridge fdb replace de:ad:be:ef:00:02 dev "$TAP1" master static


# IPv6 off on bridge (optional)
sudo sysctl -w net.ipv6.conf."$BR".disable_ipv6=1 >/dev/null

echo "[*] minimal qdisc (pfifo) on TAPs for deterministic queueing"
sudo tc qdisc replace dev "$TAP0" root pfifo limit 1000
sudo tc qdisc replace dev "$TAP1" root pfifo limit 1000

echo "[*] disable unknown-unicast flooding per-port (will program static FDB below)"
sudo bridge link set dev "$TAP0" flood off mcast_flood off
sudo bridge link set dev "$TAP1" flood off mcast_flood off

echo "[*] program static FDB (no flooding path; only exact-match forward)"
sudo bridge fdb replace "$MAC0" dev "$TAP0" master static
sudo bridge fdb replace "$MAC1" dev "$TAP1" master static

echo "[+] setup complete"
