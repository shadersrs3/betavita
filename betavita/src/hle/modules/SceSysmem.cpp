#include <hle/modules/SceSysmem.h>
#include <hle/wrapper.h>
#include <hle/kernel.h>
#include <hle/memory_manager.h>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {
uint32_t fake_address;

int sceKernelAllocMemBlock(uint32_t name_ptr, uint32_t type, uint32_t size, uint32_t opt) {
    auto *name = (const char *) memory::get_pointer_unchecked(name_ptr);

    if (!name)
        LOG_ERROR(HLE, "Name is NULL");

    LOG_ERROR(HLE, "unimplemented sceKernelAllocMemBlock(%s, 0x%08x, 0x%08x, 0x%08x)!", name, type, size, opt);
    fake_address = hle::allocate_heap(size, std::string(name) + "_memblock");
    return 2;
}

int sceKernelGetMemBlockBase(int uid, uint32_t base_addr) {
    auto *base = (uint32_t *) memory::get_pointer_unchecked(base_addr);

    if (base) {
        *base = fake_address;
    }

    LOG_HLE(HLE, "sceKernelGetMemBlockBase(0x%08x, 0x%08x)", uid, base_addr);
    //std::exit(0);
    return 0;
}

static HLEModule module_SceSysmem = {
    "SceSysmem", 0x37FE725A,
    {
        { "sceKernelAllocMemBlock", 0xB9D5EBDE, 0x0000, &wrap<sceKernelAllocMemBlock> },
        { "sceKernelGetMemBlockBase", 0xB8EF5818, 0x0000, &wrap<sceKernelGetMemBlockBase> }
    }
};

HLEModule *get_module_SceSysmem() {
    return &module_SceSysmem;
}
}