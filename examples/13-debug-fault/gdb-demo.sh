#!/usr/bin/env bash
# Start Renode with a GDB server, drive it with debug.gdb, then stop Renode.
# RESC, UART and GDB_SCRIPT select the board (the Makefile sets them).
set -euo pipefail

elf="$(realpath "$1")"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
common="${here}/../common"
renode="${RENODE:-renode}"
resc="${RESC:-${common}/board.resc}"
uart="${UART:-usart2}"
gdb_script="$(realpath "${GDB_SCRIPT:-${here}/debug.gdb}")"

log="$(dirname "${elf}")/renode-gdb.log"

rm -f "${log}"

# -P -1: no monitor at all, so Renode keeps running until we kill it.
"${renode}" --disable-gui -P -1 --plain \
    -e "\$bin=@${elf}" \
    -e "i @${resc}" \
    -e "sysbus.${uart} CreateFileBackend @$(dirname "${elf}")/uart.log true" \
    -e "machine StartGdbServer 3333" \
    > "${log}" 2>&1 < /dev/null &
renode_pid=$!
cleanup() { kill "${renode_pid}" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

for _ in $(seq 100); do
    if grep -q "GDB server with all CPUs started" "${log}" 2>/dev/null; then
        break
    fi
    sleep 0.2
done

gdb-multiarch -q -batch -x "${gdb_script}" "${elf}"
