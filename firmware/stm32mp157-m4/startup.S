/* SPDX-License-Identifier: GPL-2.0-only */
.syntax unified
.cpu cortex-m4
.thumb

.section .vectors, "a", %progbits
.word __stack_top
.word reset_handler
.rept 14
.word fault_handler
.endr

.section .text, "ax", %progbits
.thumb_func
.global reset_handler
reset_handler:
    cpsid i
    ldr r0, =boot_marker
    ldr r1, =0x4d344f4b /* "M4OK" */
    str r1, [r0]
    dsb
1:  wfi
    b 1b

.thumb_func
fault_handler:
    b fault_handler

.section .bss, "aw", %nobits
.align 2
.global boot_marker
boot_marker:
    .space 4

/* Version 1, zero resources, two reserved words. */
.section .resource_table, "a", %progbits
.align 2
.word 1, 0, 0, 0

.section .note.GNU-stack, "", %progbits
