/* input_code.h
   Purpose: Input event codes */
#pragma once

/* Inherited (copied) from old Lockdown.
  Yeah, reserved should be indicated by 0.  Blame past me. */
#define KEY_UNKNOWN 0xFFFFFF

#ifndef KEY_NO_POLLUTE

#ifndef _POWER_INPUT_CODE_H
#define _POWER_INPUT_CODE_H

#define KEY_ESCAPE 0x0
#define KEY_F1     0x1
#define KEY_F2     0x2
#define KEY_F3     0x3
#define KEY_F4     0x4
#define KEY_F5     0x5
#define KEY_F6     0x6
#define KEY_F7     0x7
#define KEY_F8     0x8
#define KEY_F9     0x9
#define KEY_F10    0xa
#define KEY_F11    0xb
#define KEY_F12    0xc
#define KEY_TILDE   0x10
#define KEY_1       0x11
#define KEY_2       0x12
#define KEY_3       0x13
#define KEY_4       0x14
#define KEY_5       0x15
#define KEY_6       0x16
#define KEY_7       0x17
#define KEY_8       0x18
#define KEY_9       0x19
#define KEY_0       0x1a
#define KEY_HYPHEN  0x1b
#define KEY_EQUAL   0x1c
#define KEY_SLASH   0x1e
#define KEY_BACKSPACE 0x1f
#define KEY_TAB       0x23
#define KEY_Q         0x24
#define KEY_W         0x25
#define KEY_E         0x26
#define KEY_R         0x27
#define KEY_T         0x28
#define KEY_Y         0x29
#define KEY_U         0x2a
#define KEY_I         0x2b
#define KEY_O         0x2c
#define KEY_P         0x2d
#define KEY_LEFT_BR   0x2e
#define KEY_RIGHT_BR  0x2f
#define KEY_CAPSLOCK  0x30
#define KEY_A         0x31
#define KEY_S         0x32
#define KEY_D         0x33
#define KEY_F         0x34
#define KEY_G         0x35
#define KEY_H         0x36
#define KEY_J         0x37
#define KEY_K         0x38
#define KEY_L         0x39
#define KEY_SEMICOLON 0x3a
#define KEY_APOSTOPHR 0x3b
#define KEY_RETURN    0x3c
#define KEY_LSHIFT    0x3d
#define KEY_Z         0x3e
#define KEY_X         0x3f
#define KEY_C         0x40
#define KEY_V         0x41
#define KEY_B         0x42
#define KEY_N         0x43
#define KEY_M         0x44
#define KEY_COMMA     0x45
#define KEY_DOT       0x46
#define KEY_FSLASH    0x47
#define KEY_RSHIFT    0x48
#define KEY_LCTRL     0x49
#define KEY_RCTRL     0x4a
#define KEY_LSYSTEM   0x4b /* Also known as left super or windows key */
#define KEY_LALT      0x4c
#define KEY_SPACE     0x4d
#define KEY_ALTGR     0x4e /* KEY_RALT is also a synonym */
#define KEY_RSYSTEM   0x4f /* Also known as right super or windows key */
#define KEY_APP       0x50 /* Also known as menu key */

/* Cursor control (arrow) keys */
#define KEY_ARROW_LEFT 0x51
#define KEY_ARROW_DOWN 0x52
#define KEY_ARROW_UP   0x53
#define KEY_ARROW_RIGHT 0x54

/* Keys only present in 100% keyboards. I guess... */
#define KEY_PRINTSCREEN 0x55
#define KEY_SCROLLOCK  0x56
#define KEY_PAUSE_BREAK 0x57
#define KEY_INSERT      0x58
#define KEY_HOME        0x59
#define KEY_PGUP        0x5a
#define KEY_DELETE      0x5b
#define KEY_END         0x5c
#define KEY_PGDWN       0x5d

/* Numeric pad keys */
#define KEY_NUMLOCK     0x5e
#define KEY_FSLASH_NUMP 0x5f
#define KEY_ASTERISK_NUMP 0x60
#define KEY_HYPHEN_NUMP   0x61
#define KEY_7_NUMP        0x62
#define KEY_8_NUMP        0x63
#define KEY_9_NUMP        0x64
#define KEY_PLUS          0x65
#define KEY_4_NUMP        0x66
#define KEY_5_NUMP        0x67
#define KEY_6_NUMP        0x68
#define KEY_1_NUMP        0x69
#define KEY_2_NUMP        0x6a
#define KEY_3_NUMP        0x6b
#define KEY_RETURN_NUMP   0x6c
#define KEY_0_NUMP        0x6d
#define KEY_DELETE_NUMP   0x6e
#define KEY_DOT_NUMP      0x6f
#define KEY_PLUS_NUMP     0x70

#define KEY_BACKTICK 0x71 /*< added much later. */

/* For consistency. */
#define KEY_RALT KEY_ALTGR

#endif /* ifndef _POWER_INPUT_CODE_H */

#endif /* !defined(KEY_NO_POLLUTE) */
