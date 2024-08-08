#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <memory>
#include <unordered_map>

#include <loader/self_loader.h>
#include <hle/hle.h>
#include <memory/memory.h>

#include <logger/logger.h>

using namespace betavita;

namespace betavita::hle {
typedef int SceUID;

enum ObjectType : SceUID {
    OBJECT_TYPE_ANY = 0,
};

struct KernelObject {
    std::string object_name;
    SceUID object_type;
    SceUID uid;

    KernelObject();
    virtual ~KernelObject() = default;
};

struct Module : public KernelObject {
private:
public:
};

KernelObject::KernelObject() : object_name("Empty Kernel Object"), object_type(OBJECT_TYPE_ANY), uid(0) {
}

KernelObject *get_kernel_object(SceUID uid) {
    return nullptr;
}
}

Logger SELFLOADER("self_loader"), MEMORY("memory"), HLE("HLE");

#include <unicorn/arm.h>
#include <unicorn/unicorn.h>

void handle_svc(uc_engine *uc, uint32_t intno, void *user_data) {
    uint32_t pc, svc_address;

    uc_reg_read(uc, UC_ARM_REG_PC, &pc);

    svc_address = pc - 4;
    hle::SvcNid *svc = hle::get_nid_from_address(svc_address);

    if (!svc) {
        printf("Unknown NID for address 0x%08X", svc_address);
        return;
    }

    switch (svc->nid) {
    // SceLibKernel
    default:
        printf("Unimplemented nid implementation 0x%08X library name (%s)\n", svc->nid, svc->library_name_ptr->c_str());
        uc_emu_stop(uc);
    }
}

bool invalid_read_access_hook(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data) {
    printf("Hi %lx\n", address);
    return false;
}

bool invalid_write_access_hook(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data) {
    printf("%lx\n", address);
    return false;
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    // printf(">>> Tracing instruction at 0x%" PRIx64 ", instruction size = 0x%x\n", address, size);
}

int main(int argc, char *argv[]) {
    std::string game_path = "../games/";
    game_path += "GTASA/eboot.bin";

    game_path = "../res/samples/hello_world/hello_world.self";
    FILE *f = fopen(game_path.c_str(), "rb");
    uint8_t *data;

    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = new uint8_t[size];

    fread(data, size, 1, f);
    fclose(f);

    std::shared_ptr<loader::SELFProgramData> self_data = loader::decrypt_self_data(data);
    bool state = loader::load_self_to_memory(self_data);

    self_data = nullptr;
    hle::resolve_runtime_modules();

    if (state) {
        uc_engine *engine;

        uc_err err = uc_open(UC_ARCH_ARM, (uc_mode)UC_CPU_ARM_CORTEX_A9, &engine);

        if (err == UC_ERR_OK) {
            memory::map_memory("ProgramStack1", 0x78000000, 0x80000000-1, memory::PROTECTION_TYPE_READ | memory::PROTECTION_TYPE_WRITE);

            auto mmap_list = memory::get_memory_map_list();

            for (auto& i : mmap_list) {
                uint32_t perms = 0;
                for (int x = 0; x < 3; x++) {
                    switch (i.protection & (1 << x)) {
                    case memory::PROTECTION_TYPE_EXECUTE:
                        perms |= UC_PROT_EXEC;
                        break;
                    case memory::PROTECTION_TYPE_READ:
                        perms |= UC_PROT_READ;
                        break;
                    case memory::PROTECTION_TYPE_WRITE:
                        perms |= UC_PROT_WRITE;
                        break;
                    }
                }

                err = uc_mem_map_ptr(engine, i.start, i.size, perms, i.fast_access);
                if (err != UC_ERR_OK) {
                    printf("Unicorn memory map error\n");
                }
            }

            std::vector<uc_hook> hooks;
            uc_hook hook;

            err = uc_hook_add(engine, &hook, UC_HOOK_MEM_READ_INVALID, (void *) &invalid_read_access_hook, nullptr, 0x0, 0xFFFFFFFF);
            hooks.push_back(hook);
            err = uc_hook_add(engine, &hook, UC_HOOK_MEM_WRITE_INVALID, (void *) &invalid_write_access_hook, nullptr, 1, 0);
            hooks.push_back(hook);
            uc_hook_add(engine, &hook, UC_HOOK_CODE, (void *) &hook_code, nullptr, 1, 0);
            hooks.push_back(hook);
            uc_hook_add(engine, &hook, UC_HOOK_INTR, (void *) &handle_svc, nullptr, 1, 0);
            hooks.push_back(hook);

            uint32_t value = 0x80000000;
            uc_reg_write(engine, UC_ARM_REG_SP, &value);

            err = uc_emu_start(engine, 0x810001b8 | 1, -1, -1, -1);
            printf("%s\n", uc_strerror(err));

            for (auto& i : hooks) {
                uc_hook_del(engine, i);
            }

            hooks.clear();
            uc_close(engine);
        } else {
            printf("Can't load unicorn\n");
        }
    } else
        printf("Can't load memory\n");

    delete[] data;
    return 0;
}