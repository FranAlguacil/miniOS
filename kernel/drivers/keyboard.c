#include "keyboard.h"
#include "vga.h"
#include "../arch/x86/pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static const char scancode_to_ascii[] = {
    0,    27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler(void)
{
    uint8_t scancode = inb(KBD_DATA_PORT);
    uint8_t released = (scancode & 0x80) != 0;

    if (released) return;  /* ignore key release */

    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c) vga_putchar(c);
    }

    pic_eoi(1);
}

void keyboard_init(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("Keyboard initialized. Type something:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    pic_enable_irq(1);
}
