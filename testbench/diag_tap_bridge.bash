#!/usr/bin/env bash
set -euo pipefail
TAP0="tap0"; TAP1="tap1"; BR="br0"

echo "---- link (brief) ----"
ip -br link show "$TAP0" "$TAP1" "$BR" || true

echo "---- addrs ----"
ip addr show "$TAP0"
ip addr show "$TAP1"
ip addr show "$BR"

echo "---- qdisc ----"
tc qdisc show dev "$TAP0"
tc qdisc show dev "$TAP1"
tc qdisc show dev "$BR" || true


echo "---- bridge ports ----"
bridge link

echo "---- FDB (forwarding table) ----"
bridge fdb show br "$BR"
