/******************************************************************************
 * @file            elf.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdint.h>

/* Fixed size data types. All of them except Elf32_Half must be 4 bytes. */
typedef unsigned long Elf32_Addr;
typedef unsigned long Elf32_Off;
typedef unsigned long Elf32_Word;
typedef signed long Elf32_Sword;
typedef unsigned short Elf32_Half; /* 2 bytes. */

/* Fixed size data types, short should be 2 bytes, int 4 bytes. */
typedef uint_fast64_t Elf64_Addr;
typedef uint_fast64_t Elf64_Off;
typedef unsigned short Elf64_Half;
typedef unsigned int Elf64_Word;
typedef signed int Elf64_Sword;
typedef uint_fast64_t Elf64_Xword;
typedef int_fast64_t Elf64_Sxword;

#define EI_NIDENT 16 /* Size of e_ident on all systems. */

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

struct Elf32_Ehdr_internal {
    unsigned char e_ident[EI_NIDENT];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned long e_version;
    unsigned long e_entry;
    unsigned long e_phoff;
    unsigned long e_shoff;
    unsigned long e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
};

#define SIZEOF_struct_Elf32_Ehdr_file 52
struct Elf32_Ehdr_file {
    unsigned char e_ident[EI_NIDENT];
    unsigned char e_type[2];
    unsigned char e_machine[2];
    unsigned char e_version[4];
    unsigned char e_entry[4];
    unsigned char e_phoff[4];
    unsigned char e_shoff[4];
    unsigned char e_flags[4];
    unsigned char e_ehsize[2];
    unsigned char e_phentsize[2];
    unsigned char e_phnum[2];
    unsigned char e_shentsize[2];
    unsigned char e_shnum[2];
    unsigned char e_shstrndx[2];
};

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

struct Elf64_Ehdr_internal {
    unsigned char e_ident[EI_NIDENT];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned long e_version;
    uint_fast64_t e_entry;
    uint_fast64_t e_phoff;
    uint_fast64_t e_shoff;
    unsigned long e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
};

#define SIZEOF_struct_Elf64_Ehdr_file 64
struct Elf64_Ehdr_file {
    unsigned char e_ident[EI_NIDENT];
    unsigned char e_type[2];
    unsigned char e_machine[2];
    unsigned char e_version[4];
    unsigned char e_entry[8];
    unsigned char e_phoff[8];
    unsigned char e_shoff[8];
    unsigned char e_flags[4];
    unsigned char e_ehsize[2];
    unsigned char e_phentsize[2];
    unsigned char e_phnum[2];
    unsigned char e_shentsize[2];
    unsigned char e_shnum[2];
    unsigned char e_shstrndx[2];
};

/* e_ident index and value macros. */
#define EI_MAG0 0
#define ELFMAG0 0x7f
#define EI_MAG1 1
#define ELFMAG1 'E'
#define EI_MAG2 2
#define ELFMAG2 'L'
#define EI_MAG3 3
#define ELFMAG3 'F'
#define EI_CLASS 4
    #define ELFCLASSNONE 0 /* Invalid class. */
    #define ELFCLASS32 1 /* 32-bit object. */
    #define ELFCLASS64 2 /* 64-bit object. */
#define EI_DATA 5
    #define ELFDATANONE 0 /* Invalid data encoding. */
    #define ELFDATA2LSB 1 /* Little-endian encoding. */
    #define ELFDATA2MSB 2 /* Big-endian encoding. */
#define EI_VERSION 6 /* Must be EV_CURRENT. */
#define EI_OSABI 7
    #define ELFOSABI_NONE 0 /* No specific extensions. */
    #define ELFOSABI_LINUX 3
    #define ELFOSABI_ARM 97
#define EI_ABIVERSION 8 /* Should be set to 0. */
#define EI_PAD 9 /* Start of pad bytes. Should be ignored. */

/* e_type */
#define ET_NONE 0
#define ET_REL 1 /* Relocatable file. */
#define ET_EXEC 2 /* Executable file. */
#define ET_DYN 3 /* Shared object file. */
#define ET_CORE 4 /* Core file. */

