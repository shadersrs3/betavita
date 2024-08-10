#include <unordered_map>

#include <hle/hle.h>

#include <hle/modules/SceLibKernel.h>
#include <hle/modules/SceSysmem.h>

#include <logger/logger.h>

namespace betavita::hle {
static std::vector<RuntimeModule> runtime_module_list;
static std::unordered_map<uint32_t, HLEFunction *> runtime_function_map;
static std::vector<HLEModule *> module_list;

void add_runtime_module(const RuntimeModule& runtime_module) {
    runtime_module_list.push_back(runtime_module);
    LOG_DEBUG(HLE, "added runtime module (0x%08x) %s", runtime_module.module_nid, runtime_module.library_name.c_str());
}

static std::unordered_map<uint32_t, SvcCall> nid_to_address_map;

void resolve_runtime_modules() {
    for (auto& i : runtime_module_list) {
        LOG_DEBUG(HLE, "resolving module 0x%08x (%s)", i.module_nid, i.library_name.c_str());
        for (auto& j : i.function_import_libraries) {
            LOG_DEBUG(HLE, "resolving import library 0x%08x (%s) functions %lu", j.library_nid, j.library_name.c_str(), j.function_imports.size());
            SvcCall svc;
            svc.library_name = j.library_name;
            for (auto& k : j.function_imports) {
                svc.nid = k.nid;
                svc.function = get_function_from_module_list(k.nid);
                nid_to_address_map[k.entry_address] = svc;
            }
        }
    }

    LOG_DEBUG(HLE, "resolved HLE functions");
}

SvcCall *get_function_from_address(uint32_t address) {
    auto it = nid_to_address_map.find(address);
    if (it != nid_to_address_map.end())
        return &it->second;
    return nullptr;
}

HLEFunction *get_function_from_module_list(uint32_t nid) {
    for (auto& i : module_list) {
        for (auto& j : i->functions) {
            if (j.nid == nid) {
                return &j;
            }
        }
    }

    return nullptr;
}

HLEFunction *get_function_from_module_list(const std::string& function_name) {
    for (auto& i : module_list) {
        for (auto& j : i->functions) {
            if (j.name == function_name) {
                return &j;
            }
        }
    }

    return nullptr;
}

void init_modules() {
    // SceSysmem modules

    // User modules
    module_list.push_back(get_module_SceSysmem());

    // Kernel modules

    // SceLibKernel modules

    // User modules
    module_list.push_back(get_module_SceLibKernel());

    // Kernel modules
}
}