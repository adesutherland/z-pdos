/******************************************************************************
 * @file            macho.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdint.h>

struct mach_header_64_internal {
    unsigned long magic;
    unsigned long cputype;
    unsigned long cpusubtype;
    unsigned long filetype;
    unsigned long ncmds;
    unsigned long sizeofcmds;
    unsigned long flags;
    unsigned long reserved;
};

#define SIZEOF_struct_mach_header_64_file 32
struct mach_header_64_file {
    unsigned char magic[4];
    unsigned char cputype[4];
    unsigned char cpusubtype[4];
    unsigned char filetype[4];
    unsigned char ncmds[4];
    unsigned char sizeofcmds[4];
    unsigned char flags[4];
    unsigned char reserved[4];
};

#define MH_MAGIC_64 0xfeedfacf

#define CPU_TYPE_x86_64 0x01000007
#define CPU_TYPE_ARM64  0x0100000c

#define CPU_SUBTYPE_I386_ALL 3

#define MH_OBJECT 1
#define MH_EXECUTE 2
#define MH_FVMLIB 3
#define MH_CORE 4
#define MH_PRELOAD 5
#define MH_DYLIB 6
#define MH_DYLINKER 7
#define MH_BUNDLE 8
#define MH_DYLIB_STUB 9
#define MH_DSYM 10
#define MH_KEXT_BUNDLE 11

#define MH_NOUNDEFS 0x1
#define MH_INCRLINK 0x2
#define MH_DYLDLINK 0x4
#define MH_BINDATLOAD 0x8
#define MH_PREBOUND 0x10
#define MH_SPLIT_SEGS 0x20
#define MH_LAZY_INIT 0x40
#define MH_TWOLEVEL 0x80
#define MH_FORCE_FLAT 0x100
#define MH_NOMULTIDEFS 0x200
#define MH_NOFIXPREBINDING 0x400
#define MH_PREBINDABLE 0x800
#define MH_ALLMODSBOUND 0x1000
#define MH_SUBSECTIONS_VIA_SYMBOLS 0x2000
#define MH_CANONICAL 0x4000
#define MH_WEAK_DEFINES 0x8000
#define MH_BINDS_TO_WEAK 0x10000
#define MH_ALLOW_STACK_EXECUTION 0x20000
#define MH_ROOT_SAFE 0x40000
#define MH_SETUID_SAFE 0x80000
#define MH_NO_REEXPORTED_DYLIBS 0x100000
#define MH_PIE 0x200000
#define MH_DEAD_STRIPPABLE_DYLIB 0x400000
#define MH_HAS_TLV_DESCRIPTORS 0x800000
#define MH_NO_HEAP_EXECUTION 0x1000000
#define MH_APP_EXTENSION_SAFE 0x2000000
#define MH_NLIST_OUTOFSYNC_WITH_DYLDINFO 0x4000000
#define MH_SIM_SUPPORT 0x8000000

struct load_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
};

#define SIZEOF_struct_load_command_file 8
struct load_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
};

#define LC_SEGMENT_64 0x19
#define LC_SYMTAB 0x2
#define LC_UNIXTHREAD 0x5
#define LC_DYSYMTAB 0xb
#define LC_LOAD_DYLIB 0xc
#define LC_LOAD_DYLINKER 0xe
#define LC_VERSION_MIN 0x24
#define LC_FUNCTION_STARTS 0x26
#define LC_MAIN (0x80000000 | 0x28)
#define LC_DATA_IN_CODE 0x29
#define LC_SOURCE_VERSION 0x2a
#define LC_BUILD_VERSION 0x32
#define LC_DYLD_EXPORTS_TRIE (0x80000000 | 0x33)
#define LC_DYLD_CHAINED_FIXUPS (0x80000000 | 0x34)

struct segment_command_64_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    char segname[16];
    uint_fast64_t vmaddr;
    uint_fast64_t vmsize;
    uint_fast64_t fileoff;
    uint_fast64_t filesize;
    unsigned long maxprot;
    unsigned long initprot;
    unsigned long nsects;
    unsigned long flags;
};

#define SIZEOF_struct_segment_command_64_file 72
struct segment_command_64_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    char segname[16];
    unsigned char vmaddr[8];
    unsigned char vmsize[8];
    unsigned char fileoff[8];
    unsigned char filesize[8];
    unsigned char maxprot[4];
    unsigned char initprot[4];
    unsigned char nsects[4];
    unsigned char flags[4];
};

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXECUTE 0x4

struct section_64_internal {
    char sectname[16];
    char segname[16];
    uint_fast64_t addr;
    uint_fast64_t size;
    unsigned long offset;
    unsigned long align;
    unsigned long reloff;
    unsigned long nreloc;
    unsigned long flags;
    unsigned long reserved1;
    unsigned long reserved2;
    unsigned long reserved3;
};

#define SIZEOF_struct_section_64_file 80
struct section_64_file {
    char sectname[16];
    char segname[16];
    unsigned char addr[8];
    unsigned char size[8];
    unsigned char offset[4];
    unsigned char align[4];
    unsigned char reloff[4];
    unsigned char nreloc[4];
    unsigned char flags[4];
    unsigned char reserved1[4];
    unsigned char reserved2[4];
    unsigned char reserved3[4];
};

#define S_REGULAR 0x400
#define S_ZEROFILL 0x1
#define S_CSTRING_LITERALS 0x2
#define S_4BYTE_LITERALS 0x3
#define S_8BYTE_LITERALS 0x4
#define S_LITERAL_POINTERS 0x5
#define S_NON_LAZY_SYMBOL_POINTERS 0x6
#define S_LAZY_SYMBOL_POINTERS 0x7
#define S_SYMBOL_STUBS 0x8
#define S_MOD_INIT_FUNC_POINTERS 0x9
#define S_MOD_TERM_FUNC_POINTERS 0xa
#define S_COALESCED 0xb
#define S_GB_ZEROFILL 0xc
#define S_INTERPOSING 0xd
#define S_16BYTE_LITERALS 0xe
#define S_DTRACE_DOF 0xf
#define S_LAZY_DYLIB_SYMBOL_POINTERS 0x10
#define S_THREAD_LOCAL_REGULAR 0x11
#define S_THREAD_LOCAL_ZEROFILL 0x12
#define S_THREAD_LOCAL_VARIABLES 0x13
#define S_THREAD_LOCAL_VARIABLE_POINTERS 0x14
#define S_THREAD_LOCAL_INIT_FUNCTION_POINTERS 0x15
#define S_INIT_FUNC_OFFSETS 0x16

/* Relocation info is the same as for a.out, only the bitfield is different.
 * Originally r_address is not unsigned
 * but supporting negative values for it does not make sense,
 * so it is changed to unsigned here.
 */
