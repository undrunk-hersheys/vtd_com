# vtd_com



### Usage
chmod +x scripts/setup_tap_bridge.bash  
./scripts/setup_tap_bridge.bash  
mkdir build  
cd build  
cmake ..  
make  
./receiver  

// in another terminal run sender  
./sender --dst-mac de:ad:be:ef:00:02 --count 10 --interval-us 1000000  


### to test bottleneck

./scripts/bottleneck_on.bash  
./scripts/bottleneck_off.bash  
  
### to test Qci(IEEE 802.1Qci)  
  
#### without policing  
  
./scripts/setup_tap_bridge.bash  
  
./build/receiver | tee ./log/nopol.log  
  
./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --payload 1000 --count 10000 --interval-us 10  
  
./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --payload 1000 --count 10000 --interval-us 10

./log/avg_mdir.bash
  
#### with policing  
  
./scripts/setup_tap_bridge.bash  
./scripts/policing.bash  
  
./build/receiver | tee ./log/pol.log  

./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --payload 1000 --count 10000 --interval-us 10  
  
./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --payload 1000 --count 10000 --interval-us 10

./log/avg_mdir.bash

#### to compare
./scripts/avg.bash  