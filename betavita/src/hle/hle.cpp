#include <unordered_map>

#include <hle/hle.h>

#include <logger/logger.h>

namespace betavita::hle {
static std::unordered_map<uint32_t, RuntimeModule> runtime_module_list;

void add_runtime_module(const RuntimeModule& runtime_module) {
    runtime_module_list[runtime_module.module_nid] = runtime_module;
    LOG_DEBUG(HLE, "added runtime module (0x%08x) %s", runtime_module.module_nid, runtime_module.library_name.c_str());
}

static std::unordered_map<uint32_t, SvcNid> nid_to_address_map;

void resolve_runtime_modules() {
    for (auto& i : runtime_module_list) {
        LOG_DEBUG(HLE, "resolving module 0x%08x (%s)", i.second.module_nid, i.second.library_name.c_str());
        for (auto& j : i.second.function_import_libraries) {
            LOG_DEBUG(HLE, "resolving import library 0x%08x (%s)", j.library_nid, j.library_name.c_str());
            SvcNid svcnid;

            svcnid.library_name_ptr = &j.library_name;
            for (auto& k : j.function_imports) {
                svcnid.nid = k.nid;
                nid_to_address_map[k.entry_address] = svcnid;
            }
        }
    }

    LOG_DEBUG(HLE, "resolved HLE functions");
}

SvcNid *get_nid_from_address(uint32_t address) {
    auto it = nid_to_address_map.find(address);
    if (it != nid_to_address_map.end())
        return &it->second;
    return nullptr;
}
}