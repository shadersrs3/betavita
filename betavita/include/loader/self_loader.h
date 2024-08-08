#ifndef _BETAVITA_SELF_LOADER
#define _BETAVITA_SELF_LOADER

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include <loader/ELF.h>

namespace betavita::loader {
struct DecryptedSegment {
    std::string name;
    std::shared_ptr<uint8_t> data;
    size_t segment_length;
    bool relocatable;
    bool can_load;
    ELFPhdr32 phdr;
};

struct SELFProgramData {
    uint32_t entry;
    bool relocatable;
    uint16_t executable_type;
    std::vector<DecryptedSegment> segments;
};

std::shared_ptr<SELFProgramData> decrypt_self_data(const uint8_t *in);
bool load_self_to_memory(const std::shared_ptr<SELFProgramData>& _data);
}

#endif