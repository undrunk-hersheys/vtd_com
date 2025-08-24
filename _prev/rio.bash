#!/usr/bin/env bash
# rio.bash - Qbv(taprio) on base dev + PCP via egress-qos-map on VLAN dev

# rio.bash is not supported in some computers, do not use it when taprio does not support more than three queues.
set -euo pipefail

NSA="nsA"
NSB="nsB"
DEV_A="vA"
DEV_B="vB"
VLAN_ID=1
VLAN_A="${DEV_A}.${VLAN_ID}"
VLAN_B="${DEV_B}.${VLAN_ID}"

IP_A="10.0.1.1/24"
IP_B="10.0.1.2/24"

SLOT_NS=300000   # 300us x 3 slots = 0.9ms cycle

require() {
  for c in ip tc python3 timeout date; do
    command -v "$c" >/dev/null 2>&1 || { echo "[ERR] '$c' not found"; exit 1; }
  done
  # iptables는 ns 안에서 실행되므로 호스트에 설치되어 있어야 함
  command -v iptables >/dev/null 2>&1 || { echo "[ERR] 'iptables' not found"; exit 1; }
}

base_time_ns() {
  python3 - <<'PY' || true
import time; print(int((time.time()+2)*1e9))
PY
}

cleanup() {
  set +e
  ip netns del "$NSA" 2>/dev/null
  ip netns del "$NSB" 2>/dev/null
  set -e
}

setup() {
  echo "[*] Clean previous state"
  cleanup

  echo "[*] Create namespaces"
  ip netns add "$NSA"
  ip netns add "$NSB"

  echo "[*] Create veth pair"
  ip link add "$DEV_A" type veth peer name "$DEV_B" numrxqueues 3 numtxqueues 3
  ip link set "$DEV_A" netns "$NSA"
  ip link set "$DEV_B" netns "$NSB"

  echo "[*] Bring up base links"
  ip -n "$NSA" link set "$DEV_A" up
  ip -n "$NSB" link set "$DEV_B" up

  echo "[*] Create VLAN subinterfaces (VID=${VLAN_ID})"
  # skb prio -> PCP 매핑: 1->3, 2->5 (기본 0->0)
  ip -n "$NSA" link add link "$DEV_A" name "$VLAN_A" type vlan id "$VLAN_ID" protocol 802.1Q egress-qos-map 0:0 1:3 2:5
  ip -n "$NSB" link add link "$DEV_B" name "$VLAN_B" type vlan id "$VLAN_ID" protocol 802.1Q

  ip -n "$NSA" addr add "$IP_A" dev "$VLAN_A"
  ip -n "$NSB" addr add "$IP_B" dev "$VLAN_B"

  ip -n "$NSA" link set "$VLAN_A" up
  ip -n "$NSB" link set "$VLAN_B" up

  echo "[*] Configure taprio (Qbv) on BASE dev ${DEV_A}"
  BASE="$(base_time_ns)"
  [[ -z "$BASE" ]] && BASE="$(($(date +%s) * 1000000000 + 2000000000))"

  # taprio on vA (NOT on vA.1)
  ip netns exec "$NSA" tc qdisc replace dev "$DEV_A" parent root handle 100: taprio \
    num_tc 3 \
    map 0 1 2 2 2 2 2 2 \
    queues 1@0 1@1 1@2 \
    base-time "$BASE" \
    sched-entry S 01 "$SLOT_NS" \
    sched-entry S 02 "$SLOT_NS" \
    sched-entry S 04 "$SLOT_NS" \
    clockid CLOCK_REALTIME

  echo "[*] L4 포트 기반 skb priority 설정 (iptables mangle OUTPUT)"
  # nsA 내부 OUTPUT 트래픽에 대해 dport별 skb prio 부여
  # 3001 -> prio 1, 3002 -> prio 2
  ip netns exec "$NSA" iptables -t mangle -F OUTPUT
  ip netns exec "$NSA" iptables -t mangle -A OUTPUT -p udp --dport 3001 -j CLASSIFY --set-class 0:1
  ip netns exec "$NSA" iptables -t mangle -A OUTPUT -p udp --dport 3002 -j CLASSIFY --set-class 0:2

  echo "[OK] Setup done."
  echo
  echo "Verify:"
  echo "  ip netns exec $NSA tc -s qdisc show dev $DEV_A"
  echo "  ip netns exec $NSA iptables -t mangle -S OUTPUT"
  echo "  ip netns exec $NSB tcpdump -i $VLAN_B -e -nn -vvv vlan"
}

show() {
  echo "=== nsA qdisc (on $DEV_A) ==="
  ip netns exec "$NSA" tc -s qdisc show dev "$DEV_A" || true
  echo
  echo "=== nsA iptables mangle OUTPUT ==="
  ip netns exec "$NSA" iptables -t mangle -S OUTPUT || true
  echo
  echo "=== Links ==="
  ip netns exec "$NSA" ip -d link show dev "$DEV_A" || true
  ip netns exec "$NSA" ip -d link show dev "$VLAN_A" || true
  ip netns exec "$NSB" ip -d link show dev "$VLAN_B" || true
}

send() {
  DSTS="10.0.1.2"
  echo "[*] Start tcpdump in nsB (Ctrl+C to stop)"
  ( ip netns exec "$NSB" timeout 8 tcpdump -i "$VLAN_B" -e -vvv -nn "udp and (port 3001 or port 3002)" ) &

  echo "[*] Send flow1 (UDP dport=3001 -> skb prio 1 -> PCP3 -> TC1 slot)"
  ip netns exec "$NSA" bash -c '
    for i in $(seq 1 200); do
      printf "flow1,%s,%s\n" "$i" "$(date +%s.%N)" > /dev/udp/10.0.1.2/3001
      usleep 2000
    done
  ' &

  echo "[*] Send flow2 (UDP dport=3002 -> skb prio 2 -> PCP5 -> TC2 slot)"
  ip netns exec "$NSA" bash -c '
    for i in $(seq 1 200); do
      printf "flow2,%s,%s\n" "$i" "$(date +%s.%N)" > /dev/udp/10.0.1.2/3002
      usleep 2000
    done
  ' &

  wait || true
  echo "[OK] Demo traffic finished."
}

help() {
  cat <<EOF
Usage: $0 [setup|show|send|cleanup]

  setup    netns/veth/VLAN 구성 + taprio(Qbv) on vA + skb prio via iptables
  show     qdisc/iptables 상태 출력
  send     두 UDP 흐름 송신 (3001→prio1→PCP3, 3002→prio2→PCP5)
  cleanup  네임스페이스 정리

Notes:
- taprio는 베이스 디바이스(vA)에 부착 (VLAN dev(vA.1) 불가)
- skb prio는 iptables mangle OUTPUT에서 dport로 분류
- vA.1의 egress-qos-map이 skb prio→PCP 변환 수행
- tcpdump에서 "vlan 1, p <PCP>"로 PCP 확인 가능
EOF
}

main() {
  require
  case "${1:-help}" in
    setup)   setup ;;
    show)    show ;;
    send)    send ;;
    cleanup) cleanup ;;
    *)       help ;;
  esac
}

main "$@"