struct relocation_info_internal {
    unsigned long r_address;
    unsigned long r_symbolnum;
    /* r_symbolnum is a bitfield but for portability it is accessed manually:
     * r_symbolnum : 24,
     * r_pcrel     : 1,
     * r_length    : 2,
     * r_extern    : 1,
     * r_type      : 4;
     */
};

#define SIZEOF_struct_relocation_info_file 8
struct relocation_info_file {
    unsigned char r_address[4];
    unsigned char r_symbolnum[4];
};

#define GENERIC_RELOC_INVALID 255
#define GENERIC_RELOC_VANILLA 0
#define GENERIC_RELOC_PAIR 1
#define GENERIC_RELOC_SECTDIFF 2
#define GENERIC_RELOC_PB_LA_PTR 3
#define GENERIC_RELOC_LOCAL_SECTDIFF 4
#define GENERIC_RELOC_TLV 5
#define PPC_RELOC_VANILLA 0
#define PPC_RELOC_PAIR 1
#define PPC_RELOC_BR14 2
#define PPC_RELOC_BR24 3
#define PPC_RELOC_HI16 4
#define PPC_RELOC_LO16 5
#define PPC_RELOC_HA16 6
#define PPC_RELOC_LO14 7
#define PPC_RELOC_SECTDIFF 8
#define PPC_RELOC_PB_LA_PTR 9
#define PPC_RELOC_HI16_SECTDIFF 10
#define PPC_RELOC_LO16_SECTDIFF 11
#define PPC_RELOC_HA16_SECTDIFF 12
#define PPC_RELOC_JBSR 13
#define PPC_RELOC_LO14_SECTDIFF 14
#define PPC_RELOC_LOCAL_SECTDIFF 15
#define ARM_RELOC_VANILLA 0
#define ARM_RELOC_PAIR 1
#define ARM_RELOC_SECTDIFF 2
#define ARM_RELOC_LOCAL_SECTDIFF 3
#define ARM_RELOC_PB_LA_PTR 4
#define ARM_RELOC_BR24 5
#define ARM_THUMB_RELOC_BR22 6
#define ARM_THUMB_32BIT_BRANCH 7
#define ARM_RELOC_HALF 8
#define ARM_RELOC_HALF_SECTDIFF 9
#define ARM64_RELOC_UNSIGNED 0
#define ARM64_RELOC_SUBTRACTOR 1
#define ARM64_RELOC_BRANCH26 2
#define ARM64_RELOC_PAGE21 3
#define ARM64_RELOC_PAGEOFF12 4
#define ARM64_RELOC_GOT_LOAD_PAGE21 5
#define ARM64_RELOC_GOT_LOAD_PAGEOFF12 6
#define ARM64_RELOC_POINTER_TO_GOT 7
#define ARM64_RELOC_TLVP_LOAD_PAGE21 8
#define ARM64_RELOC_TLVP_LOAD_PAGEOFF12 9
#define ARM64_RELOC_ADDEND 10
#define X86_64_RELOC_UNSIGNED 0
#define X86_64_RELOC_SIGNED 1
#define X86_64_RELOC_BRANCH 2
#define X86_64_RELOC_GOT_LOAD 3
#define X86_64_RELOC_GOT 4
#define X86_64_RELOC_SUBTRACTOR 5
#define X86_64_RELOC_SIGNED_1 6
#define X86_64_RELOC_SIGNED_2 7
#define X86_64_RELOC_SIGNED_4 8
#define X86_64_RELOC_TLV 9

