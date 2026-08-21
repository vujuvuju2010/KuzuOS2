#ifndef IRQ_H
#define IRQ_H

#include "interrupts.h"

// IRQ fonksiyonları
void irq_init();
void irq_handler(struct regs* r);

#endif 