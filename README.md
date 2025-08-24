# vtd_com



### Usage
chmod +x scripts/tap_settings.bash  
./scripts/tap_settings.bash  
mkdir build  
cd build  
cmake ..  
make  
./receiver  

// in another terminal run sender  
./sender --dst-mac de:ad:be:ef:00:02 --count 10 --interval-us 1000000  