struct symtab_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long symoff;
    unsigned long nsyms;
    unsigned long stroff;
    unsigned long strsize;
};

#define SIZEOF_struct_symtab_command_file 24
struct symtab_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char symoff[4];
    unsigned char nsyms[4];
    unsigned char stroff[4];
    unsigned char strsize[4];
};

struct nlist_64_internal {
    unsigned long n_strx;
    unsigned char n_type;
    unsigned char n_sect;
    unsigned short n_desc;
    uint_fast64_t n_value;
};

#define SIZEOF_struct_nlist_64_file 16
struct nlist_64_file {
    unsigned char n_strx[4];
    unsigned char n_type[1];
    unsigned char n_sect[1];
    unsigned char n_desc[2];
    unsigned char n_value[8];
};

#define N_STAB 0xe0
#define N_PEXT 0x10
#define N_TYPE 0x0e
#define N_EXT 0x01

#define N_UNDF 0x0
#define N_ABS 0x2
#define N_SECT 0xe
#define N_PUBD 0xc
#define N_INDR 0xa

#define NO_SECT 0

struct unixthread_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long flavor;
    unsigned long count;
};

#define SIZEOF_struct_unixthread_command_file 16
struct unixthread_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char flavor[4];
    unsigned char count[4];
};

struct x86_thread_state64_t_internal {
    uint_fast64_t rax;
    uint_fast64_t rbx;
    uint_fast64_t rcx;
    uint_fast64_t rdx;
    uint_fast64_t rdi;
    uint_fast64_t rsi;
    uint_fast64_t rbp;
    uint_fast64_t rsp;
    uint_fast64_t r8;
    uint_fast64_t r9;
    uint_fast64_t r10;
    uint_fast64_t r11;
    uint_fast64_t r12;
    uint_fast64_t r13;
    uint_fast64_t r14;
    uint_fast64_t r15;
    uint_fast64_t rip;
    uint_fast64_t rflags;
    uint_fast64_t cs;
    uint_fast64_t fs;
    uint_fast64_t gs;
};

