#include "pic.h"

#define PIC_MASTER_CMD      0x20
#define PIC_MASTER_DATA     0x21
#define PIC_SLAVE_CMD       0xA0
#define PIC_SLAVE_DATA      0xA1

#define ICW1_ICW4           0x01
#define ICW1_INIT           0x10
#define ICW4_8086           0x01

#define PIC_EOI             0x20

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

void pic_init(void)
{
    outb(PIC_MASTER_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC_SLAVE_CMD, ICW1_INIT | ICW1_ICW4);

    outb(PIC_MASTER_DATA, 32);  /* IRQ0-7 → ISR 32-39 */
    outb(PIC_SLAVE_DATA, 40);   /* IRQ8-15 → ISR 40-47 */

    outb(PIC_MASTER_DATA, 0x04);  /* master IRQ2 = slave connection */
    outb(PIC_SLAVE_DATA, 0x02);   /* slave cascade identity */

    outb(PIC_MASTER_DATA, ICW4_8086);
    outb(PIC_SLAVE_DATA, ICW4_8086);

    outb(PIC_MASTER_DATA, 0xFD);  /* mask all except IRQ1 (keyboard) */
    outb(PIC_SLAVE_DATA, 0xFF);   /* mask all slave IRQs */
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC_SLAVE_CMD, PIC_EOI);
    outb(PIC_MASTER_CMD, PIC_EOI);
}

void pic_disable_irq(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    uint8_t  mask = inb(port) | (1 << (irq % 8));
    outb(port, mask);
}

void pic_enable_irq(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    uint8_t  mask = inb(port) & ~(1 << (irq % 8));
    outb(port, mask);
}
