#include "include/types.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "arch/x86/idt.h"
#include "arch/x86/pic.h"

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
    vga_puts("miniOS v0.2  --  Phase 2 (IDT + PIC + Keyboard)\n");
    vga_puts("------------------------------------\n\n");

    /* Initialize interrupts */
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("Initializing IDT...\n");
    idt_init();

    vga_puts("Initializing PIC...\n");
    pic_init();

    vga_puts("Initializing keyboard...\n");
    keyboard_init();

    /* Enable interrupts */
    __asm__ volatile("sti");

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("\n[OK] IDT loaded\n");
    vga_puts("[OK] PIC configured (IRQ1 enabled)\n");
    vga_puts("[OK] Keyboard driver active\n\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("> ");

    for (;;)
        __asm__ volatile("hlt");
}
