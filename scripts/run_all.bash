#!/usr/bin/env bash
set -euo pipefail

# 0) 브리지/탭 구성 (네가 쓰는 tss.bash 또는 내가 준 호환 스크립트)
./tss.bash

# 1) receiver 먼저 실행 (별도 프로세스로 백그라운드)
../build/receiver >receiver.log 2>&1 &
rx_pid=$!
echo "[run] receiver pid=$rx_pid"
sleep 0.2

# 2) sender 실행 (별도 프로세스로 백그라운드)
../build/sender --dst-mac de:ad:be:ef:00:02 >sender.log 2>&1 &
tx_pid=$!
echo "[run] sender pid=$tx_pid"

# 3) 캐리어 뜨는 순간까지 최대 3초 폴링 후, 포트 상태 강제 forwarding
deadline=$((SECONDS+3))
tap0_carrier=0
tap1_carrier=0

while (( SECONDS < deadline )); do
  [[ -r /sys/class/net/tap0/carrier ]] && tap0_carrier=$(cat /sys/class/net/tap0/carrier || echo 0)
  [[ -r /sys/class/net/tap1/carrier ]] && tap1_carrier=$(cat /sys/class/net/tap1/carrier || echo 0)
  if [[ "$tap0_carrier" == "1" || "$tap1_carrier" == "1" ]]; then
    break
  fi
  sleep 0.05
done

# 안전빵: 포트 state forwarding 강제 (password 없이 sudo가 필요하면 sudoers 조정)
sudo bridge link set dev tap0 state forwarding || true
sudo bridge link set dev tap1 state forwarding || true

echo "---- link ----"
ip -br link show dev tap0
ip -br link show dev tap1
ip -br link show dev br0
echo "---- bridge ----"
bridge link

echo "[run] tail logs (Ctrl+C로 종료)"
tail -F receiver.log sender.log
