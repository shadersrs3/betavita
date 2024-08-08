#include <zlib.h>

#include <memory/memory.h>

#include <hle/hle.h>

#include <loader/sce-type.h>
#include <loader/sce-elf.h>
#include <loader/self_loader.h>

#include <logger/logger.h>

namespace betavita::loader {
std::shared_ptr<SELFProgramData> decrypt_self_data(const uint8_t *in) {
    std::shared_ptr<SELFProgramData> self;

    const SCEHeader *hdr = (const SCEHeader *) in;
    const ELFHdr32 *ehdr = (const ELFHdr32 *) (in + hdr->elf_offset);
    const ELFPhdr32 *phdr = (const ELFPhdr32 *) (in + hdr->phdr_offset); 
    const SegmentInfo *info = (const SegmentInfo *) (in + hdr->section_info_offset);

    std::vector<DecryptedSegment> segments;
    bool relocatable;

    switch (ehdr->e_type) {
    case ET_SCE_EXEC:
        relocatable = false;
        break;
    case ET_SCE_RELEXEC:
        relocatable = true;
        break;
    default:
        LOG_ERROR(SELFLOADER, "Unimplemented e_type 0x%08x", ehdr->e_type);
        return nullptr;
    }

    for (int i = 0; i < ehdr->e_phnum; i++, info++, phdr++) {
        DecryptedSegment segment;
        segment.phdr = *phdr;
        segment.segment_length = phdr->p_filesz;
        std::shared_ptr<uint8_t> data(std::shared_ptr<uint8_t>(new uint8_t[segment.segment_length]));
        segment.data = std::move(data);
        segment.name = "program_segment_" + std::to_string(i);
        switch (phdr->p_type) {
        case PT_LOAD:
            segment.relocatable = false;
            break;
        case PT_SCE_RELA:
            segment.relocatable = true;
            break;
        default:
            LOG_WARN(SELFLOADER, "Unknown program type 0x%08x", phdr->p_type);
            segments.push_back({});
            continue;
        }

        segment.can_load = true;

        if (info->encryption == 1) {
            LOG_ERROR(SELFLOADER, "Unimplemented encrypted SELF segment");
            return nullptr;
        }

        LOG_DEBUG(SELFLOADER, "Segment %d file size: 0x%08x segment size 0x%08lx virt addr: 0x%08x phys addr: 0x%08x enc %lx comp %lx", i, phdr->p_filesz, info->length, phdr->p_vaddr, phdr->p_paddr, info->encryption, info->compression);
        if (info->compression == 2) {
            if (uncompress(segment.data.get(), &segment.segment_length, in + info->offset, info->length) != Z_OK) {
                LOG_ERROR(SELFLOADER, "Can't decompress segment data %d", i);
                return nullptr;
            } else {
                LOG_DEBUG(SELFLOADER, "Decompressed segment data %d", i);
            }
        }

        segments.push_back(segment);
    }

    self = std::make_shared<SELFProgramData>();
    self->entry = ehdr->e_entry;
    self->relocatable = relocatable;
    self->executable_type = ehdr->e_type;
    self->segments = std::move(segments);
    return self;
}

static void debug_module_info(const sce_module_info_raw *modinfo) {
    printf("Module name: ");
    for (int i = 0; i < 27; i++)
        putchar(modinfo->name[i]);

    putchar('\n');
    printf("Module type: 0x%02x\n", modinfo->type);
}

// Common type so we can static_cast between unknown/known formats
// This is based on https://github.com/vitasdk/vita-toolchain/blob/master/src/sce-elf.h
struct Entry {};

struct EntryFormatUnknown : Entry {
    uint32_t format : 4;
};

struct EntryFormat0 : Entry {
    uint32_t format : 4;
    uint32_t symbol_segment : 4;
    uint32_t code : 8;
    uint32_t patch_segment : 4;
    uint32_t code2 : 8;
    uint32_t dist2 : 4;

    uint32_t addend;

    uint32_t offset;
};

struct EntryFormat1 : Entry {
    uint32_t format : 4;
    uint32_t symbol_segment : 4;
    uint32_t code : 8;
    uint32_t patch_segment : 4;
    uint32_t offset_lo : 12;

    uint32_t offset_hi : 10;
    uint32_t addend : 22;
};

// Used by var import relocations
struct EntryFormat1Alt : Entry {
    uint32_t format : 4;
    uint32_t patch_segment : 4;
    uint32_t code : 8;
    uint32_t addend : 16;

    uint32_t offset;
};

struct EntryFormat2 : Entry {
    uint32_t format : 4;
    uint32_t symbol_segment : 4;
    uint32_t code : 8;
    uint32_t offset : 16;

