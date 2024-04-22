/* state.h
   Purpose: PS/2 state management */
#pragma once

#include "config.h"

#define PS2_PORT_DATA 0x60
#define PS2_PORT_STATUS 0x64 /*< Read */
#define PS2_PORT_CMD    0x64 /*< Write */

#define PS2_CONFIG_TRANSLATION (1 << 6)
#define PS2_CONFIG_P2_INT      (1 << 1)
#define PS2_CONFIG_P1_INT      (1 << 0)

#define PS2_KEY_STATE_NORMAL 1
#define PS2_KEY_STATE_MULTIPLE_KEY_ONE 2 /*< Expect ONE more byte */
#define PS2_KEY_STATE_MULTIPLE_KEY_TWO 3 /*< Expect two more bytes */

struct ps2_state
{
    uint64_t CurrentKeyForm;
    int MultipleRemain;
    int NeedsSpecialByte;
    int CkOffset;
    int KeyState;
    bool Release;
    bool DualPort;
};

struct ps2_state* ps2Init();
void ps2SetDisabled(bool enb);
void ps2SendDeviceCommand(struct ps2_state*, 
                          int secondary, uint8_t command);
void ps2FlushOutputBuffer();
void ps2EnableConfig(struct ps2_state*, int cfg);
void ps2SendDeviceCommandEx(struct ps2_state* ps2, int secondary, 
                            uint8_t command, uint8_t data);