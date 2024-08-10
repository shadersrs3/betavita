#ifndef _BETAVITA_KERNEL_H
#define _BETAVITA_KERNEL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>

#include <hle/vitatypes.h>

namespace betavita::hle {
enum SceObjectType { // Fake enum
    SCE_OBJECT_TYPE_NONE = 0,
    SCE_OBJECT_TYPE_THREAD = 1,
};

typedef enum SceKernelMutexAttribute {
	SCE_KERNEL_MUTEX_ATTR_RECURSIVE   = 0x02,
	SCE_KERNEL_MUTEX_ATTR_CEILING     = 0x04
} SceKernelMutexAttribute;

typedef enum SceThreadStatus {
	SCE_THREAD_RUNNING   = 1,
	SCE_THREAD_READY     = 2,
	SCE_THREAD_STANDBY   = 4,
	SCE_THREAD_WAITING   = 8,
	SCE_THREAD_SUSPEND   = 8, /* Compatibility */
	SCE_THREAD_DORMANT   = 16,
	SCE_THREAD_STOPPED   = 16, /* Compatibility */
	SCE_THREAD_DELETED   = 32, /* Thread manager has killed the thread (stack overflow) */
	SCE_THREAD_KILLED    = 32, /* Compatibility */
	SCE_THREAD_DEAD      = 64,
	SCE_THREAD_STAGNANT  = 128,
	SCE_THREAD_SUSPENDED = 256
} SceThreadStatus;

const char *get_object_type_name(const SceObjectType& type);

struct KernelObject {
private:
    SceObjectType object_type;
    SceUID uid;
public:
    virtual ~KernelObject() {}
    inline void set_object_type(SceObjectType type) { object_type = type; }
    inline void set_uid(SceUID uid) { this->uid = uid; }
    inline SceUID get_uid() const { return uid; }
    inline SceObjectType get_object_type() const { return object_type; }
    constexpr static SceObjectType get_static_object_type() { return SCE_OBJECT_TYPE_NONE; }
    constexpr const std::string& get_object_name() { return "None"; }
};

struct Thread : public KernelObject {
    struct Context {

    };

    SceUID processId;
    std::string name;
    uint32_t attr;
    uint32_t status;
    uint32_t entryPoint;
    uint32_t stackAddress;
    int stackSize;
    int initPriority;
    int currentPriority;
    uint32_t waitType;
    SceUID waitID;

    constexpr static SceObjectType get_static_object_type() { return SCE_OBJECT_TYPE_THREAD; }
    constexpr const std::string& get_object_name() { return "Thread"; }
};

struct Kernel {
private:
    int uid_value;

    std::unordered_map<SceUID, KernelObject *> kernel_object_map;
    std::vector<SceUID> thread_list;
    std::deque<SceUID> ready_threads_queue;
private:
    int generate_uid();

    void add_uid_to_list(std::vector<SceUID>& uid_list, SceUID uid);
    void remove_uid_from_list(std::vector<SceUID>& uid_list, SceUID uid);
    void add_uid_to_list(SceUID uid, SceObjectType type);
    void remove_uid_from_list(SceUID uid);

    // runtime
    Thread *fetch_ready_thread();
public:
    Kernel();
    ~Kernel();
    int thread_count();
    std::vector<SceUID> get_thread_list();

    template<typename T>
    T *create_kernel_object() {
        T *kobj = new T;
        int uid;

        if (!kobj)
            return nullptr;

        uid = generate_uid();
        kobj->set_object_type(T::get_static_object_type());
        kobj->set_uid(uid);

        kernel_object_map[uid] = kobj;
        add_uid_to_list(uid, T::get_static_object_type());
        return kobj;
    }

    template<typename T, bool check_type = true>
    T *get_kernel_object(SceUID uid) {
        auto it = kernel_object_map.find(uid);
        
        if (it != kernel_object_map.end()) {
            if constexpr (check_type) {
                if (it->second->get_object_type() == T::get_static_object_type())
                    return (T *) it->second;
                return nullptr;
            }

            return (T *) it->second;
        }
        return nullptr;
    }

    bool destroy_kernel_object(SceUID uid);

    // runtime
    void run_kernel(uint64_t us);
    void pause_core(int corenum);
};

void set_kernel(Kernel *state);
Kernel *kernel();
}

#endif /* _BETAVITA_KERNEL_H */