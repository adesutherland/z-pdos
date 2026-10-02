/******************************************************************************
 * @file            macho_bytearray.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ld.h"
#include "bytearray.h"
#include "macho.h"
#include "macho_bytearray.h"

#define COPY(struct_name, field_name, bytes) \
 bytearray_read_##bytes##_bytes (&struct_name##_internal->field_name, struct_name##_file->field_name, LITTLE_ENDIAN)
#define COPY_CHAR_ARRAY(struct_name, field_name) memcpy (struct_name##_internal->field_name, struct_name##_file->field_name, sizeof (struct_name##_file->field_name))

void read_struct_mach_header_64 (struct mach_header_64_internal *mach_header_64_internal, const void *memory)
{
    const struct mach_header_64_file *mach_header_64_file = memory;

    COPY(mach_header_64, magic, 4);
    COPY(mach_header_64, cputype, 4);
    COPY(mach_header_64, cpusubtype, 4);
    COPY(mach_header_64, filetype, 4);
    COPY(mach_header_64, ncmds, 4);
    COPY(mach_header_64, sizeofcmds, 4);
    COPY(mach_header_64, flags, 4);
    COPY(mach_header_64, reserved, 4);
}

void read_struct_load_command (struct load_command_internal *load_command_internal, const void *memory)
{
    const struct load_command_file *load_command_file = memory;

    COPY(load_command, cmd, 4);
    COPY(load_command, cmdsize, 4);
}

void read_struct_segment_command_64 (struct segment_command_64_internal *segment_command_64_internal, const void *memory)
{
    const struct segment_command_64_file *segment_command_64_file = memory;

    COPY(segment_command_64, cmd, 4);
    COPY(segment_command_64, cmdsize, 4);
    COPY_CHAR_ARRAY (segment_command_64, segname);
    COPY(segment_command_64, vmaddr, 8);
    COPY(segment_command_64, vmsize, 8);
    COPY(segment_command_64, fileoff, 8);
    COPY(segment_command_64, filesize, 8);
    COPY(segment_command_64, maxprot, 4);
    COPY(segment_command_64, initprot, 4);
    COPY(segment_command_64, nsects, 4);
    COPY(segment_command_64, flags, 4);
}

void read_struct_section_64 (struct section_64_internal *section_64_internal, const void *memory)
{
    const struct section_64_file *section_64_file = memory;
    
    COPY_CHAR_ARRAY (section_64, sectname);
    COPY_CHAR_ARRAY (section_64, segname);
    COPY(section_64, addr, 8);
    COPY(section_64, size, 8);
    COPY(section_64, offset, 4);
    COPY(section_64, align, 4);
    COPY(section_64, reloff, 4);
    COPY(section_64, nreloc, 4);
    COPY(section_64, flags, 4);
    COPY(section_64, reserved1, 4);
    COPY(section_64, reserved2, 4);
    COPY(section_64, reserved3, 4);
}

#if 0
/* Exactly same as for a.out. */
void read_struct_relocation_info (struct relocation_info_internal *relocation_info_internal, const void *memory)
{
    const struct relocation_info_file *relocation_info_file = memory;

    COPY(relocation_info, r_address, 4);
    COPY(relocation_info, r_symbolnum, 4);
}
#endif

void read_struct_symtab_command (struct symtab_command_internal *symtab_command_internal, const void *memory)
{
    const struct symtab_command_file *symtab_command_file = memory;

    COPY(symtab_command, cmd, 4);
    COPY(symtab_command, cmdsize, 4);
    COPY(symtab_command, symoff, 4);
    COPY(symtab_command, nsyms, 4);
    COPY(symtab_command, stroff, 4);
    COPY(symtab_command, strsize, 4);
}

void read_struct_nlist_64 (struct nlist_64_internal *nlist_64_internal, const void *memory)
{
    const struct nlist_64_file *nlist_64_file = memory;

    COPY(nlist_64, n_strx, 4);
    COPY(nlist_64, n_type, 1);
    COPY(nlist_64, n_sect, 1);
    COPY(nlist_64, n_desc, 2);
    COPY(nlist_64, n_value, 8);
}

void read_struct_unixthread_command (struct unixthread_command_internal *unixthread_command_internal, const void *memory)
{
    const struct unixthread_command_file *unixthread_command_file = memory;

    COPY(unixthread_command, cmd, 4);
    COPY(unixthread_command, cmdsize, 4);
    COPY(unixthread_command, flavor, 4);
    COPY(unixthread_command, count, 4);
}

