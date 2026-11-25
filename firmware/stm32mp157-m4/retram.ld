/* SPDX-License-Identifier: GPL-2.0-only */
ENTRY(reset_handler)

MEMORY {
    RETRAM (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

PHDRS {
    code PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    .vectors : { KEEP(*(.vectors)) } > RETRAM :code
    .text : { *(.text .text.*) } > RETRAM :code
    .resource_table : { KEEP(*(.resource_table)) } > RETRAM :data
    .bss (NOLOAD) : { *(.bss .bss.*) } > RETRAM :data

    __stack_top = ORIGIN(RETRAM) + LENGTH(RETRAM);
    ASSERT(ADDR(.bss) + SIZEOF(.bss) <= __stack_top - 256,
           "Firmware overlaps reserved stack space")
}
