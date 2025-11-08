#!/usr/bin/env bash
set -euo pipefail


find_terminal() {
    if command -v xfce4-terminal >/dev/null 2>&1; then
        echo "xfce4-terminal"
    elif command -v gnome-terminal >/dev/null 2>&1; then
        echo "gnome-terminal"
    elif command -v xterm >/dev/null 2>&1; then
        echo "xterm"
    else
        echo "no-terminal"
    fi
}

TERM_CMD=$(find_terminal)

if [ "$TERM_CMD" = "no-terminal" ]; then
    echo "Can not find selected terminal."
    exit 1
fi

./scripts/setup_tap_bridge.bash

# commands runs in each terminal
CMD1="./build/receiver | tee ./log/nopol.csv"
CMD2="./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --payload 1000 --count 300000 --interval-us 10"
CMD3="./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --payload 1000 --count 300000 --interval-us 10"

# xfce4-terminal, gnome-terminal, xterm
case "$TERM_CMD" in
  xfce4-terminal)
    xfce4-terminal --hold -e "bash -lc '$CMD1'" &
    sleep 3
    xfce4-terminal --hold -e "bash -lc '$CMD2'" &
    xfce4-terminal --hold -e "bash -lc '$CMD3'" &
    ;;

  gnome-terminal)
    gnome-terminal -- bash -lc "$CMD1; exec bash" &
    sleep 3
    gnome-terminal -- bash -lc "$CMD2; exec bash" &
    gnome-terminal -- bash -lc "$CMD3; exec bash" &
    ;;

  xterm)
    xterm -hold -e bash -lc "$CMD1" &
    sleep 3
    xterm -hold -e bash -lc "$CMD2" &
    xterm -hold -e bash -lc "$CMD3" &
    ;;

esac

echo "Press Enter to continue."
read -r
sleep 2

./scripts/setup_tap_bridge.bash
./scripts/polishing.bash  

# commands runs in each terminal
CMD4="./build/receiver | tee ./log/pol.csv"
# CMD1="./build/receiver | tee ./log/pol.log"
CMD5="./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 3 --vid 100 --src-port 33333 --dst-port 44444 --payload 1000 --count 300000 --interval-us 10"
CMD6="./build/sender --tap tap0 --dst-mac de:ad:be:ef:00:02 --pcp 5 --vid 200 --src-port 33334 --dst-port 55555 --payload 1000 --count 300000 --interval-us 10"


# xfce4-terminal, gnome-terminal, xterm
case "$TERM_CMD" in
  xfce4-terminal)
    xfce4-terminal --hold -e "bash -lc '$CMD4'" &
    sleep 3
    xfce4-terminal --hold -e "bash -lc '$CMD5'" &
    xfce4-terminal --hold -e "bash -lc '$CMD6'" &
    ;;

  gnome-terminal)
    gnome-terminal -- bash -lc "$CMD4; exec bash" &
    sleep 3
    gnome-terminal -- bash -lc "$CMD5; exec bash" &
    gnome-terminal -- bash -lc "$CMD6; exec bash" &
    ;;

  xterm)
    xterm -hold -e bash -lc "$CMD4" &
    sleep 3
    xterm -hold -e bash -lc "$CMD5" &
    xterm -hold -e bash -lc "$CMD6" &
    ;;

esac
