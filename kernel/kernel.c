#include "include/types.h"
#include "drivers/vga.h"

/* Called from arch/x86/start.asm after BSS is zeroed */
void kernel_main(void)
{
    vga_init();

    /* Banner */
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  __  __  _         _    ___  ____  \n");
    vga_puts(" |  \\/  |(_) _ __  (_)  / _ \\/ ___| \n");
    vga_puts(" | |\\/| || || '_ \\ | | | | | \\___ \\ \n");
    vga_puts(" | |  | || || | | || | | |_| |___) |\n");
    vga_puts(" |_|  |_||_||_| |_||_|  \\___/|____/ \n\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("miniOS v0.1  --  educational kernel\n");
    vga_puts("------------------------------------\n\n");

    /* System info */
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Arch  : x86 (i386)\n");
    vga_puts("  Mode  : 32-bit Protected Mode\n");
    vga_puts("  VGA   : text mode 80x25\n");
    vga_puts("  Stack : 0x");
    uint32_t sp;
    __asm__ volatile("mov %%esp, %0" : "=r"(sp));
    vga_puthex(sp);
    vga_putchar('\n');
    vga_putchar('\n');

    /* Phase checklist */
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("[OK] Bootloader (MBR + A20 + GDT)\n");
    vga_puts("[OK] Protected mode  (32-bit flat)\n");
    vga_puts("[OK] Kernel loaded at 0x00010000\n");
    vga_puts("[OK] VGA text driver\n");
    vga_putchar('\n');

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("[ ] Phase 2: IDT + PIC + keyboard\n");
    vga_puts("[ ] Phase 3: paging + kmalloc\n");
    vga_puts("[ ] Phase 4: scheduler + processes\n");
    vga_puts("[ ] Phase 5: syscalls\n");
    vga_puts("[ ] Phase 6: VFS + initrd\n");
    vga_puts("[ ] Phase 7: shell\n\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Kernel idle. Waiting for Phase 2...\n");

    for (;;)
        __asm__ volatile("hlt");
}
