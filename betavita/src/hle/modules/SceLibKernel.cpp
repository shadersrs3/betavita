#include <hle/modules/SceLibKernel.h>
#include <hle/wrapper.h>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {
int sceKernelCreateThread(uint32_t name_ptr, uint32_t entry, int initPriority, uint32_t stackSize, uint32_t attr, int cpuAffinityMask, uint32_t option) {
    return 0;
}

int sceKernelCreateMutex(uint32_t name_ptr, uint32_t attr, int initCount, uint32_t option) {
    auto *name = (const char *) memory::get_pointer_unchecked(name_ptr);

    if (!name) {
        LOG_ERROR(HLE, "Name is NULL");
        return -1;
    }

    LOG_ERROR(HLE, "Unimplemented sceKernelCreateMutex(%s, 0x%08x, 0x%08x, 0x%08x)", name, attr, initCount, option);
    return -1;
}

static HLEModule module_SceLibKernel = {
    "SceLibKernel", 0xCAE9ACE6,
    {
        // Threads
        { "sceKernelCreateThread", 0xC5C11EE7, 0x1000, &wrap<sceKernelCreateThread> },

        // Mutex

        { "sceKernelCreateMutex", 0xED53334A, 0x1000, &wrap<sceKernelCreateMutex> },

        // LwMutex

        // Sema

        // Event flags

        // TLS

        // TODO
    }
}; 

HLEModule *get_module_SceLibKernel() {
    return &module_SceLibKernel;
}
}