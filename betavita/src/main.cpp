#include <cstdio>
#include <string>
#include <cstring>
#include <unordered_map>
#include <algorithm>

#include <loader/self_loader.h>
#include <hle/hle.h>
#include <hle/kernel.h>
#include <hle/memory_manager.h>
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

void stop_cpu_from_api(int core_num) {
    uc_emu_stop(engine);
}

int get_available_core() {
    return 0;
}

void run_cpu_for(int core_num, RegisterContext *context, uint64_t instructions_count) {
    int core = get_available_core();

    (void) core;

    bool thumb = (context->cpsr & 0x20) != 0;
    uint32_t pc = context->reg[15] | thumb;

    uc_err err = uc_emu_start(engine, pc, -1, -1, -1);
    printf("%s\n", uc_strerror(err));
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

void change_protection_from_api(uint32_t start, uint32_t end, uint8_t protection) {
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
        uc_mem_protect(engine, start, (end - start) + 1, perms);
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

uint32_t address[2];
void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    ::address[0] = ::address[1];
    ::address[1] = address;

    uint32_t data = *(uint32_t *) memory::get_pointer((uint32_t) address);

    (void) data;

    // printf(">>> Tracing instruction at 0x%" PRIx64 ", instruction size = 0x%x (0x%08x)\n", address, size, data);
}

#include <unordered_map>

void handle_svc(uc_engine *uc, uint32_t intno, void *user_data) {
    uint32_t pc, svc_address;

    uc_reg_read(uc, UC_ARM_REG_PC, &pc);

    svc_address = pc - 4;
    auto *svc = hle::get_function_from_address(svc_address);

    if (!svc || !svc->function) {
        printf("Unknown SVC for address 0x%08X (call 0x%08x) ", svc_address, address[0]);

        if (svc) {
            printf("NID 0x%08X library name %s", svc->nid, svc->library_name.c_str());
        }

        putchar('\n');

        uc_emu_stop(uc);
        return;
    }

    hle::kernel()->lock_core_svc();
    hle::kernel()->set_core(0);
    svc->function->func_wrapper();
    hle::kernel()->unlock_core_svc();
}

int main(int argc, char *argv[]) {
#if 0
    hle::Kernel *kernel = new hle::Kernel;

    hle::set_kernel(kernel);

    std::string game_path;

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
    delete[] data;


    hle::init_modules();
    hle::resolve_runtime_modules();

    if (state) {
        uc_err err = uc_open(UC_ARCH_ARM, (uc_mode)UC_CPU_ARM_CORTEX_A9, &engine);

        if (err == UC_ERR_OK) {
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

            // https://github.com/unicorn-engine/unicorn/pull/572
            unsigned char code[] = {
                // Enable CP10 and CP11
                0x50, 0x0f, 0x11, 0xee, // mrc p15, 0, r0, c1, c0, 2
                0x0f, 0x06, 0x80, 0xe3, // orr r0, r0, 0xf00000
                0x50, 0x0f, 0x01, 0xee, // mcr p15, 0, r0, c1, c0, 2

                // IMB
                // 0x00, 0x00, 0xf0, 0xef, // IMB (SWI 0xf00000)

                // Enable VFP in FPEXC
                0x40, 0x04, 0xa0, 0xe3, // mov r0, 0x40000000
                0x10, 0x0a, 0xe8, 0xee, // vmsr fpexc, r0

                0x1e, 0xff, 0x2f, 0xe1
            };

            uint32_t arbitrary_code_heap = hle::allocate_heap(sizeof code, "arbitrary_code_execution");

            std::memcpy(memory::get_pointer(arbitrary_code_heap), code, sizeof code);
            memory::change_protection(arbitrary_code_heap, arbitrary_code_heap + 0xFFF, memory::PROTECTION_TYPE_EXECUTE | memory::PROTECTION_TYPE_READ | memory::PROTECTION_TYPE_WRITE);

            uc_err err = uc_emu_start(engine, arbitrary_code_heap, -1, -1, -1);
            printf("%s\n", uc_strerror(err));

            int thid = kernel->create_thread("entry_thread", 0x810001b8 | 1, 32, 0x2000, 0x0, 0x0);
            kernel->start_thread(thid, 0, 0);

            kernel->run_single_thread_with_events();

            for (auto& i : hooks) {
                uc_hook_del(engine, i);
            }

            hooks.clear();
            uc_close(engine);
#if 0
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window *window = SDL_CreateWindow("PSVita emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, framebuffer.framebuffer_width, framebuffer.framebuffer_height, SDL_WINDOW_OPENGL);
            SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, framebuffer.framebuffer_width, framebuffer.framebuffer_height);

            void *pixels;
            int pitch;

            SDL_LockTexture(texture, nullptr, &pixels, &pitch);
            std::memcpy(pixels, memory::get_pointer_unchecked(framebuffer.framebuffer_base), framebuffer.framebuffer_width * framebuffer.framebuffer_height * 4);

            SDL_UnlockTexture(texture);

            SDL_RenderCopy(renderer, texture, nullptr, nullptr);

            bool quit = false;
            while (!quit) {
                SDL_Event ev;
                while (SDL_PollEvent(&ev)) {
                    if (ev.type == SDL_QUIT)
                        quit = true;
                }

                SDL_RenderPresent(renderer);
                SDL_Delay(16);
            }

            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
#endif
        } else {
            printf("Can't load unicorn\n");
        }
    } else
        printf("Can't load memory\n");

    delete kernel;
#endif
    return 0;
}