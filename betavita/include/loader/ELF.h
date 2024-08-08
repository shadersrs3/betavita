#ifndef _BETAVITA_ELF_H
#define _BETAVITA_ELF_H

#include <cstdint>

typedef uint32_t Elf32_Word;

#define ELF_FILE_SIGNATURE_LE ((uint32_t) (0x464C457F))
#define ELF_FILE_SIGNATURE_BE ((uint32_t) (0x7F454C46))
#define EI_CLASS_OFFSET (0x04)
#define EI_DATA_OFFSET (0x05)
#define EI_VERSION_OFFSET (0x06)
#define EI_OSABI_OFFSET (0x07)
#define EI_ABIVERSION_OFFSET (0x08)

#pragma pack(push, 1)

struct ELFHdr32 {
    uint8_t e_ident[16];              /* ELF identification */
    uint16_t e_type;                  /* object file type */
    uint16_t e_machine;               /* machine type */
    uint32_t e_version;               /* object file version */
    uint32_t e_entry;                 /* entry point address */
    uint32_t e_phoff;                 /* program header offset */
    uint32_t e_shoff;                 /* section header offset */
    uint32_t e_flags;                 /* processor-specific flags */
    uint16_t e_ehsize;                /* ELF header size */
    uint16_t e_phentsize;             /* size of program header entry */
    uint16_t e_phnum;                 /* number of program header entries */
    uint16_t e_shentsize;             /* size of section header entry */
    uint16_t e_shnum;                 /* number of section header entries */
    uint16_t e_shstrndx;              /* section name string table index */
};

#define PT_NULL 0x00000000
#define PT_LOAD 0x00000001

#define PHDR_PF_X 0x01
#define PHDR_PF_W 0x02
#define PHDR_PF_R 0x04

#define SHT_NULL 0x0
#define SHT_PROGBITS 0x1
#define SHT_SYMTAB 0x2
#define SHT_STRTAB 0x3
#define SHT_RELA 0x4
#define SHT_HASH 0x5
#define SHT_DYNAMIC 0x6
#define SHT_NOTE 0x7
#define SHT_NOBITS 0x8
#define SHT_REL 0x9
#define SHT_SHLIB 0x0A
#define SHT_DYNSYM 0x0B
#define SHT_SYMTAB_SHNDX 0x12
#define SHT_NUM 0x13
#define SHT_LOOS 0x60000000

struct ELFPhdr32 {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

struct ELFShdr32 {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

#pragma pack(pop)

#endif /* _BETAVITA_ELF_H */