#!/usr/bin/env bash
set -euo pipefail

IF="${1:-tap1}"

# ===== 원하는 병목/할당 (필요시 조정) =====
RATE_TOTAL="${RATE_TOTAL:-600kbit}"   # tap1 전체 egress cap
RATE_HIGH="${RATE_HIGH:-450kbit}"     # PCP=5 (band HIGH)
CEIL_HIGH="${CEIL_HIGH:-450kbit}"
RATE_LOW="${RATE_LOW:-150kbit}"       # PCP=3 (band LOW)
CEIL_LOW="${CEIL_LOW:-150kbit}"

# police는 guardrail (ceil보다 살짝 높게)
POLICE_HIGH="${POLICE_HIGH:-500kbit}"
POLICE_LOW="${POLICE_LOW:-180kbit}"
BURST_BYTES="${BURST_BYTES:-64000}"   # police/fq_codel 버스트 여유

echo "[*] device: $IF"

# 0) clsact는 egress 필터용, root는 HTB로
sudo tc qdisc add dev "$IF" clsact 2>/dev/null || true
sudo tc qdisc del dev "$IF" root 2>/dev/null || true

# 1) root를 HTB로, 전체 cap 설정
sudo tc qdisc add dev "$IF" root handle 1: htb default 30
sudo tc class add dev "$IF" parent 1: classid 1:1 htb rate "$RATE_TOTAL" ceil "$RATE_TOTAL"

# 2) 고우선(PCP=5) 클래스
sudo tc class add dev "$IF" parent 1:1 classid 1:10 htb rate "$RATE_HIGH" ceil "$CEIL_HIGH" prio 0
sudo tc qdisc add dev "$IF" parent 1:10 handle 10: fq_codel limit 100 target 5ms interval 50ms

# 3) 저우선(PCP=3) 클래스
sudo tc class add dev "$IF" parent 1:1 classid 1:20 htb rate "$RATE_LOW" ceil "$CEIL_LOW" prio 1
sudo tc qdisc add dev "$IF" parent 1:20 handle 20: fq_codel limit 200 target 5ms interval 50ms

# 4) VLAN PCP 기반 분류 (flower + skbedit는 불필요, 바로 classid로)
sudo tc filter add dev "$IF" protocol 802.1Q parent 1: prio 1 flower \
  vlan_prio 5 action goto chain 5
sudo tc filter add dev "$IF" protocol 802.1Q parent 1: prio 2 flower \
  vlan_prio 3 action goto chain 3

# 체인5: HIGH로 분류
sudo tc filter add dev "$IF" parent 1: chain 5 flower skip_hw \
  action pass classid 1:10
# 체인3: LOW로 분류
sudo tc filter add dev "$IF" parent 1: chain 3 flower skip_hw \
  action pass classid 1:20

# 5) (옵션) guardrail policing – egress 훅에 가볍게
sudo tc filter del dev "$IF" egress 2>/dev/null || true
# HIGH (PCP=5)
sudo tc filter add dev "$IF" egress pref 100 protocol 802.1Q \
  flower vlan_prio 5 \
  action police rate "$POLICE_HIGH" burst "$BURST_BYTES" mtu 1500 conform-exceed drop
# LOW (PCP=3)
sudo tc filter add dev "$IF" egress pref 200 protocol 802.1Q \
  flower vlan_prio 3 \
  action police rate "$POLICE_LOW" burst "$BURST_BYTES" mtu 1500 conform-exceed drop

# 6) 통계 보기
echo
echo "== qdisc =="
sudo tc -s qdisc show dev "$IF"
echo
echo "== class =="
sudo tc -s class show dev "$IF"
echo
echo "== egress police filters =="
sudo tc -s filter show dev "$IF" egress

cat <<'EOF'
