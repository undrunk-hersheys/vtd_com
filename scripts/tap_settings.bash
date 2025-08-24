#!/bin/bash
#!/bin/bash

# Interface and bridge names
TAP0="tap0"
TAP1="tap1"
BRIDGE="br0"
USER_NAME=$(whoami)

# MAC0="de:ad:be:ef:00:01"   # tap0
# MAC1="de:ad:be:ef:00:02"   # tap1

echo "[*] Cleaning up existing interfaces if any..."

# Delete existing TAP interfaces (ignore errors)
sudo ip link delete $TAP0 2>/dev/null
sudo ip link delete $TAP1 2>/dev/null

# Delete existing bridge
sudo ip link delete $BRIDGE type bridge 2>/dev/null

echo "[*] Creating TAP interfaces..."
sudo ip tuntap add dev $TAP0 mode tap user $USER_NAME
sudo ip tuntap add dev $TAP1 mode tap user $USER_NAME

# echo "[*] Setting fixed MAC addresses for TAPs..."
# sudo ip link set dev $TAP0 address $MAC0     # [MOD]
# sudo ip link set dev $TAP1 address $MAC1     # [MOD]

echo "[*] Bringing TAP interfaces up..."
sudo ip link set $TAP0 up
sudo ip link set $TAP1 up


echo "[*] Assigning IP addresses to TAP interfaces..."
sudo ip addr add 10.0.0.1/24 dev $TAP0
sudo ip addr add 10.0.0.2/24 dev $TAP1

echo "[*] Creating and configuring bridge: $BRIDGE"
sudo ip link add name $BRIDGE type bridge
sudo ip link set $BRIDGE up

echo "[*] Connecting TAP interfaces to bridge..."
sudo ip link set $TAP0 master $BRIDGE
sudo ip link set $TAP1 master $BRIDGE

sudo ip link set $TAP0 up
sudo ip link set $TAP1 up


# echo "[*] Assigning IP address to bridge interface..."
# sudo ip addr add 10.0.0.254/24 dev $BRIDGE

sudo sysctl -w net.ipv6.conf.br0.disable_ipv6=1

echo "[+] Setup complete. Current network configuration:"
ip addr show $TAP0
ip addr show $TAP1
ip addr show $BRIDGE

sudo tc qdisc replace dev tap0 root pfifo limit 1000
sudo tc qdisc replace dev tap1 root pfifo limit 1000
#goes to default
#tc qdisc del dev tapX root

#shows which qdiscs are active
tc qdisc show dev tap0
tc qdisc show dev tap1
tc qdisc show dev br0


