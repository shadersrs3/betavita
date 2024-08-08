#ifndef _SCE_TYPE_H
#define _SCE_TYPE_H

#include <stdint.h>

typedef enum SceSelfType {
    SCE_SELF_TYPE_KERNEL   = 7,
    SCE_SELF_TYPE_NPDRM    = 8,
    SCE_SELF_TYPE_KBL      = 9,
    SCE_SELF_TYPE_SECURITY = 0xB,
    SCE_SELF_TYPE_USER     = 0xD
} SceSelfType;

// some info taken from the wiki, see https://www.psdevwiki.com/ps3/SELF_-_SPRX

#pragma pack(push, 1)
struct SCEHeader {
    uint32_t magic;                 /* 53434500 = SCE\0 */
    uint32_t version;               /* header version 3*/
    uint16_t sdk_type;              /* */
    uint16_t header_type;           /* 1 self, 2 unknown, 3 pkg */
    uint32_t metadata_offset;       /* metadata offset */
    uint64_t header_len;            /* self header length */
    uint64_t elf_filesize;          /* ELF file length */
    uint64_t self_filesize;         /* SELF file length */
    uint64_t unknown;               /* UNKNOWN */
    uint64_t self_offset;           /* SELF offset */
    uint64_t appinfo_offset;        /* app info offset */
    uint64_t elf_offset;            /* ELF #1 offset */
    uint64_t phdr_offset;           /* program header offset */
    uint64_t shdr_offset;           /* section header offset */
    uint64_t section_info_offset;   /* section info offset */
    uint64_t sceversion_offset;     /* version offset */
    uint64_t controlinfo_offset;    /* control info offset */
    uint64_t controlinfo_size;      /* control info size */
    uint64_t padding;
};

struct SCEAppInfo {
    uint64_t authid;                /* auth id */
    uint32_t vendor_id;             /* vendor id */
    uint32_t self_type;             /* app type */
    uint64_t version;               /* app version */
    uint64_t padding;               /* UNKNOWN */
};

struct SCEVersion {
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t unk4;
};

struct SCEControlInfo {
    uint32_t type;
    uint32_t size;
    uint32_t unk;
    uint32_t pad;
};

struct SCEControlInfo4 {
    SCEControlInfo common;
    uint8_t constant[0x14];
    uint8_t elf_digest[0x20];
    uint32_t padding;
    uint64_t min_required_fw;
};

struct SCEControlInfo5 {
    SCEControlInfo common;
    char unk[0x100];
}; // npdrm info

struct SCEControlInfo6 {
    SCEControlInfo common;
    uint32_t is_used;               /* always set to 1 */
    uint32_t attr;                  /* controls several app settings */
    uint32_t phycont_memsize;       /* physically contiguous memory budget */
    uint32_t total_memsize;         /* total memory budget (user + phycont) */
    uint32_t filehandles_limit;     /* max number of opened filehandles simultaneously */
    uint32_t dir_max_level;         /* max depth for directories support */
    uint32_t encrypt_mount_max;     /* UNKNOWN */
    uint32_t redirect_mount_max;    /* UNKNOWN */
    char unk[0xE0];
} SCE_controlinfo_6;

struct SCEControlInfo7 {
    SCEControlInfo common;
    char unk[0x40];
}; // shared secret

struct SegmentInfo {
    uint64_t offset;
    uint64_t length;
    uint64_t compression; // 1 = uncompressed, 2 = compressed
    uint64_t encryption; // 1 = encrypted, 2 = plain
};
#pragma pack(pop)

#endif