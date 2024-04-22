/* bootloader_requests.h
   Purpose: Get Limine requests */
#pragma once

#include "limine.h"

struct limine_framebuffer_response* rqGetFramebufferRequest();
struct limine_memmap_response* rqGetMemoryMapRequest();
struct limine_hhdm_response* rqGetHhdmRequest();
struct limine_kernel_address_response* rqGetKernelAddressRequest();
struct limine_rsdp_response* rqGetRsdpResponse();
struct limine_module_response* rqGetModuleResponse();
struct limine_kernel_file_response* rqGetKernelFileResponse();