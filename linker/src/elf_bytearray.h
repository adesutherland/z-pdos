/******************************************************************************
 * @file            elf_bytearray.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/

#ifdef SHORT_NAMES
#define read_struct_Elf32_Ehdr rsee
#define read_struct_Elf64_Ehdr r6see
#define read_struct_Elf32_Shdr rses
#define read_struct_Elf64_Shdr r6ses
#define read_struct_Elf32_Sym rsesym
#define read_struct_Elf64_Sym r6sesym
#define read_struct_Elf32_Rel rserel
#define read_struct_Elf64_Rel r6serel
#define read_struct_Elf32_Rela rserla
#define read_struct_Elf64_Rela r6serla
#define read_struct_Elf32_Phdr rsep
#define read_struct_Elf64_Phdr r6sep

#define write_struct_Elf32_Ehdr wsee
#define write_struct_Elf64_Ehdr w6see
#define write_struct_Elf32_Shdr wses
#define write_struct_Elf64_Shdr w6ses
#define write_struct_Elf32_Sym wsesym
#define write_struct_Elf64_Sym w6sesym
#define write_struct_Elf32_Rel wserel
#define write_struct_Elf64_Rel w6serel
#define write_struct_Elf32_Rela wserla
#define write_struct_Elf64_Rela w6serla
#define write_struct_Elf32_Phdr wsep
#define write_struct_Elf64_Phdr w6sep
#endif /* SHORT_NAMES */

void read_struct_Elf32_Ehdr (struct Elf32_Ehdr_internal *Elf32_Ehdr_internal, const void *memory, int endianess);
void read_struct_Elf64_Ehdr (struct Elf64_Ehdr_internal *Elf64_Ehdr_internal, const void *memory, int endianess);
void read_struct_Elf32_Shdr (struct Elf32_Shdr_internal *Elf32_Shdr_internal, const void *memory, int endianess);
void read_struct_Elf64_Shdr (struct Elf64_Shdr_internal *Elf64_Shdr_internal, const void *memory, int endianess);
void read_struct_Elf32_Sym (struct Elf32_Sym_internal *Elf32_Sym_internal, const void *memory, int endianess);
void read_struct_Elf64_Sym (struct Elf64_Sym_internal *Elf64_Sym_internal, const void *memory, int endianess);
void read_struct_Elf32_Rel (struct Elf32_Rel_internal *Elf32_Rel_internal, const void *memory, int endianess);
void read_struct_Elf64_Rel (struct Elf64_Rel_internal *Elf64_Rel_internal, const void *memory, int endianess);
void read_struct_Elf32_Rela (struct Elf32_Rela_internal *Elf32_Rela_internal, const void *memory, int endianess);
void read_struct_Elf64_Rela (struct Elf64_Rela_internal *Elf64_Rela_internal, const void *memory, int endianess);
void read_struct_Elf32_Phdr (struct Elf32_Phdr_internal *Elf32_Phdr_internal, const void *memory, int endianess);
void read_struct_Elf64_Phdr (struct Elf64_Phdr_internal *Elf64_Phdr_internal, const void *memory, int endianess);

void write_struct_Elf32_Ehdr (void *memory, const struct Elf32_Ehdr_internal *Elf32_Ehdr_internal, int endianess);
void write_struct_Elf64_Ehdr (void *memory, const struct Elf64_Ehdr_internal *Elf64_Ehdr_internal, int endianess);
void write_struct_Elf32_Shdr (void *memory, const struct Elf32_Shdr_internal *Elf32_Shdr_internal, int endianess);
void write_struct_Elf64_Shdr (void *memory, const struct Elf64_Shdr_internal *Elf64_Shdr_internal, int endianess);
void write_struct_Elf32_Sym (void *memory, const struct Elf32_Sym_internal *Elf32_Sym_internal, int endianess);
void write_struct_Elf64_Sym (void *memory, const struct Elf64_Sym_internal *Elf64_Sym_internal, int endianess);
void write_struct_Elf32_Rel (void *memory, const struct Elf32_Rel_internal *Elf32_Rel_internal, int endianess);
void write_struct_Elf64_Rel (void *memory, const struct Elf64_Rel_internal *Elf64_Rel_internal, int endianess);
void write_struct_Elf32_Rela (void *memory, const struct Elf32_Rela_internal *Elf32_Rela_internal, int endianess);
void write_struct_Elf64_Rela (void *memory, const struct Elf64_Rela_internal *Elf64_Rela_internal, int endianess);
void write_struct_Elf32_Phdr (void *memory, const struct Elf32_Phdr_internal *Elf32_Phdr_internal, int endianess);
void write_struct_Elf64_Phdr (void *memory, const struct Elf64_Phdr_internal *Elf64_Phdr_internal, int endianess);


