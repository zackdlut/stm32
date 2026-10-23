#!/usr/bin/env bash
# Run a firmware image headless in Renode and print what it sent on USART2.
#
#   run.sh <firmware.elf> [virtual-seconds]
#
# If the current directory contains stimulus.resc, it is included instead of a
# plain `emulation RunFor`, so a lesson can press buttons or type into the UART
# at chosen moments of virtual time. STIMULUS=other.resc picks another script.
#
# The full Renode log goes to build/renode.log. Only LED changes and errors are
# echoed, plus lines matching LOG_FILTER (a regex a lesson Makefile may set);
# set V=1 to also see warnings about registers Renode does not model.
set -euo pipefail

elf="$(realpath "$1")"
seconds="${2:-1}"
common="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
renode="${RENODE:-renode}"
build_dir="$(dirname "${elf}")"
uart_log="${build_dir}/uart.log"
renode_log="${build_dir}/renode.log"

rm -f "${uart_log}"

stimulus="${STIMULUS:-stimulus.resc}"
if [[ -f "${stimulus}" ]]; then
    run_cmd="i @$(realpath "${stimulus}")"
else
    run_cmd="emulation RunFor \"${seconds}\""
fi

filter='LED state changed|ERROR|CPU abort'
if [[ -n "${LOG_FILTER:-}" ]]; then
    filter="${filter}|${LOG_FILTER}"
fi
if [[ "${V:-0}" != 0 ]]; then
    filter="${filter}|WARNING"
fi

"${renode}" --disable-gui --console --plain \
    -e "\$bin=@${elf}" \
    -e "i @${common}/board.resc" \
    -e "sysbus.usart2 CreateFileBackend @${uart_log} true" \
    -e "${run_cmd}" \
    -e "quit" \
    > "${renode_log}" 2>&1 || true

grep -E "${filter}" "${renode_log}" || true

printf '\n----- USART2 output -----\n'
if [[ -f "${uart_log}" ]]; then
    cat "${uart_log}"
fi
printf '\n-------------------------\n'
