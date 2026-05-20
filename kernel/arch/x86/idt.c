#include "idt.h"
#include "../drivers/keyboard.h"

#define IDT_ENTRIES 256

typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

extern void isr0(void);
extern void isr1(void);
extern void isr_irq0(void);
extern void isr_irq1(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_lo  = base & 0xFFFF;
    idt[num].base_hi  = (base >> 16) & 0xFFFF;
    idt[num].sel      = sel;
    idt[num].always0  = 0;
    idt[num].flags    = flags;
}

void idt_init(void)
{
    idt_ptr.base  = (uint32_t)&idt;
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, 0, 0x08, 0x8E);

    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);

    idt_set_gate(32, (uint32_t)isr_irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr_irq1, 0x08, 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

void isr_handler(uint32_t isr_num)
{
    (void)isr_num;
}

void irq_handler(uint32_t irq_num)
{
    switch (irq_num) {
        case 33:  /* IRQ1 - keyboard */
            keyboard_handler();
            break;
        default:
            break;
    }
}
