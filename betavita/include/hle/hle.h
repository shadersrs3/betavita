#ifndef _BETAVITA_HLE_H
#define _BETAVITA_HLE_H

#include <cstdint>

#include <vector>
#include <string>

namespace betavita::hle {
struct FunctionImport {
    uint32_t nid;
    uint32_t entry_address;
};

struct FunctionImportLibrary {
    std::string library_name;
    uint32_t library_nid;
    std::vector<FunctionImport> function_imports;
};

struct SvcNid {
    std::string *library_name_ptr;
    uint32_t nid;
};

struct RuntimeModule {
    std::string library_name;
    uint32_t module_nid;
    std::vector<FunctionImportLibrary> function_import_libraries;
};

struct HLEModule {
    std::string name;
    uint32_t entry_address;
    uint32_t stop_address;
};

void add_runtime_module(const RuntimeModule& runtime_module);
void resolve_runtime_modules(); /* To be called to map entries to NIDs, then cleared */
SvcNid *get_nid_from_address(uint32_t address);
}

#endif /* _BETAVITA_HLE_H */