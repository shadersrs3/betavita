#ifndef _BETAVITA_HLE_WRAPPER
#define _BETAVITA_HLE_WRAPPER

#include <arm/cortex_a9.h>
#include <hle/kernel.h>

namespace betavita::hle {
template<int f()>
inline void wrap() {
    uint32_t retval;

    retval = (uint32_t) f();
    processor::write_register_from_api(0, 0, retval);
}

template<int f(int)>
inline void wrap() {
    uint32_t r;
    uint32_t retval;

    r = processor::read_register_from_api(0, 0);

    retval = (uint32_t) f(r);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(int, int)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);

    retval = f(r[0], r[1]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(int, uint32_t)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);

    retval = f(r[0], r[1]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(uint32_t, uint32_t)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);

    retval = f(r[0], r[1]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(int, int, uint32_t)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);
    r[2] = processor::read_register_from_api(0, 2);

    retval = f(r[0], r[1], r[2]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(uint32_t, int, uint32_t)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);
    r[2] = processor::read_register_from_api(0, 2);

    retval = f(r[0], r[1], r[2]);

    processor::write_register_from_api(0, 0, retval);
}

template<int f(uint32_t, uint32_t, uint32_t)>
inline void wrap() {
    uint32_t r[3];
    uint32_t retval;

    r[0] = processor::read_register_from_api(0, 0);
    r[1] = processor::read_register_from_api(0, 1);
    r[2] = processor::read_register_from_api(0, 2);

    retval = f(r[0], r[1], r[2]);

    processor::write_register_from_api(0, 0, retval);
}

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

template<int f(uint32_t, uint32_t, int, uint32_t, uint32_t, int, uint32_t)>
inline void wrap() {
    printf("UNIMPLEMENTED WRAP!\n");
}
}

#define WRAP_FUNCTION(x) ((void *) wrap<x>)

#endif