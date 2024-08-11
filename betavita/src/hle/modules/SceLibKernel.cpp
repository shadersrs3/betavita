#include <cstring>

#include <hle/modules/SceLibKernel.h>
#include <hle/wrapper.h>
#include <hle/kernel.h>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {
// Threads
int sceKernelCreateThread(uint32_t name_ptr, uint32_t entry, int initPriority, uint32_t stackSize, uint32_t attr, int cpuAffinityMask, uint32_t option) {
    return 0;
}

int sceKernelGetThreadId() {
    return kernel()->get_current_thread()->get_uid();
}

// Mutex
int sceKernelCreateMutex(uint32_t name_ptr, uint32_t attr, int initCount, uint32_t option) {
    auto *name = (const char *) memory::get_pointer_unchecked(name_ptr);

    if (!name) {
        LOG_ERROR(HLE, "Name is NULL");
        return -1;
    }

    LOG_ERROR(HLE, "Unimplemented sceKernelCreateMutex(%s, 0x%08x, 0x%08x, 0x%08x)", name, attr, initCount, option);
    return -1;
}

int sceKernelLockMutex(int mutexId, int lockCount, uint32_t timeout) {
    LOG_ERROR(HLE, "Unimplemented sceKernelLockMutex(0x%08x, %d, 0x%08x)", mutexId, lockCount, timeout);
    return -1;
}

int sceKernelUnlockMutex(int mutexId, int unlockCount) {
    LOG_ERROR(HLE, "Unimplemented sceKernelUnlockMutex(0x%08x, %d)", mutexId, unlockCount);
    return -1;
}

// LwMutex

// Sema

// Event flags

// TLS

int sceKernelGetTLSAddr(int key) {
    uint32_t tls_addr = kernel()->get_tls_addr(kernel()->get_current_thread()->get_uid(), key);
    return tls_addr;
}

// Clib
int sceClibMemset(uint32_t dst_addr, int ch, uint32_t len) {
    if (memory::is_valid_address_range(dst_addr, dst_addr + len - 1)) {
        LOG_HLE(HLE, "sceClibMemset(0x%08x, 0x%02x, 0x%08x)", dst_addr, ch, len);
        std::memset(memory::get_pointer_unchecked(dst_addr), ch, len);
    } else {
        LOG_ERROR(HLE, "memory error at sceClibMemset(0x%08x, 0x%02x, 0x%08x)", dst_addr, ch, len);
    }
    return dst_addr;
}

int sceClibMemmove(uint32_t dst_addr, uint32_t src_addr, uint32_t len) {
    bool valid_dst_address = memory::is_valid_address_range(dst_addr, dst_addr + len);
    bool valid_src_address = memory::is_valid_address_range(src_addr, src_addr + len);

    if (valid_dst_address && valid_src_address) {
        std::memmove(memory::get_pointer(dst_addr), memory::get_pointer(src_addr), len);
        LOG_HLE(HLE, "sceClibMemmove(0x%08x, 0x%08x, 0x%08x)", dst_addr, src_addr, len);
    } else {
        LOG_ERROR(HLE, "memory error at sceClibMemmove(0x%08x, 0x%08x, 0x%08x)", dst_addr, src_addr, len);
    }
    return dst_addr;
}

static HLEModule module_SceLibKernel = {
    "SceLibKernel", 0xCAE9ACE6,
    {
        // Threads

        // { "sceKernelCreateThread", 0xC5C11EE7, 0x0000, &wrap<sceKernelCreateThread> },
        { "sceKernelGetThreadId", 0x0FB972F9, 0x0000, &wrap<sceKernelGetThreadId> },

        // Mutex

        { "sceKernelCreateMutex", 0xED53334A, 0x0000, &wrap<sceKernelCreateMutex> },
        { "sceKernelLockMutex", 0x1D8D7945, 0x0000, &wrap<sceKernelLockMutex> },
        { "sceKernelUnlockMutex", 0x1A372EC8, 0x0000, &wrap<sceKernelUnlockMutex> },

        // LwMutex

        // Sema

        // Event flags

        // TLS

        { "sceKernelGetTLSAddr", 0xB295EB61, 0x0000, &wrap<sceKernelGetTLSAddr> },

        // Clib

        { "sceClibMemset", 0x632980D7, 0x0000, &wrap<sceClibMemset> },
        { "sceClibMemmove", 0x736753C8, 0x0000, &wrap<sceClibMemmove> }

        // TODO
    }
}; 

HLEModule *get_module_SceLibKernel() {
    return &module_SceLibKernel;
}
}