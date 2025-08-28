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
  
./receiver | tee ../log/nopol.log  
  
./sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --count 0 --interval-us 0  
  
./sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --count 0 --interval-us 0  
  
#### with policing  
  
./scripts/setup_tap_bridge.bash  
./scripts/policing.bash  
  
./receiver | tee ../log/pol.log  
  
./sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --count 0 --interval-us 0  
  
./sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --count 0 --interval-us 0  

#### to compare
./scripts/avg.bash  