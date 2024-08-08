static bool load_var_exports(const uint32_t *nids, const Ptr<uint32_t> *entries, size_t count, KernelState &kernel, MemState &mem) {
    const std::lock_guard<std::mutex> guard(kernel.export_nids_mutex);
    for (size_t i = 0; i < count; ++i) {
        const uint32_t nid = nids[i];
        const Ptr<uint32_t> entry = entries[i];

        if (nid == NID_PROCESS_PARAM) {
            if (!kernel.process_param)
                kernel.load_process_param(mem, entry);
            LOG_DEBUG("\tNID {} (SCE_PROC_PARAMS) at {}", log_hex(nid), log_hex(entry.address()));
            continue;
        }

        if (nid == NID_MODULE_INFO) {
            LOG_DEBUG("\tNID {} (NID_MODULE_INFO) at {}", log_hex(nid), log_hex(entry.address()));
            continue;
        }

        if (nid == NID_SYSLYB) {
            LOG_DEBUG("\tNID {} (SYSLYB) at {}", log_hex(nid), log_hex(entry.address()));
            continue;
        }

        if (kernel.debugger.log_exports) {
            const char *const name = import_name(nid);

            LOG_DEBUG("\tNID {} ({}) at {}", log_hex(nid), name, log_hex(entry.address()));
        }

        Address old_entry_address = 0;
        auto nid_it = kernel.export_nids.find(nid);
        if (nid_it != kernel.export_nids.end()) {
            LOG_DEBUG("Found previously not found variable. nid:{}, new_entry_point:{}", log_hex(nid), log_hex(entry.address()));
            old_entry_address = kernel.export_nids[nid];
            kernel.export_nids[nid] = entry.address();
        }

        bool reloc_success = true;
        auto range = kernel.var_binding_infos.equal_range(nid);
        for (auto i = range.first; i != range.second; ++i) {
            auto &var_binding_info = i->second;
            if (var_binding_info.size == 0)
                continue;

            SegmentInfosForReloc seg;
            const auto &module_info = kernel.loaded_modules[kernel.module_uid_by_nid[var_binding_info.module_nid]];
            if (!module_info) {
                reloc_success = false;
                LOG_ERROR("Module not found by nid: {} uid: {}", log_hex(var_binding_info.module_nid), kernel.module_uid_by_nid[var_binding_info.module_nid]);
            } else {
                for (int i = 0; i < MODULE_INFO_NUM_SEGMENTS; i++) {
                    const auto &segment = module_info->info.segments[i];
                    if (segment.size > 0) {
                        seg[i] = { segment.vaddr.address(), 0, segment.memsz }; // p_vaddr is not used in variable relocations
                    }
                }
            }

            // Note: We make the assumption that variables are not imported into executable code (wouldn't make a lot of sense)
            // If this is not the case, uncomment the following
            /* if (!seg.empty()) {
                for (const auto &[key, value] : seg) {
                    kernel.invalidate_jit_cache(value.addr, value.size);
                }
            }*/

            if (!seg.empty()) {
                if (!relocate(var_binding_info.entries, var_binding_info.size, seg, mem, true, entry.address())) {
                    reloc_success = false;
                    LOG_ERROR("Failed to relocate late binding info");
                }
            }
        }
        if (old_entry_address)
            free(mem, old_entry_address);
    }
    return true;
}

static bool load_exports(SceKernelModuleInfo *kernel_module_info, const sce_module_info_raw &module, Ptr<const void> segment_address, KernelState &kernel, MemState &mem, bool is_unload = false) {
    const uint8_t *const base = segment_address.cast<const uint8_t>().get(mem);
    const sce_module_exports_raw *const exports_begin = reinterpret_cast<const sce_module_exports_raw *>(base + module.export_top);
    const sce_module_exports_raw *const exports_end = reinterpret_cast<const sce_module_exports_raw *>(base + module.export_end);

    for (const sce_module_exports_raw *exports = exports_begin; exports < exports_end; exports = reinterpret_cast<const sce_module_exports_raw *>(reinterpret_cast<const uint8_t *>(exports) + exports->size)) {
        const char *const lib_name = Ptr<const char>(exports->library_name).get(mem);

        if (kernel.debugger.log_exports)
            LOG_INFO("Loading func exports from {}", lib_name ? lib_name : "unknown");

        const uint32_t *const nids = Ptr<const uint32_t>(exports->nid_table).get(mem);
        const Ptr<uint32_t> *const entries = Ptr<Ptr<uint32_t>>(exports->entry_table).get(mem);
        if (!is_unload && !load_func_exports(kernel_module_info, nids, entries, exports->num_syms_funcs, kernel))
            return false;
        if (is_unload && !unload_func_exports(kernel_module_info, nids, entries, exports->num_syms_funcs, kernel, mem))
            return false;

        const auto var_count = exports->num_syms_vars;

        if (kernel.debugger.log_exports && var_count > 0) {
            LOG_INFO("Loading var exports from {}", lib_name ? lib_name : "unknown");
        }

        if (!is_unload && !load_var_exports(&nids[exports->num_syms_funcs], &entries[exports->num_syms_funcs], var_count, kernel, mem))
            return false;
        if (is_unload && !unload_var_exports(&nids[exports->num_syms_funcs], &entries[exports->num_syms_funcs], var_count, kernel, mem))
            return false;
    }

    return true;
}
