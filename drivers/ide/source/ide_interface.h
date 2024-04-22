/* ide_interface.h
   Purpose: Registering IDE functions inside
            Driver API "vtable" */
#pragma once

#include "ide_state.h"

void registerIdeInterface(struct ide* ide);
void registerIdeDeviceInterface(struct ide_channel* device);