/* e_machine */
#define EM_NONE 0
#define EM_386 3 /* Intel 80386. */
#define EM_68K 4
#define EM_S370 9
#define EM_S390 22
#define EM_ARM 40
#define EM_X86_64 62 /* x86-64. */
#define EM_AARCH64 183
#define EM_LOONGARCH 258

/* e_version */
#define EV_NONE 0
#define EV_CURRENT 1

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

struct Elf32_Shdr_internal {
    unsigned long sh_name;
    unsigned long sh_type;
    unsigned long sh_flags;
    unsigned long sh_addr;
    unsigned long sh_offset;
    unsigned long sh_size;
    unsigned long sh_link;
    unsigned long sh_info;
    unsigned long sh_addralign;
    unsigned long sh_entsize;
};

#define SIZEOF_struct_Elf32_Shdr_file 40
struct Elf32_Shdr_file {
    unsigned char sh_name[4];
    unsigned char sh_type[4];
    unsigned char sh_flags[4];
    unsigned char sh_addr[4];
    unsigned char sh_offset[4];
    unsigned char sh_size[4];
    unsigned char sh_link[4];
    unsigned char sh_info[4];
    unsigned char sh_addralign[4];
    unsigned char sh_entsize[4];
};

typedef struct {
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} Elf64_Shdr;

struct Elf64_Shdr_internal {
    unsigned long sh_name;
    unsigned long sh_type;
    uint_fast64_t sh_flags;
    uint_fast64_t sh_addr;
    uint_fast64_t sh_offset;
    uint_fast64_t sh_size;
    unsigned long sh_link;
    unsigned long sh_info;
    uint_fast64_t sh_addralign;
    uint_fast64_t sh_entsize;
};

#define SIZEOF_struct_Elf64_Shdr_file 64
struct Elf64_Shdr_file {
    unsigned char sh_name[4];
    unsigned char sh_type[4];
    unsigned char sh_flags[8];
    unsigned char sh_addr[8];
    unsigned char sh_offset[8];
    unsigned char sh_size[8];
    unsigned char sh_link[4];
    unsigned char sh_info[4];
    unsigned char sh_addralign[8];
    unsigned char sh_entsize[8];
};

/* Special section indexes. */
#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff
#define SHN_XINDEX 0xffff

/* Section types. */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11

/* Section flags. */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MERGE 0x10
#define SHF_STRINGS 0x20
#define SHF_INFO_LINK 0x40
#define SHF_LINK_ORDER 0x80
#define SHF_OS_NONCOMFORMING 0x100
#define SHF_GROUP 0x200
#define SHF_TLS 0x400
#define SHF_MASKOS 0x0ff00000
#define SHF_ORDERED 0x40000000
#define SHF_EXCLUDE 0x80000000
#define SHF_MASKPROC 0xf0000000

typedef struct {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;
} Elf32_Sym;

struct Elf32_Sym_internal {
    unsigned long st_name;
    unsigned long st_value;
    unsigned long st_size;
    unsigned char st_info;
    unsigned char st_other;
    unsigned short st_shndx;
};

#define SIZEOF_struct_Elf32_Sym_file 16
struct Elf32_Sym_file {
    unsigned char st_name[4];
    unsigned char st_value[4];
    unsigned char st_size[4];
    unsigned char st_info[1];
    unsigned char st_other[1];
    unsigned char st_shndx[2];
};

