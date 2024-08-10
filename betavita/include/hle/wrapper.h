#ifndef _BETAVITA_HLE_WRAPPER
#define _BETAVITA_HLE_WRAPPER

#include <arm/cortexa9.h>

namespace betavita::hle {
template<int f(uint32_t, uint32_t, int, uint32_t)>
inline void wrap() {
    uint32_t r[4];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);
    r[2] = processor::read_register_from_api(0, 2);
    r[3] = processor::read_register_from_api(0, 3);

    retval = f(r[0], r[1], r[2], r[3]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(uint32_t, uint32_t, uint32_t, uint32_t)>
inline void wrap() {
    uint32_t r[4];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);
    r[2] = processor::read_register_from_api(0, 2);
    r[3] = processor::read_register_from_api(0, 3);

    retval = f(r[0], r[1], r[2], r[3]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(uint32_t name_ptr, uint32_t entry, int initPriority, uint32_t stackSize, uint32_t attr, int cpuAffinityMask, uint32_t option)>
inline void wrap() {
    printf("UNIMPLEMENTED WRAP!\n");
}
}

#define WRAP_FUNCTION(x) ((void *) wrap<x>)

#endif