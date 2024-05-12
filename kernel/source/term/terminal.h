/* terminal.h
   Purpose: functions for writing text onto the screen */
#pragma once

void trmInit();
void trmSetFramebuffer(void* framebuffer, int h, int w, int stride);

void trmLog(const char* msg);
void trmLogf(const char* msg, ...);
void trmLogfn(const char* msg, ...);