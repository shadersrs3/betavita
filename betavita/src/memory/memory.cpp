#include <cstring>
#include <vector>
#include <algorithm>

#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::memory {
static std::vector<MemoryMap> memory_map_list;

void map_memory(const std::string& name, uint32_t start, uint32_t end, uint8_t protection, MemoryMap **memory_map) {
    MemoryMap map;
    map.name = name;
    map.start = start;
    map.end = end;
    map.size = (end - start) + 1;

    std::shared_ptr<uint8_t> data(std::shared_ptr<uint8_t>(new uint8_t[map.size]));
    std::memset(data.get(), 0, map.size);
    map.data = std::move(data);
    map.fast_access = map.data.get();
    map.protection = (ProtectionType) protection;

    memory_map_list.push_back(map);

    if (memory_map)
        *memory_map = &memory_map_list.back();

    std::sort(memory_map_list.begin(), memory_map_list.end(), [](const MemoryMap &a, const MemoryMap& b) { return a.end < b.end; });

    map_memory_from_api(start, end, map.fast_access, map.protection);
    LOG_INFO(MEMORY, "Mapped memory %s 0x%08X ... 0x%08X size 0x%08X protection flags 0x%01x", name.c_str(), map.start, map.end, map.size, protection);
}

void unmap_memory(uint32_t start, uint32_t end) {
    unmap_memory_from_api(start, end);
}

void unmap_memory_all() {
    for (auto& i : memory_map_list) {
        unmap_memory(i.start, i.end);
    }

    memory_map_list.clear();
    LOG_INFO(MEMORY, "Unmapped all memory regions");
}

std::vector<MemoryMap>& get_memory_map_list() {
    return memory_map_list;
}

void *get_pointer_unchecked(uint32_t address) {
    for (std::vector<MemoryMap>::iterator it = memory_map_list.begin();
        it != memory_map_list.end(); it++) {
        if (address >= it->start && address <= it->end) {
            return it->fast_access + (address - it->start);
        }
    }
    return nullptr;
}

void *get_pointer(uint32_t address) {
    void *data = get_pointer_unchecked(address);
    if (!data) {
        LOG_ERROR(MEMORY, "Unmapped accessed address 0x%08x\n", address);
    }
    return data;
}

void memcpy(uint32_t dst, uint32_t src, uint32_t size) {

}

void memcpy(uint32_t dst, const void *src, uint32_t size) {
    void *data = get_pointer(dst);

    if (!data) {
        LOG_ERROR(MEMORY, "Can't memory copy to 0x%08x size 0x%08x", dst, size);
        return;
    }

    std::memcpy(data, src, size);
}

void memcpy(void *dst, uint32_t addr, uint32_t size) {

}
}