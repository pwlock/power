#include "bootloader_requests.h"
#include "limine.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic push

static volatile LIMINE_BASE_REVISION(1);

#pragma GCC diagnostic pop

static volatile struct limine_framebuffer_request _framebuffer = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 1
};

static volatile struct limine_memmap_request _mmap = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request _hhdm = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_address_request _ke = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static volatile struct limine_rsdp_request _rsdp = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile struct limine_module_request _mod = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_file_request _kef = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0
};


struct limine_framebuffer_response* rqGetFramebufferRequest()
{
    return _framebuffer.response;
}

struct limine_memmap_response* rqGetMemoryMapRequest()
{
    return _mmap.response;
}

struct limine_hhdm_response* rqGetHhdmRequest()
{
    return _hhdm.response;
}

struct limine_kernel_address_response* rqGetKernelAddressRequest()
{
    return _ke.response;
}

struct limine_rsdp_response* rqGetRsdpResponse()
{
    return _rsdp.response;
}

struct limine_module_response* rqGetModuleResponse()
{
    return _mod.response;
}

struct limine_kernel_file_response* rqGetKernelFileResponse()
{
    return _kef.response;
}
