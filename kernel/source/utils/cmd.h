/* cmd.h
   Purpose: command line handling */
#pragma once

#include "config.h"

size_t cmdGetCommandArgument(const char* name, const char** output);
bool cmdHasArgument(const char* name);
