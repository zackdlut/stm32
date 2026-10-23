#!/usr/bin/env bash
# Download the vendor code used from lesson 11 on:
#   - ARM CMSIS-Core headers (core_cm4.h and friends)
#   - ST CMSIS device headers for STM32F4 (stm32f407xx.h, system_stm32f4xx.c)
#   - ST STM32F4 HAL driver
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

CMSIS_CORE_TAG=5.9.0
CMSIS_DEVICE_TAG=v2.6.9
HAL_TAG=v1.8.5

clone() {
    local url="$1" tag="$2" dir="$3"
    if [[ -d "${dir}" ]]; then
        echo "${dir} already present"
        return
    fi
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "${tag}" "${url}" "${dir}"
    rm -rf "${dir}/.git"
    echo "fetched ${dir} (${tag})"
}

clone https://github.com/STMicroelectronics/cmsis-device-f4.git "${CMSIS_DEVICE_TAG}" cmsis-device-f4
clone https://github.com/STMicroelectronics/stm32f4xx-hal-driver.git "${HAL_TAG}" stm32f4xx-hal-driver

if [[ ! -d cmsis-core ]]; then
    mkdir -p cmsis-core/Include
    base="https://raw.githubusercontent.com/ARM-software/CMSIS_5/${CMSIS_CORE_TAG}/CMSIS/Core/Include"
    for f in cmsis_compiler.h cmsis_gcc.h cmsis_version.h core_cm4.h mpu_armv7.h; do
        curl -fsSL "${base}/${f}" -o "cmsis-core/Include/${f}"
    done
    echo "fetched cmsis-core (${CMSIS_CORE_TAG})"
fi