#define SIZEOF_struct_x86_thread_state64_t_file 168
struct x86_thread_state64_t_file {
    unsigned char rax[8];
    unsigned char rbx[8];
    unsigned char rcx[8];
    unsigned char rdx[8];
    unsigned char rdi[8];
    unsigned char rsi[8];
    unsigned char rbp[8];
    unsigned char rsp[8];
    unsigned char r8[8];
    unsigned char r9[8];
    unsigned char r10[8];
    unsigned char r11[8];
    unsigned char r12[8];
    unsigned char r13[8];
    unsigned char r14[8];
    unsigned char r15[8];
    unsigned char rip[8];
    unsigned char rflags[8];
    unsigned char cs[8];
    unsigned char fs[8];
    unsigned char gs[8];
};

struct dysymtab_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long ilocalsym;
    unsigned long nlocalsym;
    unsigned long iextdefsym;
    unsigned long nextdefsym;
    unsigned long iundefsym;
    unsigned long nundefsym;
    unsigned long tocoff;
    unsigned long ntoc;
    unsigned long modtaboff;
    unsigned long nmodtab;
    unsigned long extrefsymoff;
    unsigned long nextrefsyms;
    unsigned long indirectsymoff;
    unsigned long nindirectsyms;
    unsigned long extreloff;
    unsigned long nextrel;
    unsigned long locreloff;
    unsigned long nlocrel;
};

#define SIZEOF_struct_dysymtab_command_file 80
struct dysymtab_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char ilocalsym[4];
    unsigned char nlocalsym[4];
    unsigned char iextdefsym[4];
    unsigned char nextdefsym[4];
    unsigned char iundefsym[4];
    unsigned char nundefsym[4];
    unsigned char tocoff[4];
    unsigned char ntoc[4];
    unsigned char modtaboff[4];
    unsigned char nmodtab[4];
    unsigned char extrefsymoff[4];
    unsigned char nextrefsyms[4];
    unsigned char indirectsymoff[4];
    unsigned char nindirectsyms[4];
    unsigned char extreloff[4];
    unsigned char nextrel[4];
    unsigned char locreloff[4];
    unsigned char nlocrel[4];
};

struct dylib_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long name_offset;
    unsigned long timestamp;
    unsigned long current_version;
    unsigned long compatibility_version;
};

#define SIZEOF_struct_dylib_command_file 24
struct dylib_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char name_offset[4];
    unsigned char timestamp[4];
    unsigned char current_version[4];
    unsigned char compatibility_version[4];
};

struct dylinker_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long name_offset;
};

#define SIZEOF_struct_dylinker_command_file 12
struct dylinker_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char name_offset[4];
};

struct version_min_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long version;
    unsigned long sdk;
};

#define SIZEOF_struct_version_min_command_file 16
struct version_min_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char version[4];
    unsigned char sdk[4];
};

struct function_starts_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long dataoff;
    unsigned long datasize;
};

#define SIZEOF_struct_function_starts_command_file 16
struct function_starts_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char dataoff[4];
    unsigned char datasize[4];
};

struct main_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    uint_fast64_t entryoff;
    uint_fast64_t stacksize;
};

#define SIZEOF_struct_main_command_file 24
struct main_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char entryoff[8];
    unsigned char stacksize[8];
};

struct data_in_code_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long dataoff;
    unsigned long datasize;
};

#define SIZEOF_struct_data_in_code_command_file 16
struct data_in_code_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char dataoff[4];
    unsigned char datasize[4];
};

struct source_version_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    uint_fast64_t version;
};

#define SIZEOF_struct_source_version_command_file 16
struct source_version_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char version[8];
};

struct build_version_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long platform;
    unsigned long minos;
    unsigned long sdk;
    unsigned long ntools;
    unsigned long unknown1;
    unsigned long unknown2;
};

#define SIZEOF_struct_build_version_command_file 32
struct build_version_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char platform[4];
    unsigned char minos[4];
    unsigned char sdk[4];
    unsigned char ntools[4];
    unsigned char unknown1[4];
    unsigned char unknown2[4];
};

struct dyld_exports_trie_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long dataoff;
    unsigned long datasize;
};

#define SIZEOF_struct_dyld_exports_trie_command_file 16
struct dyld_exports_trie_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char dataoff[4];
    unsigned char datasize[4];
};

