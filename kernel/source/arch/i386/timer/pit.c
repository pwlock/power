#include "pit.h"
#include "arch/i386/ports.h"
#include <stdint.h>

void pitDisable()
{
    outb(0x43, (0b11 << 4) | 1 << 1);
    outb(0x40, 0);
    outb(0x40, 0);
}
