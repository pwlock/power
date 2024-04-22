#include "terminal.h"
#include "bootloader_requests.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "utils/string.h"

#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include "../ssfn.h"

extern unsigned char* terminalFont;

static inline bool handleNewLine(char c) 
{
    if (c == '\n') {
        ssfn_dst.x = 0; 
        ssfn_dst.y += ssfn_src->height;
        return false;
    }

    return true;
}

static inline void reverseString(char* str)
{
	int len;
	int index;
	char* start, *end, temp;

	len = strlen(str);

	start = str;
	end = str+len-1;

    for (index = 0; index < len / 2; index++) { 
		temp = *end;
		*end = *start;
		*start = temp;

		start++; 
		end--;
	}
}

static inline char* numberToString(uint64_t number, char* str, int base)
{
    int i = 0;
    bool neg = false;

    if (number == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    while (number != 0) {
        int rem = number % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        number = number / base;
    }

    if (neg)
        str[i++] = '-';
    str[i] = '\0';
    reverseString(str);
    return str;
}

static void logfv(const char* msg, va_list vl)
{
    while(*msg != 0) {
        if (!handleNewLine(*msg))
            continue;

        if (*msg == '%') {
            msg++;
            switch (*msg) {
            case 's': 
            {
                const char* s = va_arg(vl, char*);
                trmLog(s);
                break;
            }

            case 'i':
            {
                char str[20];
                uint64_t i = va_arg(vl, uint64_t);
                numberToString(i, str, 10);
                trmLog(str);
                break;
            }

            case 'x':
            case 'p':
            {
                char str[20];
                uint64_t i = va_arg(vl, uint64_t);
                numberToString(i, str, 16);

                if (*msg == 'p')
                    trmLog("0x");
                trmLog(str);
                break;
            }

            case 'c':
            {
                char i = va_arg(vl, int);
                ssfn_putc(i);
                break;
            }

            case '%':
                ssfn_putc('%');
                break;

            }
            msg++;
            continue;
        }

        ssfn_putc(*msg);
        msg++;
    }

}

void trmInit() 
{
    ssfn_src = (ssfn_font_t*)&terminalFont;
    
    struct limine_framebuffer_response* r = rqGetFramebufferRequest();
    struct limine_framebuffer* fb = r->framebuffers[0];
    ssfn_dst.ptr = fb->address;
    ssfn_dst.p = fb->pitch;
    ssfn_dst.w = fb->width;
    ssfn_dst.h = fb->height;
    ssfn_dst.x = 0;
    ssfn_dst.y = 0;
    ssfn_dst.fg = 0x0DDB14;
}

void trmLog(const char* msg) 
{
    while(*msg != 0) {
        if (!handleNewLine(*msg))
            continue;

        ssfn_putc(*msg);
        ++msg;
    }
}

void trmLogf(const char* msg, ...)
{
    va_list vl;
    va_start(vl, msg);
    logfv(msg, vl);
    va_end(vl);
}

void trmLogfn(const char *msg, ...)
{
    va_list vl;
    va_start(vl, msg);
    logfv(msg, vl);
    va_end(vl);

    ssfn_dst.x = 0;
    ssfn_dst.y += ssfn_src->height;
}