    uint32_t addend;
};

// Used by var import relocations
struct EntryFormat2Alt : Entry {
    uint32_t format : 4;
    uint32_t patch_segment : 4;
    uint32_t code : 8;
    uint32_t padding : 16;

    uint32_t offset;
    uint32_t addend;
};

struct EntryFormat3 : Entry {
    uint32_t format : 4;
    uint32_t symbol_segment : 4;
    uint32_t mode : 1; // ARM = 0, THUMB = 1
    uint32_t offset : 18;
    uint32_t dist2 : 5;

    uint32_t addend : 22;
};

struct EntryFormat4 : Entry {
    uint32_t format : 4;
    uint32_t offset : 23;
    uint32_t dist2 : 5;
};

struct EntryFormat5 : Entry {
    uint32_t format : 4;
    uint32_t dist1 : 9;
    uint32_t dist2 : 5;
    uint32_t dist3 : 9;
    uint32_t dist4 : 5;
};

struct EntryFormat6 : Entry {
    uint32_t format : 4;
    uint32_t offset : 28;
};

struct EntryFormat7_8_9 : Entry {
    uint32_t format : 4;
    // format 7 has 4 offsets, 7 bits each
    // format 8 has 7 offsets, 4 bits each
    // format 9 has 14 offsets, 2 bits each
    uint32_t offsets : 28;
};

enum ArmRelocType {
    R_ARM_NONE = 0,
    R_ARM_ABS32 = 2, R_ARM_TARGET1 = 38,
    R_ARM_REL32 = 3, R_ARM_TARGET2 = 41,
    R_ARM_THM_CALL = 10,
    R_ARM_CALL = 28,
    R_ARM_JUMP24 = 29,
    R_ARM_V4BX = 40,
    R_ARM_PREL31 = 42,
    R_ARM_MOVW_ABS_NC = 43,
    R_ARM_MOVT_ABS = 44,
    R_ARM_THM_MOVW_ABS_NC = 47,
    R_ARM_THM_MOVT_ABS = 48,
    R_ARM_RBASE = 255
};

static bool do_self_relocation_program_data(const std::shared_ptr<SELFProgramData>& _data) {
    SELFProgramData *data = _data.get();

    auto apply_one_relocation = [&](uint8_t relocation_type, uint32_t S, uint32_t A, uint32_t P) -> bool {
        void *data = memory::get_pointer_unchecked(P);
        const uint32_t displacement = A - P + S;

        if (!data) {
            LOG_ERROR(SELFLOADER, "invalid relocation patch address 0x%08x", P);
            return false;
        }

        switch (relocation_type) {
        case R_ARM_ABS32:
        case R_ARM_TARGET1:
            *(uint32_t *) data = S + A;
            break;
        case R_ARM_PREL31:
            *(uint32_t *) data = displacement & 0x7FFFFFFF;
            break;
        case R_ARM_THM_CALL:
        {
            struct Upper {
                uint16_t imm10 : 10;
                uint16_t sign : 1;
                uint16_t ignored : 5;
            };

            struct Lower {
                uint16_t imm11 : 11;
                uint16_t j2 : 1;
                uint16_t unknown : 1;
                uint16_t j1 : 1;
                uint16_t unknown2 : 2;
            };

            struct Pair {
                Upper upper;
                Lower lower;
            };

            Pair *pair = (Pair *)(data);
            pair->lower.imm11 = displacement >> 1;
            pair->upper.imm10 = displacement >> 12;
            pair->upper.sign = displacement >> 24;
            pair->lower.j2 = pair->upper.sign ^ ((~displacement) >> 22);
            pair->lower.j1 = pair->upper.sign ^ ((~displacement) >> 23);
            break;
        }
        case R_ARM_THM_MOVW_ABS_NC:
        {
            uint32_t opcode = *(uint32_t *) data;

            uint32_t target = (S + A);
            int imm8 = target & 0xFF;
            int imm3 = (target >> 8)  & 0x7;
            int imm1 = (target >> 11) & 0x1;
            int imm4 = (target >> 12) & 0xF;

            opcode &= 0x8F00FBF0;
            opcode |= (imm8 << 16) | (imm3 << 28) | (imm1 << 10) | imm4;
            *(uint32_t *) data = opcode;
            break;
        }
        case R_ARM_THM_MOVT_ABS:
        {
            uint32_t opcode = *(uint32_t *) data;
            uint32_t target = (S + A);
            int imm8 = (target >> 16) & 0xFF;
            int imm3 = (target >> 24)  & 0x7;
            int imm1 = (target >> 27) & 0x1;
            int imm4 = (target >> 28) & 0xF;			
            opcode &= 0x8F00FBF0;
            opcode |= (imm8 << 16) | (imm3 << 28) | (imm1 << 10) | imm4;
            *(uint32_t *) data = opcode;
            break;
        }
        default:
            LOG_ERROR(SELFLOADER, "Unimplemented relocation type %d", relocation_type);
            return false;
        }
        return true;
    };

    uint32_t g_addr = 0, g_offset = 0, g_patchseg = 0,
        g_saddr = 0, g_addend = 0, g_type = 0, g_type2 = 0;

    auto perform_segment_relocation = [&](const DecryptedSegment *seg) -> bool {
        uint8_t *program_data = seg->data.get();
        uint8_t *program_data_end = seg->data.get() + seg->phdr.p_filesz;

        LOG_DEBUG(SELFLOADER, "Performing %s_prx2reloc relocation size 0x%08x", seg->name.c_str(), seg->phdr.p_filesz);

        while (program_data < program_data_end) {
            int struct_size = 0;
            EntryFormatUnknown *rel_unknown = (EntryFormatUnknown *) program_data;
            switch (rel_unknown->format) {
            case 0:
            {
                EntryFormat0 *fmt0 = (EntryFormat0 *) rel_unknown;

                int symbol_segment = fmt0->symbol_segment;
                int patch_segment = fmt0->patch_segment;

                uint32_t symbol_address, addend, patch_address, patch_address_start;
                if (symbol_segment < data->segments.size()) {
                    symbol_address = data->segments[symbol_segment].phdr.p_vaddr;
                } else {
                    symbol_address = 0;
                }

                if (patch_segment < data->segments.size()) {
                    patch_address_start = data->segments[patch_segment].phdr.p_vaddr;
                    patch_address = patch_address_start + fmt0->offset;
                } else {
                    patch_address_start = 0;
                    patch_address = 0;
                }

                addend = fmt0->addend;
                if (!apply_one_relocation(fmt0->code, symbol_address, addend, patch_address)) {
                    LOG_ERROR(SELFLOADER, "Can't apply relocation for code format 0");
                    return false;
                }

                if (fmt0->code2 != 0) {
                    patch_address = patch_address + fmt0->dist2 * 2;
                    if (!apply_one_relocation(fmt0->code2, symbol_address, addend, patch_address + fmt0->dist2 * 2)) {
                        LOG_ERROR(SELFLOADER, "Can't apply relocation for code2 format 0");
                        return false;
                    }
                }

                g_addr = patch_address_start;
                g_patchseg = fmt0->patch_segment;
                g_offset = fmt0->offset;
                g_type = fmt0->code;
                g_type2 = fmt0->code2;
                g_saddr = symbol_address;
                g_addend = fmt0->addend;
                struct_size = sizeof *fmt0;
                break;
            }
            default:
                LOG_ERROR(SELFLOADER, "Unimplemented format %d for self relocation %s!", rel_unknown->format, seg->name.c_str());
                return false;
            }

            program_data += struct_size;
            // break;
        }
        return true;
    };

    for (auto& i : data->segments) {
        if (i.relocatable) {
            bool performed;

            performed = perform_segment_relocation(&i);
            if (!performed)
                return false;
        }
    }
    return true;
}

static __attribute__((unused)) bool relocate_imports_exports(const std::shared_ptr<SELFProgramData>& data, const sce_module_info_raw *modinfo) {
    return false;
}

static void patch_function_import_data(uint32_t *data) {
    uint32_t patch_data[2] = {
        0xEF000000,
        0xE12FFF1E,
    };

    for (int i = 0; i < 2; i++)
        *(data + i) = patch_data[i];
}

static bool add_runtime_library_functions(const sce_module_info_raw *modinfo, uint32_t base_address = 0) {
    hle::RuntimeModule mod;

    uint32_t import_start_addr, import_end_addr;

    import_start_addr = modinfo->import_top;
    import_end_addr = modinfo->import_end;

    while (import_start_addr < import_end_addr) {
        int import_struct_size;
        hle::FunctionImportLibrary library {};
        uint32_t import_address = base_address + import_start_addr;
        auto import = (sce_module_imports_raw *) memory::get_pointer_unchecked(import_address);

        if (!import) {
            LOG_ERROR(SELFLOADER, "Invalid import address 0x%08x", import_address);
            return false;
        }

        uint32_t library_name_addr, library_nid, func_nid_table, func_entry_table;
        int num_funcs;

        import_struct_size = import->size;
        if (import_struct_size == 0x24) {
            // mod.function_import_libraries.push_back(library);
            LOG_ERROR(SELFLOADER, "unimplemented struct size 0x24 for sce_module_import*");
            return false;
        } else if (import_struct_size == 0x34) {
            num_funcs = import->num_syms_funcs;
            library_name_addr = import->library_name;
            library_nid = import->library_nid;
            func_nid_table = import->func_nid_table;
            func_entry_table = import->func_entry_table;
        } else {
            LOG_ERROR(SELFLOADER, "unimplemented struct size 0x%04x for sce_module_import*", import_struct_size);
            return false;
        }

        uint32_t *nid = (uint32_t *) memory::get_pointer_unchecked(func_nid_table);

        if (!nid) {
            LOG_ERROR(SELFLOADER, "unknown address for function nid table 0x%08x", func_nid_table);
            return false;
        }

        uint32_t *entry_addr = (uint32_t *) memory::get_pointer_unchecked(func_entry_table);

        if (!entry_addr) {
            LOG_ERROR(SELFLOADER, "unknown address for function nid table 0x%08x", func_entry_table);
            return false;
        }

        library.library_nid = library_nid;
        library.library_name = (const char *) memory::get_pointer(library_name_addr);

        for (int i = 0; i < num_funcs; i++) {
            hle::FunctionImport function_import {};
            uint32_t *entry_data = (uint32_t *) memory::get_pointer(*entry_addr);

            if (!entry_data) {
                LOG_ERROR(SELFLOADER, "can't patch out entry address 0x%08x", *entry_addr);
                return false;
            }

            patch_function_import_data(entry_data);

            function_import.nid = *nid;
            function_import.entry_address = *entry_addr;
            nid++;
            entry_addr++;
            library.function_imports.push_back(function_import);
        }

        mod.function_import_libraries.push_back(library);
        import_start_addr += import_struct_size;
    }

    mod.library_name = modinfo->name;
    mod.module_nid = modinfo->module_nid;
    hle::add_runtime_module(mod);
    LOG_DEBUG(SELFLOADER, "Added runtime library functions to HLE");
    return true;
}

bool load_self_to_memory(const std::shared_ptr<SELFProgramData>& _data) {
    bool self_relocated;

    if (!_data) {
        return false;
    }

    uint32_t bottom_address = 0xFFFFFFFF;
    uint32_t top_address = 0;
    SELFProgramData *data = _data.get();
    sce_module_info_raw *modinfo;

    for (auto& i : data->segments) {
        if (i.can_load) {
            uint8_t protection_flags = 0;

            if (i.phdr.p_flags & PHDR_PF_R)
                protection_flags |= memory::PROTECTION_TYPE_READ;

            if (i.phdr.p_flags & PHDR_PF_W)
                protection_flags |= memory::PROTECTION_TYPE_WRITE;

            if (i.phdr.p_flags & PHDR_PF_X)
                protection_flags |= memory::PROTECTION_TYPE_EXECUTE;

            if (!i.relocatable) {
                uint32_t page_aligned_top_address_end = ((i.phdr.p_vaddr + i.phdr.p_memsz + 0x3FFF) & ~0x3FFF) - 1;
                if (bottom_address > i.phdr.p_vaddr)
                    bottom_address = i.phdr.p_vaddr;
                if (top_address < page_aligned_top_address_end)
                    top_address = page_aligned_top_address_end - 1;
                memory::map_memory(i.name, i.phdr.p_vaddr, page_aligned_top_address_end, protection_flags);
                memory::memcpy(i.phdr.p_vaddr, i.data.get(), i.segment_length);
            } else {
                LOG_WARN(SELFLOADER, "Relocatable memory region 0x%08x .. 0x%08x", i.phdr.p_vaddr, i.phdr.p_vaddr + (uint32_t)i.segment_length - 1);
            }
        }
    }

    if (data->relocatable) {
        int segment = data->entry >> 30;
        int offset = data->entry & 0x3FFFFFFF;
        modinfo = (sce_module_info_raw *) memory::get_pointer(data->segments[segment].phdr.p_vaddr + offset);
    } else {
        modinfo = nullptr;
        LOG_ERROR(SELFLOADER, "Unimplemented %s ET_SCE_EXEC", __func__);
        return false;
    }

    LOG_DEBUG(SELFLOADER, "%s: Found sce_module_info_raw module name performing relocations..", __func__);
    debug_module_info(modinfo);
    self_relocated = do_self_relocation_program_data(_data);
    if (!self_relocated) {
        LOG_ERROR(SELFLOADER, "Can't perform self relocations");
        memory::unmap_memory_all();
        return false;
    }

    add_runtime_library_functions(modinfo, data->relocatable ? bottom_address : 0);
    return true;
}
}