typedef struct {
    Elf64_Word st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

struct Elf64_Sym_internal {
    unsigned long st_name;
    unsigned char st_info;
    unsigned char st_other;
    unsigned short st_shndx;
    uint_fast64_t st_value;
    uint_fast64_t st_size;
};

#define SIZEOF_struct_Elf64_Sym_file 24
struct Elf64_Sym_file {
    unsigned char st_name[4];
    unsigned char st_info[1];
    unsigned char st_other[1];
    unsigned char st_shndx[2];
    unsigned char st_value[8];
    unsigned char st_size[8];
};

#define STN_UNDEF 0

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

#define ELF64_ST_BIND ELF32_ST_BIND
#define ELF64_ST_TYPE ELF32_ST_TYPE
#define ELF64_ST_INFO ELF32_ST_INFO

/* Symbol binding. */
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

/* Symbol types. */
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

struct Elf32_Rel_internal {
    unsigned long r_offset;
    unsigned long r_info;
};

#define SIZEOF_struct_Elf32_Rel_file 8
struct Elf32_Rel_file {
    unsigned char r_offset[4];
    unsigned char r_info[4];
};

typedef struct {
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
} Elf64_Rel;

struct Elf64_Rel_internal {
    uint_fast64_t r_offset;
    uint_fast64_t r_info;
};

#define SIZEOF_struct_Elf64_Rel_file 16
struct Elf64_Rel_file {
    unsigned char r_offset[8];
    unsigned char r_info[8];
};

typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
    Elf32_Sword r_addend;
} Elf32_Rela;

struct Elf32_Rela_internal {
    unsigned long r_offset;
    unsigned long r_info;
    long r_addend;
};

#define SIZEOF_struct_Elf32_Rela_file 12
struct Elf32_Rela_file {
    unsigned char r_offset[4];
    unsigned char r_info[4];
    unsigned char r_addend[4];
};

typedef struct {
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
} Elf64_Rela;

struct Elf64_Rela_internal {
    uint_fast64_t r_offset;
    uint_fast64_t r_info;
    int_fast64_t r_addend;
};

#define SIZEOF_struct_Elf64_Rela_file 24
struct Elf64_Rela_file {
    unsigned char r_offset[8];
    unsigned char r_info[8];
    unsigned char r_addend[8];
};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffff)
#define ELF64_R_INFO(s,t) (((s) << 32)+((t) & 0xffffffff))

/* Relocation types (bottom byte of r_info). */
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF 9
#define R_386_GOTPC 10

#define R_68K_NONE 0
#define R_68K_32 1
#define R_68K_16 2
#define R_68K_8 3
#define R_68K_PC32 4
#define R_68K_PC16 5
#define R_68K_PC8 6
#define R_68K_GOT32 7
#define R_68K_GOT16 8
#define R_68K_GOT8 9
#define R_68K_GOT32O 10
#define R_68K_GOT16O 11
#define R_68K_GOT8O 12
#define R_68K_PLT32 13
#define R_68K_PLT16 14
#define R_68K_PLT8 15
#define R_68K_PLT32O 16
#define R_68K_PLT16O 17
#define R_68K_PLT8O 18
#define R_68K_COPY 19
#define R_68K_GLOB_DAT 20
#define R_68K_JMP_SLOT 21
#define R_68K_RELATIVE 22
#define R_68K_NUM 23

#define R_390_NONE 0
#define R_390_8 1
#define R_390_12 2
#define R_390_16 3
#define R_390_32 4
#define R_390_PC32 5
#define R_390_GOT12 6
#define R_390_GOT32 7
#define R_390_PLT32 8
#define R_390_COPY 9
#define R_390_GLOB_DAT 10
#define R_390_JMP_SLOT 11
#define R_390_RELATIVE 12
#define R_390_GOTOFF 13
#define R_390_GOTPC 14
#define R_390_GOT16 15
#define R_390_PC16 16
#define R_390_PC16DBL 17
#define R_390_PLT16DBL 18
#define R_390_PC32DBL 19
#define R_390_PLT32DBL 20
#define R_390_GOTPCDBL 21
#define R_390_64 22
#define R_390_PC64 23
#define R_390_GOT64 24
#define R_390_PLT64 25
#define R_390_GOTEN 26
#define R_390_GOTOFF16 27
#define R_390_GOTOFF64 28
#define R_390_GOTPLT12 29
#define R_390_GOTPLT16 30
#define R_390_GOTPLT32 31
#define R_390_GOTPLT64 32
#define R_390_GOTPLTENT 33
#define R_390_PLTOFF16 34
#define R_390_PLTOFF32 35
#define R_390_PLTOFF64 36
#define R_390_TLS_LOAD 37
#define R_390_TLS_GDCALL 38
#define R_390_TLS_LDCALL 39
#define R_390_TLS_GD64 41
#define R_390_TLS_GOTIE12 42
#define R_390_TLS_GOTIE64 44
#define R_390_TLS_LDM64 46
#define R_390_TLS_IE64 48
#define R_390_TLS_IEENT 49
#define R_390_TLS_LE64 51
#define R_390_TLS_LDO64 53
#define R_390_TLS_DTPMOD 54
#define R_390_TLS_DTPOFF 55
#define R_390_TLS_TPOFF 56
#define R_390_20 57
#define R_390_GOT20 58
#define R_390_GOTPLT20 59
#define R_390_TLS_GOTIE20 60
#define R_390_IRELATIVE 61
#define R_390_PC12DBL 62
#define R_390_PLT12DBL 63
#define R_390_PC24DBL 64
#define R_390_PLT24DBL 65

