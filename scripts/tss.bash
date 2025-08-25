#!/usr/bin/env bash
set -e

# -------- helper: safe delete if exists --------
del_link() {
  local L=$1
  ip link show "$L" &>/dev/null && sudo ip link del "$L" || true
}

echo "[*] cleanup"
del_link tap0
del_link tap1
del_link br0

echo "[*] create TAPs"
# NOTE: 일부 iproute2는 'dev' 키워드를 지원하지 않음 → dev 빼고 작성
sudo ip tuntap add tap0 mode tap user "$USER"
sudo ip tuntap add tap1 mode tap user "$USER"

# (선택) 고정 MAC
sudo ip link set tap0 address de:ad:be:ef:00:01 || true
sudo ip link set tap1 address de:ad:be:ef:00:02 || true

echo "[*] create bridge"
# STP off, VLAN filtering off
sudo ip link add br0 type bridge stp_state 0 vlan_filtering 0
sudo ip link set br0 up

echo "[*] enslave + up"
sudo ip link set tap0 master br0
sudo ip link set tap1 master br0
sudo ip link set tap0 up
sudo ip link set tap1 up

echo "[*] IP to br0 (TAP들엔 IP 제거)"
sudo ip addr flush dev tap0 || true
sudo ip addr flush dev tap1 || true
sudo ip addr replace 10.0.0.254/24 dev br0

echo "[*] simple qdisc"
sudo tc qdisc replace dev tap0 root pfifo limit 1000 || true
sudo tc qdisc replace dev tap1 root pfifo limit 1000 || true

echo "---- link ----"
ip -br link show dev tap0
ip -br link show dev tap1
ip -br link show dev br0

echo "---- bridge ----"
bridge link
