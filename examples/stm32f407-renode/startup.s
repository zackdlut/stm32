.syntax unified
.cpu cortex-m4
.thumb

.section .isr_vector, "a", %progbits
.global isr_vector
isr_vector:
    .word _estack
    .word Reset_Handler

.section .text
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    ldr sp, =_estack
    bl main
    b .

.size Reset_Handler, . - Reset_Handler
