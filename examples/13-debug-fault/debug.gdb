# Scripted GDB session for lesson 13 (used by gdb-demo.sh).
# Every command here also works interactively after `make sim` + `make gdb`.

set pagination off
set confirm off
target remote :3333

echo \n### 1. Stop in compute_average and look around\n
break compute_average
continue
backtrace
info args
print g_samples
print samples[0]@4
print/x $pc

echo \n### 2. Peripheral registers are memory: read them as 32-bit words\n
x/6xw 0x40020C00
print/x ((GPIO_TypeDef *)0x40020C00)->MODER
echo Renode monitor commands work from GDB too:\n
monitor gpioPortD.RedLED State

echo \n### 3. Arm a breakpoint on the fault report, then step into the bug\n
break fault_report
next
info locals
continue

echo \n### 4. Inside fault_report: who faulted, and where?\n
backtrace
print/x *frame
info symbol frame->pc
info line *frame->pc
monitor gpioPortD.RedLED State

kill
quit
