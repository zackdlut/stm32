#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
renode="${RENODE:-${HOME}/.local/opt/renode/renode}"

cd "${root}"
make
rm -f uart.log renode.log

"${renode}" \
    --disable-gui \
    --console \
    --plain \
    -e "\$bin=@${root}/firmware.elf" \
    -e "\$uartLog=@${root}/uart.log" \
    -e "i @${root}/stm32f4_discovery.resc"

printf '\nUSART2 output:\n'
cat "${root}/uart.log"
