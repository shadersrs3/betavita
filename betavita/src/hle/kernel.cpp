#include <cstring>
#include <algorithm>

#include <hle/hle.h>
#include <hle/kernel.h>
#include <hle/memory_manager.h>

#include <logger/logger.h>

namespace betavita::hle {
static Kernel *m_kernel = nullptr;

const char *get_object_type_name(const SceObjectType& type) {
    switch (type) {
    case SCE_OBJECT_TYPE_NONE: return "SCE_OBJECT_TYPE_NONE";
    case SCE_OBJECT_TYPE_THREAD: return "SCE_OBJECT_TYPE_THREAD";
    }
    return "NULL";
}

Kernel::Kernel() {
    uid_value = 0x100;
    current_core = -1;

    for (int i = 0; i < 4; i++)
        current_thread[i] = nullptr;

    display = new Display;
    display->set_framebuffer(nullptr, 0);
}

Kernel::~Kernel() {
    for (auto& i : kernel_object_map) {
        SceUID uid = i.second->get_uid();
        KernelObject *kobj = get_kernel_object<KernelObject, false>(uid);
        if (!kobj)
            continue;

        remove_uid_from_list(uid);
        delete kobj;
    }

    if (display) {
        delete display;
        display = nullptr;
    }

    kernel_object_map.clear();
}

int Kernel::generate_uid() {
    uint32_t new_val = uid_value; // temporary solution
    uid_value += 1;
    return new_val;
}

void Kernel::add_uid_to_list(std::vector<SceUID>& uid_list, SceUID uid) {
    uid_list.push_back(uid);
}

void Kernel::remove_uid_from_list(std::vector<SceUID>& uid_list, SceUID uid) {
    auto found = std::find(uid_list.begin(), uid_list.end(), uid);
    if (found != uid_list.end())
        uid_list.erase(found);
}

void Kernel::add_uid_to_list(SceUID uid, SceObjectType type) {
    KernelObject *kobj = get_kernel_object<KernelObject, false>(uid);

    if (!kobj) {
        LOG_ERROR(KERNEL, "can't add UID to list if kobj is NULL");
        return;
    }

    switch (auto obj_type = kobj->get_object_type(); obj_type) {
    case SCE_OBJECT_TYPE_THREAD:
        add_uid_to_list(thread_list, kobj->get_uid());
        break;
    default:
        LOG_ERROR(KERNEL, "can't add kernel object type %d (%s) to list!", obj_type, get_object_type_name(obj_type));
    }
}

void Kernel::remove_uid_from_list(SceUID uid) {
    KernelObject *kobj = get_kernel_object<KernelObject, false>(uid);

    if (!kobj) {
        LOG_ERROR(KERNEL, "can't remove UID to list if kobj is NULL");
        return;
    }

    switch (auto obj_type = kobj->get_object_type(); obj_type) {
    case SCE_OBJECT_TYPE_THREAD:
        remove_uid_from_list(thread_list, kobj->get_uid());
        break;
    default:
        LOG_ERROR(KERNEL, "can't remove kernel object type %d (%s) to list!", obj_type, get_object_type_name(obj_type));
    }
}

bool Kernel::destroy_kernel_object(SceUID uid) {
    KernelObject *kobj;

    auto it = kernel_object_map.find(uid);
    if (it == kernel_object_map.end())
        return false;

    kobj = it->second;
    if (!kobj)
        return false;

    remove_uid_from_list(uid);
    delete kobj;
    kernel_object_map.erase(it);
    return true;
}

Display *Kernel::get_display() {
    return display;
}

int Kernel::create_thread(const std::string& name, uint32_t entry, int initPriority, uint32_t stackSize, uint32_t attr, uint32_t option) {
    auto *t = create_kernel_object<Thread>();

    if (!t)
        return -1; /* TODO: error code */

    t->processId = 0;
    t->name = name;
    t->attr = attr;
    t->status = SCE_THREAD_STAGNANT;
    t->entryPoint = entry;
    t->stackAddress = 0;
    t->stackSize = stackSize;
    t->currentPriority = t->initPriority = initPriority;
    t->waitType = SCE_WAIT_TYPE_NONE;
    t->waitID = 0;
    t->tlsLowAddress = 0;
    std::memset(&t->context, 0, sizeof t->context);
    return t->get_uid();
}

int Kernel::start_thread(SceUID thid, uint32_t argLen, uint32_t argPtr) {
    auto t = get_kernel_object<Thread>(thid);

    if (!t)
        return -1; /* TODO handle error code */

    if (argPtr != 0) {
        LOG_ERROR(KERNEL, "argptr is valid!");
        std::exit(0);
    }

    t->context.reg[15] = t->entryPoint;
    t->context.reg[14] = 0xBAD4F00D;
    t->context.reg[13] = hle::allocate_stack(t->stackSize, t->name);
    t->context.cpsr = 0x400001f3;
    t->tlsLowAddress = hle::allocate_heap(0x1000, t->name + "_tls");
    t->status = SCE_THREAD_READY;
    return 0;
}

uint32_t Kernel::get_tls_addr(SceUID thid, int key) {
    constexpr uint32_t bad_addr = 0xFFFFFFFF;
    auto t = get_kernel_object<Thread>(thid);

    if (!t)
        return bad_addr;

    if (key >= 0 && key <= 0x100)
        return t->tlsLowAddress + key * 0x4;

    LOG_WARN(KERNEL, "attempting to get TLS key 0x%08x", key);
    return 0xFFFFFFFF;
}

Thread *Kernel::fetch_ready_thread() {
    if (ready_threads_queue.size() == 0) {
        for (auto& i : thread_list) {
            Thread *thr = get_kernel_object<Thread>(i);
            if (!thr)
                continue;

            if (thr->status == SCE_THREAD_READY)
                ready_threads_queue.push_back(thr->get_uid());
        }

        if (ready_threads_queue.size() == 0)
            return nullptr;
    }

    Thread *t = nullptr;

    do {
        SceUID thread_uid = ready_threads_queue.front();
        t = get_kernel_object<Thread>(thread_uid);
        ready_threads_queue.pop_front();
    } while (ready_threads_queue.size() != 0 && t != nullptr);

    if (!t)
        LOG_WARN(KERNEL, "how is the fetched thread NULL?");
    return t;
}

Thread *Kernel::get_current_thread() {
    return current_thread[current_core];
}

void Kernel::run_single_thread_with_events() {
    int available_core;

    constexpr uint32_t cpu_hz = 2'000'000'000;
    constexpr auto us_to_cycles = [](double us) -> uint64_t { return (uint64_t)((double)(cpu_hz / 1'000'000) * us); };
    auto t = fetch_ready_thread();

    if (!t)
        return;

    available_core = processor::get_available_core();

    current_thread[available_core] = t;
    current_core = available_core;

    t->status = SCE_THREAD_RUNNING;

    for (int i = 0; i < 16; i++) {
        processor::write_register_from_api(available_core, i, t->context.reg[i]);
    }

    processor::write_register_from_api(available_core, 16, t->context.cpsr);
    t->context.cpsr = processor::read_register_from_api(available_core, 16);

    processor::run_cpu_for(available_core, &t->context, us_to_cycles(120.));

    for (int i = 0; i < 16; i++)
        t->context.reg[i] = processor::read_register_from_api(available_core, i);

    t->context.cpsr = processor::read_register_from_api(available_core, 16);
    if (t->status == SCE_THREAD_RUNNING) {
        t->status = SCE_THREAD_READY;
    }
}

void Kernel::lock_core_svc() {
    svc_mutex.lock();
}

void Kernel::unlock_core_svc() {
    svc_mutex.unlock();
}

void Kernel::set_core(int core_num) {
    current_core = core_num;
}

void Kernel::pause_current_core() {
    processor::stop_cpu_from_api(current_core);
}

void set_kernel(Kernel *state) {
    m_kernel = state;
}

Kernel *kernel() {
    return m_kernel;
}
}