void read_struct_x86_thread_state64_t (struct x86_thread_state64_t_internal *x86_thread_state64_t_internal, const void *memory)
{
    const struct x86_thread_state64_t_file *x86_thread_state64_t_file = memory;

    COPY(x86_thread_state64_t, rax, 8);
    COPY(x86_thread_state64_t, rbx, 8);
    COPY(x86_thread_state64_t, rcx, 8);
    COPY(x86_thread_state64_t, rdx, 8);
    COPY(x86_thread_state64_t, rdi, 8);
    COPY(x86_thread_state64_t, rsi, 8);
    COPY(x86_thread_state64_t, rbp, 8);
    COPY(x86_thread_state64_t, rsp, 8);
    COPY(x86_thread_state64_t, r8, 8);
    COPY(x86_thread_state64_t, r9, 8);
    COPY(x86_thread_state64_t, r10, 8);
    COPY(x86_thread_state64_t, r11, 8);
    COPY(x86_thread_state64_t, r12, 8);
    COPY(x86_thread_state64_t, r13, 8);
    COPY(x86_thread_state64_t, r14, 8);
    COPY(x86_thread_state64_t, r15, 8);
    COPY(x86_thread_state64_t, rip, 8);
    COPY(x86_thread_state64_t, rflags, 8);
    COPY(x86_thread_state64_t, cs, 8);
    COPY(x86_thread_state64_t, fs, 8);
    COPY(x86_thread_state64_t, gs, 8);
}

void read_struct_dysymtab_command (struct dysymtab_command_internal *dysymtab_command_internal, const void *memory)
{
    const struct dysymtab_command_file *dysymtab_command_file = memory;

    COPY(dysymtab_command, cmd, 4);
    COPY(dysymtab_command, cmdsize, 4);
    COPY(dysymtab_command, ilocalsym, 4);
    COPY(dysymtab_command, nlocalsym, 4);
    COPY(dysymtab_command, iextdefsym, 4);
    COPY(dysymtab_command, nextdefsym, 4);
    COPY(dysymtab_command, iundefsym, 4);
    COPY(dysymtab_command, nundefsym, 4);
    COPY(dysymtab_command, tocoff, 4);
    COPY(dysymtab_command, ntoc, 4);
    COPY(dysymtab_command, modtaboff, 4);
    COPY(dysymtab_command, nmodtab, 4);
    COPY(dysymtab_command, extrefsymoff, 4);
    COPY(dysymtab_command, nextrefsyms, 4);
    COPY(dysymtab_command, indirectsymoff, 4);
    COPY(dysymtab_command, nindirectsyms, 4);
    COPY(dysymtab_command, extreloff, 4);
    COPY(dysymtab_command, nextrel, 4);
    COPY(dysymtab_command, locreloff, 4);
    COPY(dysymtab_command, nlocrel, 4);
}

void read_struct_dylib_command (struct dylib_command_internal *dylib_command_internal, const void *memory)
{
    const struct dylib_command_file *dylib_command_file = memory;

    COPY(dylib_command, cmd, 4);
    COPY(dylib_command, cmdsize, 4);
    COPY(dylib_command, name_offset, 4);
    COPY(dylib_command, timestamp, 4);
    COPY(dylib_command, current_version, 4);
    COPY(dylib_command, compatibility_version, 4);
}

void read_struct_dylinker_command (struct dylinker_command_internal *dylinker_command_internal, const void *memory)
{
    const struct dylinker_command_file *dylinker_command_file = memory;

    COPY(dylinker_command, cmd, 4);
    COPY(dylinker_command, cmdsize, 4);
    COPY(dylinker_command, name_offset, 4);
}

void read_struct_version_min_command (struct version_min_command_internal *version_min_command_internal, const void *memory)
{
    const struct version_min_command_file *version_min_command_file = memory;

    COPY(version_min_command, cmd, 4);
    COPY(version_min_command, cmdsize, 4);
    COPY(version_min_command, version, 4);
    COPY(version_min_command, sdk, 4);
}

void read_struct_function_starts_command (struct function_starts_command_internal *function_starts_command_internal, const void *memory)
{
    const struct function_starts_command_file *function_starts_command_file = memory;

    COPY(function_starts_command, cmd, 4);
    COPY(function_starts_command, cmdsize, 4);
    COPY(function_starts_command, dataoff, 4);
    COPY(function_starts_command, datasize, 4);
}

