/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk. */
ENTRY(harmony_boot)
MEMORY { FLASH (rx) : ORIGIN = 0x08008800, LENGTH = 0x4000 }
SECTIONS {
    .text : {
        KEEP(*(.text.harmony_boot))
        KEEP(*(.text.harmony_tick))
        KEEP(*(.text.harmony_systick))
        KEEP(*(.text.harmony_bin))
        *(.text*) *(.rodata*)
    } > FLASH
    .data : { *(.data*) } > FLASH
    .bss : { *(.bss*) *(COMMON) } > FLASH
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.eh_frame*) }
    ASSERT(SIZEOF(.data) == 0, "Extension must not require data initialization")
    ASSERT(SIZEOF(.bss) == 0, "Extension must not allocate uninitialized globals")
}
