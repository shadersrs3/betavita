#ifndef _BETAVITA_MEMORY_MANAGER_H
#define _BETAVITA_MEMORY_MANAGER_H

#include <utility>
#include <cstdint>
#include <string>

namespace betavita::hle {
uint32_t allocate_stack(uint32_t size, const std::string& name = "");
uint32_t allocate_heap(uint32_t size, const std::string& name = "");
}

#endif /* _BETAVITA_MEMORY_MANAGER_H */