void read_struct_main_command (struct main_command_internal *main_command_internal, const void *memory)
{
    const struct main_command_file *main_command_file = memory;

    COPY(main_command, cmd, 4);
    COPY(main_command, cmdsize, 4);
    COPY(main_command, entryoff, 8);
    COPY(main_command, stacksize, 8);
}

void read_struct_data_in_code_command (struct data_in_code_command_internal *data_in_code_command_internal, const void *memory)
{
    const struct data_in_code_command_file *data_in_code_command_file = memory;

    COPY(data_in_code_command, cmd, 4);
    COPY(data_in_code_command, cmdsize, 4);
    COPY(data_in_code_command, dataoff, 4);
    COPY(data_in_code_command, datasize, 4);
}

void read_struct_source_version_command (struct source_version_command_internal *source_version_command_internal, const void *memory)
{
    const struct source_version_command_file *source_version_command_file = memory;

    COPY(source_version_command, cmd, 4);
    COPY(source_version_command, cmdsize, 4);
    COPY(source_version_command, version, 8);
}

void read_struct_build_version_command (struct build_version_command_internal *build_version_command_internal, const void *memory)
{
    const struct build_version_command_file *build_version_command_file = memory;

    COPY(build_version_command, cmd, 4);
    COPY(build_version_command, cmdsize, 4);
    COPY(build_version_command, platform, 4);
    COPY(build_version_command, minos, 4);
    COPY(build_version_command, sdk, 4);
    COPY(build_version_command, ntools, 4);
    COPY(build_version_command, unknown1, 4);
    COPY(build_version_command, unknown2, 4);
}

void read_struct_dyld_exports_trie_command (struct dyld_exports_trie_command_internal *dyld_exports_trie_command_internal, const void *memory)
{
    const struct dyld_exports_trie_command_file *dyld_exports_trie_command_file = memory;

    COPY(dyld_exports_trie_command, cmd, 4);
    COPY(dyld_exports_trie_command, cmdsize, 4);
    COPY(dyld_exports_trie_command, dataoff, 4);
    COPY(dyld_exports_trie_command, datasize, 4);
}

void read_struct_dyld_chained_fixups_command (struct dyld_chained_fixups_command_internal *dyld_chained_fixups_command_internal, const void *memory)
{
    const struct dyld_chained_fixups_command_file *dyld_chained_fixups_command_file = memory;

    COPY(dyld_chained_fixups_command, cmd, 4);
    COPY(dyld_chained_fixups_command, cmdsize, 4);
    COPY(dyld_chained_fixups_command, dataoff, 4);
    COPY(dyld_chained_fixups_command, datasize, 4);
}

void read_struct_dyld_chained_fixups_header (struct dyld_chained_fixups_header_internal *dyld_chained_fixups_header_internal, const void *memory)
{
    const struct dyld_chained_fixups_header_file *dyld_chained_fixups_header_file = memory;

    COPY(dyld_chained_fixups_header, fixups_version, 4);
    COPY(dyld_chained_fixups_header, starts_offset, 4);
    COPY(dyld_chained_fixups_header, imports_offset, 4);
    COPY(dyld_chained_fixups_header, symbols_offset, 4);
    COPY(dyld_chained_fixups_header, imports_counts, 4);
    COPY(dyld_chained_fixups_header, imports_format, 4);
    COPY(dyld_chained_fixups_header, symbols_format, 4);
    COPY(dyld_chained_fixups_header, padding, 4);
}

void read_struct_dyld_chained_starts_in_segment (struct dyld_chained_starts_in_segment_internal *dyld_chained_starts_in_segment_internal, const void *memory)
{
    const struct dyld_chained_starts_in_segment_file *dyld_chained_starts_in_segment_file = memory;

    COPY(dyld_chained_starts_in_segment, size, 4);
    COPY(dyld_chained_starts_in_segment, page_size, 2);
    COPY(dyld_chained_starts_in_segment, pointer_format, 2);
    COPY(dyld_chained_starts_in_segment, segment_offset, 8);
    COPY(dyld_chained_starts_in_segment, max_valid_pointer, 4);
    COPY(dyld_chained_starts_in_segment, page_count, 2);
    COPY(dyld_chained_starts_in_segment, page_start, 2);
}

