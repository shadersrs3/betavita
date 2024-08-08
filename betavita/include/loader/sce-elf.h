
#ifndef SCE_ELF_H
#define SCE_ELF_H

#include <loader/ELF.h>

/* SCE-specific definitions for e_type: */
#define ET_SCE_EXEC         0xFE00		/* SCE Executable file */
#define ET_SCE_RELEXEC      0xFE04		/* SCE Relocatable file */
#define ET_SCE_STUBLIB      0xFE0C		/* SCE SDK Stubs */
#define ET_SCE_DYNAMIC      0xFE18		/* Unused */
#define ET_SCE_PSPRELEXEC   0xFFA0		/* Unused (PSP ELF only) */
#define ET_SCE_PPURELEXEC   0xFFA4		/* Unused (SPU ELF only) */
#define ET_SCE_RELARMEXEC   0xFFA5		/* Unknown */

/* SCE-specific definitions for sh_type: */
#define SHT_SCE_RELA       0x60000000	/* SCE Relocations */
#define SHT_SCENID         0x61000001	/* Unused (PSP ELF only) */
#define SHT_SCE_PSPRELA    0x700000A0	/* Unused (PSP ELF only) */
#define SHT_SCE_ARMRELA    0x700000A4	/* Unused (PSP ELF only) */

/* SCE-specific definitions for p_type: */
#define PT_SCE_RELA       0x60000000	/* SCE Relocations */
#define PT_SCE_COMMENT    0x6FFFFF00	/* Unused */
#define PT_SCE_VERSION    0x6FFFFF01	/* Unused */
#define PT_SCE_UNK        0x70000001	/* Unknown */
#define PT_SCE_PSPRELA    0x700000A0	/* Unused (PSP ELF only) */
#define PT_SCE_PPURELA    0x700000A4	/* Unused (SPU ELF only) */

#define NID_MODULE_STOP         0x79F8E492
#define NID_MODULE_EXIT         0x913482A9
#define NID_MODULE_START        0x935CD196
#define NID_MODULE_BOOTSTART    0x5C424D40
#define NID_MODULE_INFO         0x6C2224BA
#define NID_PROCESS_PARAM       0x70FBA1E7
#define NID_MODULE_SDK_VERSION  0x936C8A78

#define PSP2_SDK_VERSION 0x03570011

typedef union {
	Elf32_Word r_short : 4;
	struct {
		Elf32_Word r_short     : 4;
		Elf32_Word r_symseg    : 4;
		Elf32_Word r_code      : 8;
		Elf32_Word r_datseg    : 4;
		Elf32_Word r_offset_lo : 12;
		Elf32_Word r_offset_hi : 20;
		Elf32_Word r_addend    : 12;
	} r_short_entry;
	struct {
		Elf32_Word r_short     : 4;
		Elf32_Word r_symseg    : 4;
		Elf32_Word r_code      : 8;
		Elf32_Word r_datseg    : 4;
		Elf32_Word r_code2     : 8;
		Elf32_Word r_dist2     : 4;
		Elf32_Word r_addend;
		Elf32_Word r_offset;
	} r_long_entry;
	struct {
		Elf32_Word r_short     : 4; /* 0x1 */
		Elf32_Word r_datseg    : 4;
		Elf32_Word r_code      : 8;
		Elf32_Word r_addend    : 16;
		Elf32_Word r_offset;
	} r_variable_short_entry;
	struct {
		Elf32_Word r_short     : 4; /* 0x2 */
		Elf32_Word r_datseg    : 4;
		Elf32_Word r_code      : 8;
		Elf32_Word r_pad       : 16;
		Elf32_Word r_offset;
		Elf32_Word r_addend;
	} r_variable_long_entry;
	struct {
		Elf32_Word r_word1;
		Elf32_Word r_word2;
		Elf32_Word r_word3;
	} r_raw_entry;
} SCE_Rel;

#define SCE_ELF_DEFS_HOST
#include "sce-elf-defs.h"
#undef SCE_ELF_DEFS_HOST

#define SCE_ELF_DEFS_TARGET
#include "sce-elf-defs.h"
#undef SCE_ELF_DEFS_TARGET

#endif