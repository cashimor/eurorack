/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk. */
ENTRY(drum_boot)
MEMORY { FLASH (rx) : ORIGIN = 0x08008800, LENGTH = 0x4000 }
SECTIONS {
    .text : {
        KEEP(*(.text.drum_boot)) KEEP(*(.text.drum_tick))
        KEEP(*(.text.drum_systick)) KEEP(*(.text.drum_left_irq)) KEEP(*(.text.drum_right_irq))
        *(.text*) *(.rodata*)
    } > FLASH
    .data : { *(.data*) } > FLASH
    .bss : { *(.bss*) *(COMMON) } > FLASH
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.eh_frame*) }
    ASSERT(SIZEOF(.data) == 0, "Drum requires no initialized globals")
    ASSERT(SIZEOF(.bss) == 0, "Drum runtime must stay in its reserved RAM")
}