#define R_ARM_NONE 0
#define R_ARM_PC24 1
#define R_ARM_ABS32 2
#define R_ARM_REL32 3
#define R_ARM_LDR_PC_G0 4
#define R_ARM_ABS16 5
#define R_ARM_ABS12 6
#define R_ARM_THM_ABS5 7
#define R_ARM_ABS8 8
#define R_ARM_SBREL32 9
#define R_ARM_THM_CALL 10
#define R_ARM_THM_PC8 11
#define R_ARM_BREL_ADJ 12
#define R_ARM_TLS_DESC 13
#define R_ARM_TLS_DTPMOD32 17
#define R_ARM_TLS_DTPOFF32 18
#define R_ARM_TLS_TPOFF32 19
#define R_ARM_COPY 20
#define R_ARM_GLOB_DAT 21
#define R_ARM_JUMP_SLOT 22
#define R_ARM_RELATIVE 23
#define R_ARM_GOTOFF32 24
#define R_ARM_BASE_PREL 25
#define R_ARM_GOT_BREL 26
#define R_ARM_CALL 28
#define R_ARM_JUMP24 29
#define R_ARM_THM_JUMP24 30
#define R_ARM_BASE_ABS 31
#define R_ARM_TARGET1 38
#define R_ARM_V4BX 40
#define R_ARM_TARGET2 41
#define R_ARM_PREL31 42
#define R_ARM_MOVW_ABS_NC 43
#define R_ARM_MOVT_ABS 44
#define R_ARM_MOVW_PREL_NC 45
#define R_ARM_MOVT_PREL 46
#define R_ARM_THM_MOVW_ABS_NC 47
#define R_ARM_THM_MOVT_ABS 48
#define R_ARM_THM_MOVW_PREL_NC 49
#define R_ARM_THM_MOVT_PREL 50
#define R_ARM_THM_JUMP19 51
#define R_ARM_THM_JUMP6 52
#define R_ARM_THM_ALU_PREL_11_0 53
#define R_ARM_THM_PC12 54
#define R_ARM_ABS32_NOI 55
#define R_ARM_REL32_NOI 56
#define R_ARM_ALU_PC_G0_NC 57
#define R_ARM_ALU_PC_G0 58
#define R_ARM_ALU_PC_G1_NC 59
#define R_ARM_ALU_PC_G1 60
#define R_ARM_ALU_PC_G2 61
#define R_ARM_LDR_PC_G1 62
#define R_ARM_LDR_PC_G2 63
#define R_ARM_LDRS_PC_G0 64
#define R_ARM_LDRS_PC_G1 65
#define R_ARM_LDRS_PC_G2 66
#define R_ARM_LDC_PC_G0 67
#define R_ARM_LDC_PC_G1 68
#define R_ARM_LDC_PC_G2 69
#define R_ARM_ALU_SB_G0_NC 70
#define R_ARM_ALU_SB_G0 71
#define R_ARM_ALU_SB_G1_NC 72
#define R_ARM_ALU_SB_G1 73
#define R_ARM_ALU_SB_G2 74
#define R_ARM_LDR_SB_G0 75
#define R_ARM_LDR_SB_G1 76
#define R_ARM_LDR_SB_G2 77
#define R_ARM_LDRS_SB_G0 78
#define R_ARM_LDRS_SB_G1 79
#define R_ARM_LDRS_SB_G2 80
#define R_ARM_LDC_SB_G0 81
#define R_ARM_LDC_SB_G1 82
#define R_ARM_LDC_SB_G2 83
#define R_ARM_MOVW_BREL_NC 84
#define R_ARM_MOVT_BREL 85
#define R_ARM_MOVW_BREL 86
#define R_ARM_THM_MOVW_BREL_NC 87
#define R_ARM_THM_MOVT_BREL 88
#define R_ARM_THM_MOVW_BREL 89
#define R_ARM_TLS_GOTDESC 90
#define R_ARM_TLS_CALL 91
#define R_ARM_TLS_DESCSEQ 92
#define R_ARM_THM_TLS_CALL 93
#define R_ARM_PLT32_ABS 94
#define R_ARM_GOT_ABS 95
#define R_ARM_GOT_PREL 96
#define R_ARM_GOT_BREL12 97
#define R_ARM_GOTOFF12 98
#define R_ARM_GOTRELAX 99
#define R_ARM_THM_JUMP11 102
#define R_ARM_THM_JUMP8 103
#define R_ARM_TLS_GD32 104
#define R_ARM_TLS_LDM32 105
#define R_ARM_TLS_LDO32 106
#define R_ARM_TLS_IE32 107
#define R_ARM_TLS_LE32 108
#define R_ARM_TLS_LDO12 109
#define R_ARM_TLS_LE12 110
#define R_ARM_TLS_IE12GP 111
#define R_ARM_PRIVATE_0 112
#define R_ARM_PRIVATE_1 113
#define R_ARM_PRIVATE_2 114
#define R_ARM_PRIVATE_3 115
#define R_ARM_PRIVATE_4 116
#define R_ARM_PRIVATE_5 117
#define R_ARM_PRIVATE_6 118
#define R_ARM_PRIVATE_7 119
#define R_ARM_PRIVATE_8 120
#define R_ARM_PRIVATE_9 121
#define R_ARM_PRIVATE_10 122
#define R_ARM_PRIVATE_11 123
#define R_ARM_PRIVATE_12 124
#define R_ARM_PRIVATE_13 125
#define R_ARM_PRIVATE_14 126
#define R_ARM_PRIVATE_15 127
#define R_ARM_THM_TLS_DESCSEQ16 129
#define R_ARM_THM_TLS_DESCSEQ32 130
#define R_ARM_THM_GOT_BREL12 131
#define R_ARM_THM_ALU_ABS_G0_NC 132
#define R_ARM_THM_ALU_ABS_G1_NC 133
#define R_ARM_THM_ALU_ABS_G2_NC 134
#define R_ARM_THM_ALU_ABS_G3 135
#define R_ARM_THM_BF16 136
#define R_ARM_THM_BF12 137
#define R_ARM_THM_BF18 138
#define R_ARM_IRELATIVE 160
#define R_ARM_PRIVATE_16 161
#define R_ARM_PRIVATE_17 162
#define R_ARM_PRIVATE_18 163
#define R_ARM_PRIVATE_19 164
#define R_ARM_PRIVATE_20 165
#define R_ARM_PRIVATE_21 166
#define R_ARM_PRIVATE_22 167
#define R_ARM_PRIVATE_23 168
#define R_ARM_PRIVATE_24 169
#define R_ARM_PRIVATE_25 170
#define R_ARM_PRIVATE_26 171
#define R_ARM_PRIVATE_27 172
#define R_ARM_PRIVATE_28 173
#define R_ARM_PRIVATE_29 174
#define R_ARM_PRIVATE_30 175
#define R_ARM_PRIVATE_31 176