#undef COPY_CHAR_ARRAY
#undef COPY

#define COPY(struct_name, field_name, bytes) \
 bytearray_write_##bytes##_bytes (struct_name##_file->field_name, struct_name##_internal->field_name, LITTLE_ENDIAN)
#define COPY_CHAR_ARRAY(struct_name, field_name) memcpy (struct_name##_file->field_name, struct_name##_internal->field_name, sizeof (struct_name##_file->field_name))

void write_struct_mach_header_64 (void *memory, const struct mach_header_64_internal *mach_header_64_internal)
{
    struct mach_header_64_file *mach_header_64_file = memory;

    COPY(mach_header_64, magic, 4);
    COPY(mach_header_64, cputype, 4);
    COPY(mach_header_64, cpusubtype, 4);
    COPY(mach_header_64, filetype, 4);
    COPY(mach_header_64, ncmds, 4);
    COPY(mach_header_64, sizeofcmds, 4);
    COPY(mach_header_64, flags, 4);
    COPY(mach_header_64, reserved, 4);
}

void write_struct_load_command (void *memory, const struct load_command_internal *load_command_internal)
{
    struct load_command_file *load_command_file = memory;

    COPY(load_command, cmd, 4);
    COPY(load_command, cmdsize, 4);
}

void write_struct_segment_command_64 (void *memory, const struct segment_command_64_internal *segment_command_64_internal)
{
    struct segment_command_64_file *segment_command_64_file = memory;

    COPY(segment_command_64, cmd, 4);
    COPY(segment_command_64, cmdsize, 4);
    COPY_CHAR_ARRAY (segment_command_64, segname);
    COPY(segment_command_64, vmaddr, 8);
    COPY(segment_command_64, vmsize, 8);
    COPY(segment_command_64, fileoff, 8);
    COPY(segment_command_64, filesize, 8);
    COPY(segment_command_64, maxprot, 4);
    COPY(segment_command_64, initprot, 4);
    COPY(segment_command_64, nsects, 4);
    COPY(segment_command_64, flags, 4);
}

void write_struct_section_64 (void *memory, const struct section_64_internal *section_64_internal)
{
    struct section_64_file *section_64_file = memory;

    COPY_CHAR_ARRAY (section_64, sectname);
    COPY_CHAR_ARRAY (section_64, segname);
    COPY(section_64, addr, 8);
    COPY(section_64, size, 8);
    COPY(section_64, offset, 4);
    COPY(section_64, align, 4);
    COPY(section_64, reloff, 4);
    COPY(section_64, nreloc, 4);
    COPY(section_64, flags, 4);
    COPY(section_64, reserved1, 4);
    COPY(section_64, reserved2, 4);
    COPY(section_64, reserved3, 4);
}

#if 0
/* Exactly same as for a.out. */
void write_struct_relocation_info (void *memory, const struct relocation_info_internal *relocation_info_internal)
{
    struct relocation_info_file *relocation_info_file = memory;

    COPY(relocation_info, r_address, 4);
    COPY(relocation_info, r_symbolnum, 4);
}
#endif

void write_struct_symtab_command (void *memory, const struct symtab_command_internal *symtab_command_internal)
{
    struct symtab_command_file *symtab_command_file = memory;

    COPY(symtab_command, cmd, 4);
    COPY(symtab_command, cmdsize, 4);
    COPY(symtab_command, symoff, 4);
    COPY(symtab_command, nsyms, 4);
    COPY(symtab_command, stroff, 4);
    COPY(symtab_command, strsize, 4);
}

void write_struct_nlist_64 (void *memory, const struct nlist_64_internal *nlist_64_internal)
{
    struct nlist_64_file *nlist_64_file = memory;

    COPY(nlist_64, n_strx, 4);
    COPY(nlist_64, n_type, 1);
    COPY(nlist_64, n_sect, 1);
    COPY(nlist_64, n_desc, 2);
    COPY(nlist_64, n_value, 8);
}

void write_struct_unixthread_command (void *memory, const struct unixthread_command_internal *unixthread_command_internal)
{
    struct unixthread_command_file *unixthread_command_file = memory;

    COPY(unixthread_command, cmd, 4);
    COPY(unixthread_command, cmdsize, 4);
    COPY(unixthread_command, flavor, 4);
    COPY(unixthread_command, count, 4);
}

