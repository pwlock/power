#include "state.h"
#include "arch/i386/ports.h"
#include "memory/physical.h"
#include "term/terminal.h"

void ps2SetDisabled(bool enb)
{
    if (enb) {
        outb(PS2_PORT_CMD, 0xAD);
        outb(PS2_PORT_CMD, 0xA7);
        return;
    }

    /* Enable */
    outb(PS2_PORT_CMD, 0xAE);
    outb(PS2_PORT_CMD, 0xA8);
}

struct ps2_state* ps2Init()
{
    struct ps2_state* ps = mmAlignedAlloc(sizeof(struct ps2_state), 1);
    uint8_t status;
    ps2SetDisabled(true);
    do {
        status = inb(PS2_PORT_STATUS);
        if (status & 0x1)
            inb(PS2_PORT_DATA);
    } while(status & 0x1);

    ps->DualPort = inb(0x20) & (1 << 5);

    /* ctl self test */
    outb(PS2_PORT_CMD, 0xAA);
    do {
        status = inb(PS2_PORT_DATA);
    } while(status != 0x55 && status != 0xFC);

    if (status == 0xFC) {
        trmLogfn("ps2: self-test failed");
        /* TODO add kernel panic */
    }

    outb(PS2_PORT_CMD, 0xAB);
    do {
        status = inb(PS2_PORT_DATA);
    } while(status > 0x05);
    if (status)
        trmLogfn("ps2: first port self-test failed");

    if (ps->DualPort) {
        outb(PS2_PORT_CMD, 0xA9);
        do {
            status = inb(PS2_PORT_DATA);
        } while(status > 0x05);
        if (status)
            trmLogfn("ps2: second port self-test failed");
    }

    ps2SetDisabled(false);
    ps2SendDeviceCommand(ps, false, 0xFF);
    ps2SendDeviceCommand(ps, true, 0xFF);

    return ps;
}

void ps2EnableConfig(struct ps2_state* ps2, int cfg)
{
    uint8_t status;
    uint8_t cfb;
    if (!ps2->DualPort) {
        cfg &= ~(PS2_CONFIG_P2_INT);
    }

    do {
        status = inb(PS2_PORT_STATUS);
        inb(PS2_PORT_DATA);
    } while(status & 1);

    outb(PS2_PORT_CMD, 0x20);
    cfb = inb(PS2_PORT_DATA);
    cfb |= cfg;
    cfb &= ~(1 << 6); /* disable translation */
    outb(PS2_PORT_CMD, 0x60);

    do {
        status = inb(PS2_PORT_STATUS);
    } while(status & (1 << 1));

    outb(PS2_PORT_DATA, cfb);
}

void ps2SendDeviceCommand(struct ps2_state* ps2, int secondary, uint8_t command)
{
    if (secondary && !ps2->DualPort) {
        return;
    }

    if (secondary) {
        outb(PS2_PORT_CMD, 0xD4);
    }

    uint8_t status;
    do {
        status = inb(PS2_PORT_STATUS);
    } while (status & (1 << 1));
    outb(PS2_PORT_DATA, command);
}

void ps2SendDeviceCommandEx(struct ps2_state* ps2, int secondary, 
                            uint8_t command, uint8_t data)
{
    if (secondary && !ps2->DualPort) {
        return;
    }

    if (secondary) {
        outb(PS2_PORT_CMD, 0xD4);
    }

    uint8_t status;
    do {
        status = inb(PS2_PORT_STATUS);
    } while (status & (1 << 1));
    outb(PS2_PORT_DATA, command);

    do {
        status = inb(PS2_PORT_STATUS);
    } while (status & (1 << 1));
    outb(PS2_PORT_DATA, data);
}