#define R_X86_64_NONE 0
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_GOT32 3
#define R_X86_64_PLT32 4
#define R_X86_64_COPY 5
#define R_X86_64_GLOB_DAT 6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE 8
#define R_X86_64_GOTPCREL 9
#define R_X86_64_32 10
#define R_X86_64_32S 11
#define R_X86_64_16 12
#define R_X86_64_PC16 13
#define R_X86_64_8 14
#define R_X86_64_PC8 15
#define R_X86_64_DTPMOD64 16
#define R_X86_64_DTPOFF64 17
#define R_X86_64_TPOFF64 18
#define R_X86_64_TLSGD 19
#define R_X86_64_TLSLD 20
#define R_X86_64_DTPOFF32 21
#define R_X86_64_GOTTPOFF 22
#define R_X86_64_TPOFF32 23
#define R_X86_64_PC64 24
#define R_X86_64_GOTOFF64 25
#define R_X86_64_GOTPC32 26
#define R_X86_64_SIZE32 32
#define R_X86_64_SIZE64 33
#define R_X86_64_GOTPC32_TLSDESC 34
#define R_X86_64_TLSDESC_CALL 35
#define R_X86_64_TLSDESC 36
#define R_X86_64_IRELATIVE 37

#define R_AARCH64_NONE 0
#define R_AARCH64_ABS64 257
#define R_AARCH64_ABS32 258
#define R_AARCH64_ABS16 259
#define R_AARCH64_PREL64 260
#define R_AARCH64_PREL32 261
#define R_AARCH64_PREL16 262
#define R_AARCH64_MOVW_UABS_G0 263 
#define R_AARCH64_MOVW_UABS_G0_NC 264
#define R_AARCH64_MOVW_UABS_G1 265
#define R_AARCH64_MOVW_UABS_G1_NC 266
#define R_AARCH64_MOVW_UABS_G2 267
#define R_AARCH64_MOVW_UABS_G2_NC 268
#define R_AARCH64_MOVW_UABS_G3 269
#define R_AARCH64_MOVW_SABS_G0 270
#define R_AARCH64_MOVW_SABS_G1 271
#define R_AARCH64_MOVW_SABS_G2 272
#define R_AARCH64_LD_PREL_LO19 273
#define R_AARCH64_ADR_PREL_LO21 274
#define R_AARCH64_ADR_PREL_PG_HI21 275
#define R_AARCH64_ADR_PREL_PG_HI21_NC 276
#define R_AARCH64_ADD_ABS_LO12_NC 277
#define R_AARCH64_LDST8_ABS_LO12_NC 278
#define R_AARCH64_LDST16_ABS_LO12_NC 284
#define R_AARCH64_LDST32_ABS_LO12_NC 285
#define R_AARCH64_LDST64_ABS_LO12_NC 286
#define R_AARCH64_LDST128_ABS_LO12_NC 299
#define R_AARCH64_TSTBR14 279
#define R_AARCH64_CONDBR19 280
#define R_AARCH64_JUMP26 282
#define R_AARCH64_CALL26 283
#define R_AARCH64_MOVW_PREL_G0 287
#define R_AARCH64_MOVW_PREL_G0_NC 288
#define R_AARCH64_MOVW_PREL_G1 289
#define R_AARCH64_MOVW_PREL_G1_NC 290
#define R_AARCH64_MOVW_PREL_G2 291
#define R_AARCH64_MOVW_PREL_G2_NC 292
#define R_AARCH64_MOVW_PREL_G3 293
#define R_AARCH64_MOVW_GOTOFF_G0 300
#define R_AARCH64_MOVW_GOTOFF_G0_NC 301
#define R_AARCH64_MOVW_GOTOFF_G1 302
#define R_AARCH64_MOVW_GOTOFF_G1_NC 303
#define R_AARCH64_MOVW_GOTOFF_G2 304
#define R_AARCH64_MOVW_GOTOFF_G2_NC 305
#define R_AARCH64_MOVW_GOTOFF_G3 306
#define R_AARCH64_GOTREL64 307
#define R_AARCH64_GOTREL32 308
#define R_AARCH64_GOT_LD_PREL19 309
#define R_AARCH64_LD64_GOTOFF_LO15 310
#define R_AARCH64_ADR_GOT_PAGE 311
#define R_AARCH64_LD64_GOT_LO12_NC 312
#define R_AARCH64_LD64_GOTPAGE_LO15 313

