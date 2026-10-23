*** Comments ***
System test: boot the real firmware ELF in Renode and drive it over USART2.
Run with `make robot` (needs the Python packages listed in chapter 25).

*** Variables ***
${ELF}          ${CURDIR}/../build/firmware.elf
${BOARD}        ${CURDIR}/../../common/board.resc
${UART}         sysbus.usart2
${PROMPT}       >

*** Keywords ***
Boot Firmware
    Execute Command          $bin=@${ELF}
    Execute Command          include @${BOARD}
    Create Terminal Tester   ${UART}    timeout=1    defaultPauseEmulation=true
    Start Emulation
    Wait For Line On Uart    lesson 25: command shell
    Wait For Prompt On Uart  ${PROMPT}

Send Command
    [Arguments]    ${command}
    Write Line To Uart       ${command}    waitForEcho=true

*** Test Cases ***
Prints Help
    Boot Firmware
    Send Command             help
    Wait For Line On Uart    commands: help, led

Turns The Red LED On And Off
    Boot Firmware
    Create LED Tester        sysbus.gpioPortD.RedLED    defaultTimeout=0.1
    Assert LED State         false
    Send Command             led red on
    Wait For Line On Uart    ok
    Assert LED State         true
    Send Command             led red off
    Wait For Line On Uart    ok
    Assert LED State         false

Computes A Baud Rate Divider
    Boot Firmware
    Send Command             baud 16000000 115200
    # Renode reads any argument that looks like name=value as a named argument,
    # even with the "=" escaped, so match the text after "BRR=" (a substring is enough).
    Wait For Line On Uart    139 (0x008b), actual 115107 baud, error -807 ppm

Rejects Unknown Commands
    Boot Firmware
    Send Command             selfdestruct
    Wait For Line On Uart    error: unknown command 'selfdestruct'
