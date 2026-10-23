#!/usr/bin/env bash
# Start Renode interactively for a firmware image.
#
#   USART2 -> TCP port 3456   (connect with: nc localhost 3456)
#   GDB    -> TCP port 3333   (connect with: make gdb)
#
# Type `start` in the Renode monitor to run, `pause` to stop, `quit` to exit.
set -euo pipefail

elf="$(realpath "$1")"
common="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
renode="${RENODE:-renode}"

echo "USART2: nc localhost 3456    GDB: make gdb    Renode monitor: start / pause / quit"

exec "${renode}" --disable-gui --console \
    -e "\$bin=@${elf}" \
    -e "i @${common}/board.resc" \
    -e "emulation CreateServerSocketTerminal 3456 \"uart\" false" \
    -e "connector Connect sysbus.usart2 uart" \
    -e "machine StartGdbServer 3333"