#define R_LARCH_NONE 0
#define R_LARCH_32 1
#define R_LARCH_64 2
#define R_LARCH_RELATIVE 3
#define R_LARCH_COPY 4
#define R_LARCH_JUMP_SLOT 5
#define R_LARCH_TLS_DTPMOD32 6
#define R_LARCH_TLS_DTPMOD64 7
#define R_LARCH_TLS_DTPREL32 8
#define R_LARCH_TLS_DTPREL64 9
#define R_LARCH_TLS_TPREL32 10
#define R_LARCH_TLS_TPREL64 11
#define R_LARCH_IRELATIVE 12
#define R_LARCH_TLS_DESC32 13
#define R_LARCH_TLS_DESC64 14
#define R_LARCH_MARK_LA 20
#define R_LARCH_MARK_PCREL 21
#define R_LARCH_SOP_PUSH_PCREL 22
#define R_LARCH_SOP_PUSH_ABSOLUTE 23
#define R_LARCH_SOP_PUSH_DUP 24
#define R_LARCH_SOP_PUSH_GPREL 25
#define R_LARCH_SOP_PUSH_TLS_TPREL 26
#define R_LARCH_SOP_PUSH_TLS_GOT 27
#define R_LARCH_SOP_PUSH_TLS_GD 28
#define R_LARCH_SOP_PUSH_PLT_PCREL 29
#define R_LARCH_SOP_ASSERT 30
#define R_LARCH_SOP_NOT 31
#define R_LARCH_SOP_SUB 32
#define R_LARCH_SOP_SL 33
#define R_LARCH_SOP_SR 34
#define R_LARCH_SOP_ADD 35
#define R_LARCH_SOP_AND 36
#define R_LARCH_SOP_IF_ELSE 37
#define R_LARCH_SOP_POP_32_S_10_5 38
#define R_LARCH_SOP_POP_32_U_10_12 39
#define R_LARCH_SOP_POP_32_S_10_12 40
#define R_LARCH_SOP_POP_32_S_10_16 41
#define R_LARCH_SOP_POP_32_S_10_16_S2 42
#define R_LARCH_SOP_POP_32_S_5_20 43
#define R_LARCH_SOP_POP_32_S_0_5_10_16_S2 44
#define R_LARCH_SOP_POP_32_S_0_10_10_16_S2 45
#define R_LARCH_SOP_POP_32_U 46
#define R_LARCH_ADD8 47
#define R_LARCH_ADD16 48
#define R_LARCH_ADD24 49
#define R_LARCH_ADD32 50
#define R_LARCH_ADD64 51
#define R_LARCH_SUB8 52
#define R_LARCH_SUB16 53
#define R_LARCH_SUB24 54
#define R_LARCH_SUB32 55
#define R_LARCH_SUB64 56
#define R_LARCH_GNU_VTINHERIT 57
#define R_LARCH_GNU_VTENTRY 58
#define R_LARCH_B16 64
#define R_LARCH_B21 65
#define R_LARCH_B26 66
#define R_LARCH_ABS_HI20 67
#define R_LARCH_ABS_LO12 68
#define R_LARCH_ABS64_LO20 69
#define R_LARCH_ABS64_HI12 70
#define R_LARCH_PCALA_HI20 71
#define R_LARCH_PCALA_LO12 72
#define R_LARCH_PCALA64_LO20 73
#define R_LARCH_PCALA64_HI12 74
#define R_LARCH_GOT_PC_HI20 75
#define R_LARCH_GOT_PC_LO12 76
#define R_LARCH_GOT64_PC_LO20 77
#define R_LARCH_GOT64_PC_HI12 78
#define R_LARCH_GOT_HI20 79
#define R_LARCH_GOT_LO12 80
#define R_LARCH_GOT64_LO20 81
#define R_LARCH_GOT64_HI12 82
#define R_LARCH_TLS_LE_HI20 83
#define R_LARCH_TLS_LE_LO12 84
#define R_LARCH_TLS_LE64_LO20 85
#define R_LARCH_TLS_LE64_HI12 86
#define R_LARCH_TLS_IE_PC_HI20 87
#define R_LARCH_TLS_IE_PC_LO12 88
#define R_LARCH_TLS_IE64_PC_LO20 89
#define R_LARCH_TLS_IE64_PC_HI12 90
#define R_LARCH_TLS_IE_HI20 91
#define R_LARCH_TLS_IE_LO12 92
#define R_LARCH_TLS_IE64_LO20 93
#define R_LARCH_TLS_IE64_HI12 94
#define R_LARCH_TLS_LD_PC_HI20 95
#define R_LARCH_TLS_LD_HI20 96
#define R_LARCH_TLS_GD_PC_HI20 97
#define R_LARCH_TLS_GD_HI20 98
#define R_LARCH_32_PCREL 99
#define R_LARCH_RELAX 100
#define R_LARCH_ALIGN 102
#define R_LARCH_PCREL20_S2 103
#define R_LARCH_ADD6 105
#define R_LARCH_SUB6 106
#define R_LARCH_ADD_ULEB128 107
#define R_LARCH_SUB_ULEB128 108
#define R_LARCH_64_PCREL 109
#define R_LARCH_CALL36 110
#define R_LARCH_TLS_DESC_PC_HI20 111
#define R_LARCH_TLS_DESC_PC_LO12 112
#define R_LARCH_TLS_DESC64_PC_LO20 113
#define R_LARCH_TLS_DESC64_PC_HI12 114
#define R_LARCH_TLS_DESC_HI20 115
#define R_LARCH_TLS_DESC_LO12 116
#define R_LARCH_TLS_DESC64_LO20 117
#define R_LARCH_TLS_DESC64_HI12 118
#define R_LARCH_TLS_DESC_LD 119
#define R_LARCH_TLS_DESC_CALL 120
#define R_LARCH_TLS_LE_HI20_R 121
#define R_LARCH_TLS_LE_ADD_R 122
#define R_LARCH_TLS_LE_LO12_R 123
#define R_LARCH_TLS_LD_PCREL20_S2 124
#define R_LARCH_TLS_GD_PCREL20_S2 125
#define R_LARCH_TLS_DESC_PCREL20_S2 126

typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

struct Elf32_Phdr_internal {
    unsigned long p_type;
    unsigned long p_offset;
    unsigned long p_vaddr;
    unsigned long p_paddr;
    unsigned long p_filesz;
    unsigned long p_memsz;
    unsigned long p_flags;
    unsigned long p_align;
};

#define SIZEOF_struct_Elf32_Phdr_file 32
struct Elf32_Phdr_file {
    unsigned char p_type[4];
    unsigned char p_offset[4];
    unsigned char p_vaddr[4];
    unsigned char p_paddr[4];
    unsigned char p_filesz[4];
    unsigned char p_memsz[4];
    unsigned char p_flags[4];
    unsigned char p_align[4];
};

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

struct Elf64_Phdr_internal {
    unsigned long p_type;
    unsigned long p_flags;
    uint_fast64_t p_offset;
    uint_fast64_t p_vaddr;
    uint_fast64_t p_paddr;
    uint_fast64_t p_filesz;
    uint_fast64_t p_memsz;
    uint_fast64_t p_align;
};

#define SIZEOF_struct_Elf64_Phdr_file 56
struct Elf64_Phdr_file {
    unsigned char p_type[4];
    unsigned char p_flags[4];
    unsigned char p_offset[8];
    unsigned char p_vaddr[8];
    unsigned char p_paddr[8];
    unsigned char p_filesz[8];
    unsigned char p_memsz[8];
    unsigned char p_align[8];
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4
#define PF_MASKPROC 0xf0000000
