#include <hle/modules/SceDisplayUser.h>
#include <hle/wrapper.h>
#include <hle/display.h>
#include <hle/kernel.h>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {

int sceDisplaySetFramebuf(uint32_t pParam, uint32_t sync) {
    kernel()->get_display()->set_framebuffer((SceDisplayFramebuf *) memory::get_pointer_unchecked(pParam), sync);

    LOG_HLE(HLE, "sceDisplaySetFramebuf(0x%08x, 0x%08x)", pParam, sync);
    return 0;
}

HLEModule module_SceDisplayUser = {
    "SceDisplayUser", 0x4FAACD11,
    {
        { "sceDisplaySetFramebuf", 0x7A410B64, 0x0000, &wrap<sceDisplaySetFramebuf> }
    }
};

HLEModule *get_module_SceDisplayUser() {
    return &module_SceDisplayUser;
}
}