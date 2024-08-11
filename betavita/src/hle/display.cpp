#include <cstdio>
#include <cstring>

#include <hle/display.h>

namespace betavita::hle {
void Display::set_framebuffer(const SceDisplayFramebuf *framebuffer, uint32_t sync_type) {
    this->sync_type = sync_type;
    if (framebuffer) {
        std::memcpy(&framebuffer_params, framebuffer, sizeof *framebuffer);
    } else {
        framebuffer_params.framebuffer_base = 0x1EADBEEF;
    }
}

SceDisplayFramebuf& Display::get_framebuffer() {
    return framebuffer_params;
}
}