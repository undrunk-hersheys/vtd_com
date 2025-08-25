#!/usr/bin/env bash
set -euo pipefail
pkill -f "../build/receiver" || true
pkill -f "../build/sender" || true
echo "[stop] processes killed"
