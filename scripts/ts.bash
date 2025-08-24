#!/bin/bash
set -euo pipefail

# Interface and bridge names
TAP0="tap0"
TAP1="tap1"
BRIDGE="br0"
USER_NAME=$(whoami)

# Fixed MACs for TAPs
MAC0="de:ad:be:ef:00:01"   # tap0
MAC1="de:ad:be:ef:00:02"   # tap1

# [MOD] Give br0 a unique MAC so it never equals a TAP MAC
BR_MAC="de:ad:be:ef:ff:ff"  # must not conflict with MAC0/MAC1

echo "[*] Cleaning up existing interfaces if any..."
sudo ip link delete "$TAP0" 2>/dev/null || true
sudo ip link delete "$TAP1" 2>/dev/null || true
sudo ip link delete "$BRIDGE" type bridge 2>/dev/null || true

echo "[*] Creating TAP interfaces..."
sudo ip tuntap add dev "$TAP0" mode tap user "$USER_NAME"
sudo ip tuntap add dev "$TAP1" mode tap user "$USER_NAME"

echo "[*] Setting fixed MAC addresses for TAPs..."
sudo ip link set dev "$TAP0" address "$MAC0"   # [MOD]
sudo ip link set dev "$TAP1" address "$MAC1"   # [MOD]

echo "[*] Bringing TAP interfaces up..."
sudo ip link set "$TAP0" up
sudo ip link set "$TAP1" up

echo "[*] Assigning IP addresses to TAP interfaces..."
sudo ip addr add 10.0.0.1/24 dev "$TAP0"
sudo ip addr add 10.0.0.2/24 dev "$TAP1"

echo "[*] Creating and configuring bridge: $BRIDGE"
sudo ip link add name "$BRIDGE" type bridge
# [MOD] give br0 distinct MAC to avoid “dst==bridge” local-consume
sudo ip link set dev "$BRIDGE" address "$BR_MAC"      # [MOD]
# [MOD] disable VLAN filtering for simple pass-through
sudo ip link set dev "$BRIDGE" type bridge vlan_filtering 0   # [MOD]
sudo ip link set "$BRIDGE" up

echo "[*] Connecting TAP interfaces to bridge..."
sudo ip link set "$TAP0" master "$BRIDGE"
sudo ip link set "$TAP1" master "$BRIDGE"

# [MOD] re-up after enslave (some envs keep state disabled otherwise)
sudo ip link set "$TAP0" up    # [MOD]
sudo ip link set "$TAP1" up    # [MOD]

# [MOD] (optional but handy) ensure frames are delivered to user-space
sudo ip link set dev "$TAP0" promisc on  # [MOD]
sudo ip link set dev "$TAP1" promisc on  # [MOD]

# [MOD] clean FDB and (optionally) add static entries for determinism
sudo bridge fdb flush br "$BRIDGE"       # [MOD]
sudo bridge fdb add "$MAC0" dev "$TAP0" master static  # [MOD]
sudo bridge fdb add "$MAC1" dev "$TAP1" master static  # [MOD]

# IPv6 control (bridge only)
sudo sysctl -w net.ipv6.conf."$BRIDGE".disable_ipv6=1

echo "[*] Queueing (qdisc) setup..."
sudo tc qdisc replace dev "$TAP0" root pfifo limit 1000
sudo tc qdisc replace dev "$TAP1" root pfifo limit 1000
# reset to default: tc qdisc del dev tapX root

echo "[+] Setup complete. Current network configuration:"
ip addr show "$TAP0"
ip addr show "$TAP1"
ip addr show "$BRIDGE"

echo "[+] Qdisc state:"
tc qdisc show dev "$TAP0"
tc qdisc show dev "$TAP1"
tc qdisc show dev "$BRIDGE"

echo "[+] Bridge/port state:"
bridge link
bridge fdb show br "$BRIDGE"
bridge vlan show
