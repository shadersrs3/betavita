#include <hle/modules/SceSysmem.h>

#include <hle/wrapper.h>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {

int sceKernelAllocMemBlock(uint32_t name_ptr, uint32_t type, uint32_t size, uint32_t opt) {
    auto *name = (const char *) memory::get_pointer_unchecked(name_ptr);

    if (!name)
        LOG_ERROR(HLE, "Name is NULL");

    LOG_ERROR(HLE, "unimplemented sceKernelAllocMemBlock(%s, 0x%08x, 0x%08x, 0x%08x)!", name, type, size, opt);
    return -1;
}

static HLEModule module_SceSysmem = {
    "SceSysmem", 0x37FE725A,
    {
        { "sceKernelAllocMemBlock", 0xB9D5EBDE, 0x1500, wrap<sceKernelAllocMemBlock> },
    } 
};

HLEModule *get_module_SceSysmem() {
    return &module_SceSysmem;
}
}