struct dyld_chained_fixups_command_internal {
    unsigned long cmd;
    unsigned long cmdsize;
    unsigned long dataoff;
    unsigned long datasize;
};

#define SIZEOF_struct_dyld_chained_fixups_command_file 16
struct dyld_chained_fixups_command_file {
    unsigned char cmd[4];
    unsigned char cmdsize[4];
    unsigned char dataoff[4];
    unsigned char datasize[4];
};

struct dyld_chained_fixups_header_internal {
    unsigned long fixups_version; /* 0. */
    unsigned long starts_offset;
    unsigned long imports_offset;
    unsigned long symbols_offset;
    unsigned long imports_counts;
    unsigned long imports_format;
    unsigned long symbols_format; /* 0 - uncommpressed, 1 - compressed. */
    unsigned long padding;
};

#define SIZEOF_struct_dyld_chained_fixups_header_file 32
struct dyld_chained_fixups_header_file {
    unsigned char fixups_version[4];
    unsigned char starts_offset[4];
    unsigned char imports_offset[4];
    unsigned char symbols_offset[4];
    unsigned char imports_counts[4];
    unsigned char imports_format[4];
    unsigned char symbols_format[4];
    unsigned char padding[4];
};

#define DYLD_CHAINED_IMPORT 1
#define DYLD_CHAINED_IMPORT_ADDEND 2
#define DYLD_CHAINED_IMPORT_ADDEND64 3

struct dyld_chained_starts_in_image_internal {
    unsigned long seg_count;
    unsigned long seg_info_offset[1];
};

#define SIZEOF_struct_dyld_chained_starts_in_image_file 8
struct dyld_chained_starts_in_image_file {
    unsigned char seg_count[4];
    unsigned char seg_info_offset[4][1];
};

struct dyld_chained_starts_in_segment_internal {
    unsigned long size;
    unsigned short page_size;
    unsigned short pointer_format;
    uint_fast64_t segment_offset;
    unsigned long max_valid_pointer;
    unsigned short page_count;
    unsigned short page_start/*[1]*/;
};

#define SIZEOF_struct_dyld_chained_starts_in_segment_file 24
struct dyld_chained_starts_in_segment_file {
    unsigned char size[4];
    unsigned char page_size[2];
    unsigned char pointer_format[2];
    unsigned char segment_offset[8];
    unsigned char max_valid_pointer[4];
    unsigned char page_count[2];
    unsigned char page_start[2]/*[1]*/;
};

#define DYLD_CHAINED_PTR_ARM64E 1
#define DYLD_CHAINED_PTR_64 2
#define DYLD_CHAINED_PTR_32 3
#define DYLD_CHAINED_PTR_32_CACHE 4
#define DYLD_CHAINED_PTR_32_FIRMWARE 5
#define DYLD_CHAINED_PTR_64_OFFSET 6
#define DYLD_CHAINED_PTR_ARM64E_OFFSET 7
#define DYLD_CHAINED_PTR_ARM64E_KERNEL 7
#define DYLD_CHAINED_PTR_64_KERNEL_CACHE 8
#define DYLD_CHAINED_PTR_ARM64E_USERLAND 9
#define DYLD_CHAINED_PTR_ARM64E_FIRMWARE 10
#define DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE 11
#define DYLD_CHAINED_PTR_ARM64E_USERLAND24 12

#define DYLD_CHAINED_PTR_START_NONE 0xFFFF
#define DYLD_CHAINED_PTR_START_MULTI 0x8000
#define DYLD_CHAINED_PTR_START_LAST 0x8000

/* The following struct is not used directly,
 * it is here for reference for manual bitfield manipulation.
 */
#if 0
/* DYLD_CHAINED_PTR_64_OFFSET */
struct dyld_chained_ptr_64_rebase {
    uint_fast64_t target : 36; /* Original value without base address. */
    uint_fast64_t high8 : 8; /* Not sure, leave 0. */
    uint_fast64_t reserved : 7; /* 0. */
    uint_fast64_t next : 12; /* 4-byte stride. */
    uint_fast64_t bind : 1; /* 0. */
};
#endif
