#!/bin/bash
#!/bin/bash

# Interface and bridge names
TAP0="tap0"
TAP1="tap1"
BRIDGE="br0"
USER_NAME=$(whoami)

echo "[*] Cleaning up existing interfaces if any..."

# Delete existing TAP interfaces (ignore errors)
sudo ip link delete $TAP0 2>/dev/null
sudo ip link delete $TAP1 2>/dev/null

# Delete existing bridge
sudo ip link delete $BRIDGE type bridge 2>/dev/null

echo "[*] Creating TAP interfaces..."
sudo ip tuntap add dev $TAP0 mode tap user $USER_NAME
sudo ip tuntap add dev $TAP1 mode tap user $USER_NAME

echo "[*] Bringing TAP interfaces up..."
sudo ip link set $TAP0 up
sudo ip link set $TAP1 up

# echo "[*] Assigning IP addresses to TAP interfaces..."
# sudo ip addr add 10.0.0.1/24 dev $TAP0
# sudo ip addr add 10.0.0.2/24 dev $TAP1

echo "[*] Creating and configuring bridge: $BRIDGE"
sudo ip link add name $BRIDGE type bridge
sudo ip link set $BRIDGE up

echo "[*] Connecting TAP interfaces to bridge..."
sudo ip link set $TAP0 master $BRIDGE
sudo ip link set $TAP1 master $BRIDGE

# echo "[*] Assigning IP address to bridge interface..."
# sudo ip addr add 10.0.0.254/24 dev $BRIDGE

sudo sysctl -w net.ipv6.conf.br0.disable_ipv6=1

echo "[+] Setup complete. Current network configuration:"
ip addr show $TAP0
ip addr show $TAP1
ip addr show $BRIDGE

