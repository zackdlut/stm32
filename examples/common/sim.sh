#!/usr/bin/env bash
# Start Renode interactively for a firmware image.
#
#   debug UART -> TCP port 3456   (connect with: nc localhost 3456)
#   GDB        -> TCP port 3333   (connect with: make gdb)
#
# Type `start` in the Renode monitor to run, `pause` to stop, `quit` to exit.
#
# GUI=1 opens the monitor in its own window and shows the UART in a terminal
# window as well (needs a display: WSLg, X11 or Wayland).
# RESC and UART select the board as in run.sh.
set -euo pipefail

elf="$(realpath "$1")"
common="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
renode="${RENODE:-renode}"
resc="${RESC:-${common}/board.resc}"
uart="${UART:-usart2}"

if [[ "${GUI:-0}" != 0 ]]; then
    ui_args=()
    analyzer=(-e "showAnalyzer sysbus.${uart}")
    echo "${uart^^}: analyzer window or nc localhost 3456    GDB: make gdb    Monitor window: start / pause / quit"
else
    ui_args=(--disable-gui --console)
    analyzer=()
    echo "${uart^^}: nc localhost 3456    GDB: make gdb    Renode monitor: start / pause / quit"
fi

exec "${renode}" "${ui_args[@]}" \
    -e "\$bin=@${elf}" \
    -e "i @${resc}" \
    -e "emulation CreateServerSocketTerminal 3456 \"uart\" false" \
    -e "connector Connect sysbus.${uart} uart" \
    "${analyzer[@]}" \
    -e "machine StartGdbServer 3333"
