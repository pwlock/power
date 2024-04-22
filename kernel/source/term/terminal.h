/* terminal.h
   Purpose: functions for writing text onto the screen */
#pragma once

void trmInit();

void trmLog(const char* msg);
void trmLogf(const char* msg, ...);
void trmLogfn(const char* msg, ...);