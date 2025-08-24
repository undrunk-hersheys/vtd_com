# IFB 
sudo modprobe ifb
sudo ip link add ifb0 type ifb
sudo ip link set ifb0 up

# tap1 ingress IFB redirect
sudo tc qdisc add dev tap1 ingress
sudo tc filter add dev tap1 ingress matchall \
    action mirred egress redirect dev ifb0

# IFB conjunction
sudo tc qdisc replace dev ifb0 root tbf rate 1mbit burst 32k latency 400ms