void write_struct_x86_thread_state64_t (void *memory, const struct x86_thread_state64_t_internal *x86_thread_state64_t_internal)
{
    struct x86_thread_state64_t_file *x86_thread_state64_t_file = memory;

    COPY(x86_thread_state64_t, rax, 8);
    COPY(x86_thread_state64_t, rbx, 8);
    COPY(x86_thread_state64_t, rcx, 8);
    COPY(x86_thread_state64_t, rdx, 8);
    COPY(x86_thread_state64_t, rdi, 8);
    COPY(x86_thread_state64_t, rsi, 8);
    COPY(x86_thread_state64_t, rbp, 8);
    COPY(x86_thread_state64_t, rsp, 8);
    COPY(x86_thread_state64_t, r8, 8);
    COPY(x86_thread_state64_t, r9, 8);
    COPY(x86_thread_state64_t, r10, 8);
    COPY(x86_thread_state64_t, r11, 8);
    COPY(x86_thread_state64_t, r12, 8);
    COPY(x86_thread_state64_t, r13, 8);
    COPY(x86_thread_state64_t, r14, 8);
    COPY(x86_thread_state64_t, r15, 8);
    COPY(x86_thread_state64_t, rip, 8);
    COPY(x86_thread_state64_t, rflags, 8);
    COPY(x86_thread_state64_t, cs, 8);
    COPY(x86_thread_state64_t, fs, 8);
    COPY(x86_thread_state64_t, gs, 8);
}

void write_struct_dysymtab_command (void *memory, const struct dysymtab_command_internal *dysymtab_command_internal)
{
    struct dysymtab_command_file *dysymtab_command_file = memory;

    COPY(dysymtab_command, cmd, 4);
    COPY(dysymtab_command, cmdsize, 4);
    COPY(dysymtab_command, ilocalsym, 4);
    COPY(dysymtab_command, nlocalsym, 4);
    COPY(dysymtab_command, iextdefsym, 4);
    COPY(dysymtab_command, nextdefsym, 4);
    COPY(dysymtab_command, iundefsym, 4);
    COPY(dysymtab_command, nundefsym, 4);
    COPY(dysymtab_command, tocoff, 4);
    COPY(dysymtab_command, ntoc, 4);
    COPY(dysymtab_command, modtaboff, 4);
    COPY(dysymtab_command, nmodtab, 4);
    COPY(dysymtab_command, extrefsymoff, 4);
    COPY(dysymtab_command, nextrefsyms, 4);
    COPY(dysymtab_command, indirectsymoff, 4);
    COPY(dysymtab_command, nindirectsyms, 4);
    COPY(dysymtab_command, extreloff, 4);
    COPY(dysymtab_command, nextrel, 4);
    COPY(dysymtab_command, locreloff, 4);
    COPY(dysymtab_command, nlocrel, 4);
}

void write_struct_dylib_command (void *memory, const struct dylib_command_internal *dylib_command_internal)
{
    struct dylib_command_file *dylib_command_file = memory;

    COPY(dylib_command, cmd, 4);
    COPY(dylib_command, cmdsize, 4);
    COPY(dylib_command, name_offset, 4);
    COPY(dylib_command, timestamp, 4);
    COPY(dylib_command, current_version, 4);
    COPY(dylib_command, compatibility_version, 4);
}

void write_struct_dylinker_command (void *memory, const struct dylinker_command_internal *dylinker_command_internal)
{
    struct dylinker_command_file *dylinker_command_file = memory;

    COPY(dylinker_command, cmd, 4);
    COPY(dylinker_command, cmdsize, 4);
    COPY(dylinker_command, name_offset, 4);
}

void write_struct_version_min_command (void * memory, const struct version_min_command_internal *version_min_command_internal)
{
    struct version_min_command_file *version_min_command_file = memory;

    COPY(version_min_command, cmd, 4);
    COPY(version_min_command, cmdsize, 4);
    COPY(version_min_command, version, 4);
    COPY(version_min_command, sdk, 4);
}

void write_struct_function_starts_command (void *memory, const struct function_starts_command_internal *function_starts_command_internal)
{
    struct function_starts_command_file *function_starts_command_file = memory;

    COPY(function_starts_command, cmd, 4);
    COPY(function_starts_command, cmdsize, 4);
    COPY(function_starts_command, dataoff, 4);
    COPY(function_starts_command, datasize, 4);
}

