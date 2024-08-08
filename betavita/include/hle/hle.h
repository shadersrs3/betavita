#ifndef _BETAVITA_HLE_H
#define _BETAVITA_HLE_H

#include <cstdint>

#include <vector>
#include <string>

namespace betavita::hle {
struct HLEFunction;

struct FunctionImport {
    uint32_t nid;
    uint32_t entry_address;
};

struct FunctionImportLibrary {
    std::string library_name;
    uint32_t library_nid;
    std::vector<FunctionImport> function_imports;
};

struct SvcCall {
    std::string library_name;
    uint32_t nid;
    HLEFunction *function;
};

struct RuntimeModule {
    std::string library_name;
    uint32_t module_nid;
    std::vector<FunctionImportLibrary> function_import_libraries;
};

struct HLEFunction {
    std::string name;
    uint32_t nid;
    int supervisor_call_number;
    void (*func_wrapper)();
};

struct HLEModule {
    std::string name;
    uint32_t nid;
    std::vector<HLEFunction> functions;
};

void add_runtime_module(const RuntimeModule& runtime_module);

void resolve_runtime_modules(); /* To be called to map entries to NIDs, then cleared */
SvcCall *get_function_from_address(uint32_t address);

HLEFunction *get_function_from_module_list(uint32_t nid);
HLEFunction *get_function_from_module_list(const std::string& function_name);

void init_modules();
}

#endif /* _BETAVITA_HLE_H */