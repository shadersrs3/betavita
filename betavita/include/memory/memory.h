#ifndef _BETAVITA_MEMORY_H
#define _BETAVITA_MEMORY_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace betavita::memory {
enum ProtectionType : uint8_t {
    PROTECTION_TYPE_READ = (1 << 0), PROTECTION_TYPE_WRITE = (1 << 1), PROTECTION_TYPE_EXECUTE = (1 << 2)
};

struct MemoryMap {
    std::string name;
    uint32_t start, end;
    uint32_t size;
    ProtectionType protection;
    std::shared_ptr<uint8_t> data;
    uint8_t *fast_access;
};

void map_memory(const std::string& name, uint32_t start, uint32_t end, uint8_t protection);
void unmap_memory(uint32_t start, uint32_t end);
void unmap_memory_all();
std::vector<MemoryMap>& get_memory_map_list();
void *get_pointer_unchecked(uint32_t address);
void *get_pointer(uint32_t address);
void memcpy(uint32_t dst, uint32_t src, uint32_t size);
void memcpy(uint32_t dst, const void *src, uint32_t size);
void memcpy(void *dst, uint32_t addr, uint32_t size);
}

#endif /* _BETAVITA_MEMORY_H */