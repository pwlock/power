#include "intnotify.h"
#include "arch/i386/ports.h"
#include "power/input_code.h"
#include "power/input.h"
#include "state.h"
#include "term/terminal.h"
#include "um/input.h"
#include <stdint.h>

/* Common (non-extended) keycodes */
static uint32_t commonkeycode[256] = {
        KEY_UNKNOWN, 
    KEY_F9, 
        KEY_UNKNOWN, 
    KEY_F5, KEY_F3, KEY_F1, KEY_F2, KEY_F12, 
        KEY_UNKNOWN,
    KEY_F10, KEY_F8, KEY_F6, KEY_F4, KEY_TAB, KEY_BACKTICK, 
        KEY_UNKNOWN, KEY_UNKNOWN, 
    KEY_LALT, KEY_LSHIFT, 
        KEY_UNKNOWN, 
    KEY_LCTRL, KEY_Q, KEY_1, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_Z, KEY_S, KEY_A, KEY_W, KEY_2, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_C, KEY_X, KEY_D, KEY_E, KEY_4, KEY_3, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_SPACE, KEY_V, KEY_F, KEY_T, KEY_R, KEY_5, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_N, KEY_B, KEY_H, KEY_G, KEY_Y, KEY_6, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_M, KEY_J, KEY_U, KEY_7, KEY_8, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_COMMA, KEY_K, KEY_I, KEY_O, KEY_0, KEY_9, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_DOT, KEY_FSLASH, KEY_L, KEY_SEMICOLON, KEY_P, KEY_HYPHEN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_APOSTOPHR, 
        KEY_UNKNOWN,
    KEY_LEFT_BR, KEY_EQUAL, 
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_CAPSLOCK, KEY_RSHIFT, KEY_RETURN, KEY_LEFT_BR, 
        KEY_UNKNOWN,
    KEY_SLASH, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_BACKSPACE, KEY_UNKNOWN, 
        KEY_UNKNOWN,
    KEY_1_NUMP, 
        KEY_UNKNOWN,
    KEY_4_NUMP, KEY_7_NUMP, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_0_NUMP, KEY_DOT_NUMP, KEY_2_NUMP, KEY_5_NUMP, KEY_6_NUMP, KEY_8_NUMP, KEY_ESCAPE,
    KEY_NUMLOCK, KEY_F11, KEY_PLUS_NUMP, KEY_3_NUMP, KEY_HYPHEN_NUMP, 
    KEY_ASTERISK_NUMP, KEY_9_NUMP, KEY_SCROLLOCK, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_F7,

        /* Padding to extended codes */
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,

    /* Extended key codes */
        KEY_UNKNOWN, 
    KEY_RALT, 
        KEY_UNKNOWN, KEY_UNKNOWN, 
    KEY_RCTRL,
        /* There is a bunch of multimedia keys spread out on this block.
           Should we support them? */
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_FSLASH_NUMP,
        /* More multimedia/power buttons here... */
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_RETURN_NUMP, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, 
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_END,
        KEY_UNKNOWN,
    KEY_ARROW_LEFT, KEY_HOME,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_INSERT, KEY_DELETE, KEY_ARROW_DOWN,
        KEY_UNKNOWN,
    KEY_ARROW_RIGHT, KEY_ARROW_UP,
        KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_PGDWN,
        KEY_UNKNOWN, KEY_UNKNOWN,
    KEY_PGUP
};

static inline bool isSpecial(uint8_t byte)
{
    return byte == 0x00
        || byte == 0xAA
        || byte == 0xEE
        || byte == 0xFA
        || byte == 0xFC
        || byte == 0xFD
        || byte == 0xFE
        || byte == 0xFF;
}

void kbdInterrupt(void* p)
{
    struct ps2_state* ps2 = (struct ps2_state*)p;
    uint8_t data = inb(PS2_PORT_DATA);

    if (isSpecial(data))
        return;

    switch(data) {
    case 0xF0:
        ps2->Release = true;
        return;
    case 0xE0:
        ps2->MultipleRemain = 1;
        return;
    case 0xE1:
        ps2->MultipleRemain = 2;
        return;
    }

    int addend = 0;
    if (ps2->MultipleRemain == 1) {
        addend = 0x80;
    }

    uint32_t kc = commonkeycode[data | addend];

    int type = EV_INPUT_KEYPRESS;
    if (ps2->Release)
        type = EV_INPUT_KEYRELEASE;
    inpSendInputEvent(type, kc, 0);

    if (ps2->MultipleRemain > 0)
        ps2->MultipleRemain--;
    ps2->Release = false;
}

void mouInterrupt(void* p)
{

}

