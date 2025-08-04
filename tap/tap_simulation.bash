#!/bin/bash

#not working in the vscode ssh terminal

# create new terminal window and run the executable from current directory
x-terminal-emulator -e bash -c "cd $(dirname "$0") && ./read_eth.exe; exec bash"
echo "[*] Receiving process."

# create another terminal window and run the sending executable from current directory
x-terminal-emulator -e bash -c "cd $(dirname "$0") && ./send_eth.exe; exec bash"
echo "[*] Sending process."

# create another terminal window and run the evalution process
# working on ...



