#pragma once
#include "../include/types.h"

void pic_init(void);
void pic_eoi(uint8_t irq);
void pic_disable_irq(uint8_t irq);
void pic_enable_irq(uint8_t irq);
