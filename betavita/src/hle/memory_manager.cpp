#include <hle/memory_manager.h>
#include <memory/memory.h>

#include <logger/logger.h>

namespace betavita::hle {
static std::pair<uint32_t, uint32_t> more_memory(uint32_t size) {
    auto memlist = memory::get_memory_map_list();
    uint32_t top_address = 0x0, memory_size = (size + 0x1000) & ~0xFFF;

    if (memlist.size() == 0)
        return { 0x60000000, (size + 0x1000) & ~0xFFF };

    for (auto it = memlist.begin(); it != memlist.end(); it++) {
        memory::MemoryMap *start_map = &*it, *end_map;

        if (top_address < start_map->end) {
            top_address = (start_map->end + 1 + 0x1000) & ~0xFFF;
        }

        end_map = it + 1 != memlist.end() ? &*(it + 1) : nullptr;

        if (end_map) {
            uint32_t new_paged_addr = (start_map->end + 1 + 0x1000) & ~0xFFF;
            int empty_paged_block = (end_map->start - 0x1000) - new_paged_addr - memory_size;
            if (empty_paged_block >= 0) { // a gap is available
                top_address = new_paged_addr;
                break;
            }
        }
    }

    if (top_address) {
        return { top_address, memory_size };
    }

    return { -1, -1 };
}

uint32_t allocate_stack(uint32_t size, const std::string& name) {
    auto [address, memory_size] = more_memory(size);

    if (address == 0xFFFFFFFF) {
        LOG_ERROR(HLE, "can't allocate stack for %s", name.c_str());
        return 0;
    }

    memory::map_memory(name + "_stack", address, address + memory_size - 1, memory::PROTECTION_TYPE_READ | memory::PROTECTION_TYPE_WRITE);
    return address + memory_size;
}

uint32_t allocate_heap(uint32_t size, const std::string& name) {
    auto [address, memory_size] = more_memory(size);

    if (address == 0xFFFFFFFF) {
        LOG_ERROR(HLE, "can't allocate heap for %s", name.c_str());
        return 0;
    }

    memory::map_memory(name + "_heap", address, address + memory_size - 1, memory::PROTECTION_TYPE_READ | memory::PROTECTION_TYPE_WRITE);
    return address;
}
}