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
#include <hle/kernel.h>
#include <memory/memory.h>

#include <logger/logger.h>

#include <unicorn/arm.h>
#include <unicorn/unicorn.h>

using namespace betavita;

Logger SELFLOADER("self_loader"), MEMORY("memory"), HLE("hle"), KERNEL("kernel");

uc_engine *engine;

namespace betavita::processor {
uint32_t read_register_from_api(int core_num, int regnum) {
    uint32_t reg = 0;
    switch (regnum) {
    case 15:
        uc_reg_read(engine, UC_ARM_REG_PC, &reg);
        break;
    case 14:
        uc_reg_read(engine, UC_ARM_REG_LR, &reg);
        break;
    case 13:
        uc_reg_read(engine, UC_ARM_REG_SP, &reg);
        break;
    case 16:
        uc_reg_read(engine, UC_ARM_REG_CPSR, &reg);
        break;
    default:
        if (regnum >= 0 && regnum <= 12) {
            uc_reg_read(engine, UC_ARM_REG_R0 + regnum, &reg);
        }
    }

    return reg;
}

void write_register_from_api(int core_num, int regnum, uint32_t value) {
    switch (regnum) {
    case 15:
        uc_reg_write(engine, UC_ARM_REG_PC, &value);
        break;
    case 14:
        uc_reg_write(engine, UC_ARM_REG_LR, &value);
        break;
    case 13:
        uc_reg_write(engine, UC_ARM_REG_SP, &value);
        break;
    case 16:
        uc_reg_write(engine, UC_ARM_REG_CPSR, &value);
        break;
    default:
        if (regnum >= 0 && regnum <= 12) {
            uc_reg_write(engine, UC_ARM_REG_R0 + regnum, &value);
        }
    }
}

void stop_cpu_from_api() {
    uc_emu_stop(engine);
}
}

namespace betavita::memory {
void map_memory_from_api(uint32_t start, uint32_t end, void *ptr, uint8_t protection) {
    uint32_t perms = 0;
    for (int x = 0; x < 3; x++) {
        switch (protection & (1 << x)) {
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

    if (engine) {
        uc_err err = uc_mem_map_ptr(engine, start, (end - start) + 1, perms, ptr);
        if (err != UC_ERR_OK) {
            printf("Can't map memory 0x%08x ... 0x%08x\n", start, end);
        }
    }
}

void unmap_memory_from_api(uint32_t start, uint32_t end) {
    if (engine) {
        uc_mem_unmap(engine, start, (end - start) + 1);
    }
}
}

bool invalid_read_access_hook(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data) {
    printf("invalid read access %lx\n", address);
    return false;
}

bool invalid_write_access_hook(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data) {
    printf("invalid write access %lx = 0x%lx\n", address, value);
    return false;
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    // printf(">>> Tracing instruction at 0x%" PRIx64 ", instruction size = 0x%x\n", address, size);
}

void handle_svc(uc_engine *uc, uint32_t intno, void *user_data) {
    uint32_t pc, svc_address;

    uc_reg_read(uc, UC_ARM_REG_PC, &pc);

    svc_address = pc - 4;
    auto *svc = hle::get_function_from_address(svc_address);

    if (!svc || !svc->function) {
        printf("Unknown SVC for address 0x%08X ", svc_address);

        if (svc) {
            printf("NID 0x%08X library name %s", svc->nid, svc->library_name.c_str());
        }

        putchar('\n');

        uc_emu_stop(uc);
        return;
    }

    svc->function->func_wrapper();
}

namespace betavita::hle {

struct MemoryBlock {
    uint32_t address;
    int size;
    bool is_free;
};

std::vector<MemoryBlock> allocated_memory_blocks;
std::vector<MemoryBlock> free_memory_blocks;

static std::pair<uint32_t, uint32_t> more_memory(uint32_t size) {
    auto memlist = memory::get_memory_map_list();
    uint32_t top_address = 0x0, memory_size = (size + 0x1000) & ~0xFFF;

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

uint32_t allocate_memory_block(uint32_t size, const std::string& name = "") {
    auto [address, memory_size] = more_memory(size);

    return 0x0;
}

void free_memory(uint32_t addr) {

}
}

int main(int argc, char *argv[]) {
    hle::Kernel *kernel = new hle::Kernel;

    hle::Thread *thread = kernel->create_kernel_object<hle::Thread>();

    hle::allocate_memory_block(0x20, "stack_for_thread");

    delete kernel;
#if 0
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
    hle::init_modules();
    hle::resolve_runtime_modules();

    if (0) {
        uc_err err = uc_open(UC_ARCH_ARM, (uc_mode)UC_CPU_ARM_CORTEX_A9, &engine);

        if (err == UC_ERR_OK) {
            memory::map_memory("ProgramStack1", 0x78000000, 0x80000000-1, memory::PROTECTION_TYPE_READ | memory::PROTECTION_TYPE_WRITE);

            auto mmap_list = memory::get_memory_map_list();

            for (auto& i : mmap_list) {
                memory::map_memory_from_api(i.start, i.end, i.fast_access, i.protection);
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
#endif
    return 0;
}