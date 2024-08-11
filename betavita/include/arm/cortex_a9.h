#ifndef _BETAVITA_CORTEX_A9_H
#define _BETAVITA_CORTEX_A9_H

#include <cstdint>

namespace betavita::processor {
#pragma pack(push, 1)
/*
// This processor is emulating userspace registers only
union PSR {
    struct {
        uint8_t current_mode : 5;
        uint8_t thumb : 1;
        uint8_t fiq : 1;
        uint8_t irq : 1;
        uint8_t async_abort : 1;
        uint8_t endianness : 1;
        uint8_t it_7_2 : 6;
        uint8_t ge : 4;
        uint8_t raz_sbzp : 4;
        uint8_t jazelle : 1;
        uint8_t it_1_0 : 2;
        uint8_t q : 1;
        uint8_t v : 1;
        uint8_t c : 1;
        uint8_t z : 1;
        uint8_t n : 1;
    };
    uint32_t data;
};

struct CortexA9 {
private:
    uint32_t reg[16];
    PSR cpsr;
public:
    CortexA9();

};
*/

struct RegisterContext {
    uint32_t reg[16];
    uint32_t cpsr;
};

void init_cpu();
uint32_t read_register_from_api(int core_num, int regnum);
void write_register_from_api(int core_num, int regnum, uint32_t value);
void stop_cpu_from_api(int core_num);
int get_available_core();
void run_cpu_for(int core_num, RegisterContext *context, uint64_t instructions_count);

#pragma pack(pop)
}

#endif /* _BETAVITA_CORTEX_A9_H */