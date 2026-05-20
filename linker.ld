/*
 * Kernel linker script
 *
 * The bootloader loads the kernel flat binary to physical 0x10000.
 * start.asm (_start) must be the very first instruction, so we put
 * .text.entry first, which is where start.asm places _start.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text.entry)      /* arch/x86/start.asm → must be first */
        *(.text .text.*)
    }

    .rodata ALIGN(4) : {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4) : {
        *(.data .data.*)
    }

    .bss ALIGN(4) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /* Discard debug and other non-essential sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
