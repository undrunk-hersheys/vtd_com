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
  
  