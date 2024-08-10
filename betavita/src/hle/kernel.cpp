#include <algorithm>

#include <hle/hle.h>
#include <hle/kernel.h>

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

void Kernel::pause_core(int corenum) {

}

void set_kernel(Kernel *state) {
    m_kernel = state;
}

Kernel *kernel() {
    return m_kernel;
}
}