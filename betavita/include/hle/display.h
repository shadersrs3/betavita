#ifndef _BETAVITA_DISPLAY_H
#define _BETAVITA_DISPLAY_H

#include <cstdint>

namespace betavita::hle {
struct SceDisplayFramebuf {
    uint32_t size;
    uint32_t framebuffer_base;
    uint32_t pitch;
    uint32_t framebuffer_pixelformat;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
};

struct Display {
private:
    SceDisplayFramebuf framebuffer_params;
    uint32_t sync_type;
public:
    void set_framebuffer(const SceDisplayFramebuf *framebuffer, uint32_t sync_type);
    SceDisplayFramebuf& get_framebuffer();
};
}

#endif