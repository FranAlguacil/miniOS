#include "vga.h"

static volatile uint16_t *const vga_buf = (volatile uint16_t *)VGA_ADDR;
static int    vga_row   = 0;
static int    vga_col   = 0;
static uint8_t vga_attr = 0x07;    /* light-grey on black */

static inline uint16_t make_entry(char c, uint8_t attr)
{
    return (uint16_t)(unsigned char)c | ((uint16_t)attr << 8);
}

void vga_init(void)
{
    vga_attr = 0x07;
    vga_clear();
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_ROWS * VGA_COLS; i++)
        vga_buf[i] = make_entry(' ', vga_attr);
    vga_row = vga_col = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_attr = (uint8_t)(((uint8_t)bg << 4) | ((uint8_t)fg & 0x0F));
}

static void scroll_up(void)
{
    for (int r = 1; r < VGA_ROWS; r++)
        for (int c = 0; c < VGA_COLS; c++)
            vga_buf[(r - 1) * VGA_COLS + c] = vga_buf[r * VGA_COLS + c];

    for (int c = 0; c < VGA_COLS; c++)
        vga_buf[(VGA_ROWS - 1) * VGA_COLS + c] = make_entry(' ', vga_attr);

    vga_row = VGA_ROWS - 1;
}

void vga_putchar(char c)
{
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row >= VGA_ROWS) scroll_up();
        return;
    }
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    if (c == '\t') {
        int spaces = 8 - (vga_col % 8);
        for (int i = 0; i < spaces; i++) vga_putchar(' ');
        return;
    }

    vga_buf[vga_row * VGA_COLS + vga_col] = make_entry(c, vga_attr);
    if (++vga_col >= VGA_COLS) {
        vga_col = 0;
        if (++vga_row >= VGA_ROWS) scroll_up();
    }
}

void vga_puts(const char *s)
{
    while (*s) vga_putchar(*s++);
}

void vga_puthex(uint32_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    vga_putchar('0'); vga_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        vga_putchar(hex[(val >> i) & 0xF]);
}

void vga_putdec(uint32_t val)
{
    if (val == 0) { vga_putchar('0'); return; }
    char buf[12];
    int  i = 0;
    while (val) { buf[i++] = (char)('0' + val % 10); val /= 10; }
    while (i--) vga_putchar(buf[i]);
}
