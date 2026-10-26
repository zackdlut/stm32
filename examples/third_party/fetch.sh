#!/usr/bin/env bash
# Download the vendor code used from lesson 11 on:
#   - ARM CMSIS-Core headers (core_cm4.h, core_cm3.h and friends)
#   - ST CMSIS device headers for STM32F4 (stm32f407xx.h, system_stm32f4xx.c)
#     and STM32F1 (stm32f103xb.h, system_stm32f1xx.c; appendix C)
#   - ST STM32F4 HAL driver
#   - FreeRTOS kernel, TinyUSB, Unity test framework (lessons 22, 24, 25)
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

CMSIS_CORE_TAG=5.9.0
CMSIS_DEVICE_TAG=v2.6.9
CMSIS_DEVICE_F1_TAG=v4.3.5   # appendix C
HAL_TAG=v1.8.5
FREERTOS_TAG=V11.3.2     # lesson 22
TINYUSB_TAG=0.21.0       # lesson 24
UNITY_TAG=v2.6.1         # lesson 25

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
clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git "${FREERTOS_TAG}" freertos-kernel
clone https://github.com/hathach/tinyusb.git "${TINYUSB_TAG}" tinyusb
clone https://github.com/ThrowTheSwitch/Unity.git "${UNITY_TAG}" unity

# Appendix C (BOARD=dkx): STM32F103 device headers, Cortex-M3 core header.
clone https://github.com/STMicroelectronics/cmsis-device-f1.git "${CMSIS_DEVICE_F1_TAG}" cmsis-device-f1

# File by file, so a cmsis-core fetched before core_cm3.h was added gets it too.
mkdir -p cmsis-core/Include
base="https://raw.githubusercontent.com/ARM-software/CMSIS_5/${CMSIS_CORE_TAG}/CMSIS/Core/Include"
for f in cmsis_compiler.h cmsis_gcc.h cmsis_version.h core_cm3.h core_cm4.h mpu_armv7.h; do
    if [[ ! -f "cmsis-core/Include/${f}" ]]; then
        curl -fsSL "${base}/${f}" -o "cmsis-core/Include/${f}"
        echo "fetched cmsis-core/Include/${f} (${CMSIS_CORE_TAG})"
    fi
done