void write_struct_main_command (void *memory, const struct main_command_internal *main_command_internal)
{
    struct main_command_file *main_command_file = memory;

    COPY(main_command, cmd, 4);
    COPY(main_command, cmdsize, 4);
    COPY(main_command, entryoff, 8);
    COPY(main_command, stacksize, 8);
}

void write_struct_data_in_code_command (void *memory, const struct data_in_code_command_internal *data_in_code_command_internal)
{
    struct data_in_code_command_file *data_in_code_command_file = memory;

    COPY(data_in_code_command, cmd, 4);
    COPY(data_in_code_command, cmdsize, 4);
    COPY(data_in_code_command, dataoff, 4);
    COPY(data_in_code_command, datasize, 4);
}

void write_struct_source_version_command (void *memory, const struct source_version_command_internal *source_version_command_internal)
{
    struct source_version_command_file *source_version_command_file = memory;

    COPY(source_version_command, cmd, 4);
    COPY(source_version_command, cmdsize, 4);
    COPY(source_version_command, version, 8);
}

void write_struct_build_version_command (void *memory, const struct build_version_command_internal *build_version_command_internal)
{
    struct build_version_command_file *build_version_command_file = memory;

    COPY(build_version_command, cmd, 4);
    COPY(build_version_command, cmdsize, 4);
    COPY(build_version_command, platform, 4);
    COPY(build_version_command, minos, 4);
    COPY(build_version_command, sdk, 4);
    COPY(build_version_command, ntools, 4);
    COPY(build_version_command, unknown1, 4);
    COPY(build_version_command, unknown2, 4);
}

void write_struct_dyld_exports_trie_command (void *memory, const struct dyld_exports_trie_command_internal *dyld_exports_trie_command_internal)
{
    struct dyld_exports_trie_command_file *dyld_exports_trie_command_file = memory;

    COPY(dyld_exports_trie_command, cmd, 4);
    COPY(dyld_exports_trie_command, cmdsize, 4);
    COPY(dyld_exports_trie_command, dataoff, 4);
    COPY(dyld_exports_trie_command, datasize, 4);
}

void write_struct_dyld_chained_fixups_command (void *memory, const struct dyld_chained_fixups_command_internal *dyld_chained_fixups_command_internal)
{
    struct dyld_chained_fixups_command_file *dyld_chained_fixups_command_file = memory;

    COPY(dyld_chained_fixups_command, cmd, 4);
    COPY(dyld_chained_fixups_command, cmdsize, 4);
    COPY(dyld_chained_fixups_command, dataoff, 4);
    COPY(dyld_chained_fixups_command, datasize, 4);
}

void write_struct_dyld_chained_fixups_header (void *memory, const struct dyld_chained_fixups_header_internal *dyld_chained_fixups_header_internal)
{
    struct dyld_chained_fixups_header_file *dyld_chained_fixups_header_file = memory;

    COPY(dyld_chained_fixups_header, fixups_version, 4);
    COPY(dyld_chained_fixups_header, starts_offset, 4);
    COPY(dyld_chained_fixups_header, imports_offset, 4);
    COPY(dyld_chained_fixups_header, symbols_offset, 4);
    COPY(dyld_chained_fixups_header, imports_counts, 4);
    COPY(dyld_chained_fixups_header, imports_format, 4);
    COPY(dyld_chained_fixups_header, symbols_format, 4);
    COPY(dyld_chained_fixups_header, padding, 4);
}

void write_struct_dyld_chained_starts_in_segment (void *memory, const struct dyld_chained_starts_in_segment_internal *dyld_chained_starts_in_segment_internal)
{
    struct dyld_chained_starts_in_segment_file *dyld_chained_starts_in_segment_file = memory;

    COPY(dyld_chained_starts_in_segment, size, 4);
    COPY(dyld_chained_starts_in_segment, page_size, 2);
    COPY(dyld_chained_starts_in_segment, pointer_format, 2);
    COPY(dyld_chained_starts_in_segment, segment_offset, 8);
    COPY(dyld_chained_starts_in_segment, max_valid_pointer, 4);
    COPY(dyld_chained_starts_in_segment, page_count, 2);
    COPY(dyld_chained_starts_in_segment, page_start, 2);
}

#undef COPY_CHAR_ARRAY
#undef COPY
