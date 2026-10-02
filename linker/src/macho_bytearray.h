/******************************************************************************
 * @file            macho_bytearray.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/

#ifdef SHORT_NAMES
#define read_struct_mach_header_64 rsmh64
#define read_struct_load_command rslc
#define read_struct_segment_command_64 rssc64
#define read_struct_section_64 rss64
#define read_struct_relocation_info rsri
#define read_struct_symtab_command rssc
#define read_struct_nlist_64 rsn64

#define write_struct_mach_header_64 wsmh64
#define write_struct_load_command wslc
#define write_struct_segment_command_64 wssc64
#define write_struct_section_64 wss64
#define write_struct_relocation_info wsri
#define write_struct_symtab_command wssc
#define write_struct_nlist_64 wsn64
#endif /* SHORT_NAMES */

void read_struct_mach_header_64 (struct mach_header_64_internal *mach_header_64_internal, const void *memory);
void read_struct_load_command (struct load_command_internal *load_command_internal, const void *memory);
void read_struct_segment_command_64 (struct segment_command_64_internal *segment_command_64_internal, const void *memory);
void read_struct_section_64 (struct section_64_internal *section_64_internal, const void *memory);
void read_struct_relocation_info (struct relocation_info_internal *relocation_info_internal, const void *memory);
void read_struct_symtab_command (struct symtab_command_internal *symtab_command_internal, const void *memory);
void read_struct_nlist_64 (struct nlist_64_internal *nlist_64_internal, const void *memory);
void read_struct_unixthread_command (struct unixthread_command_internal *unixthread_command_internal, const void *memory);
void read_struct_x86_thread_state64_t (struct x86_thread_state64_t_internal *x86_thread_state64_t_internal, const void *memory);
void read_struct_dysymtab_command (struct dysymtab_command_internal *dysymtab_command_internal, const void *memory);
void read_struct_dylib_command (struct dylib_command_internal *dylib_command_internal, const void *memory);
void read_struct_dylinker_command (struct dylinker_command_internal *dylinker_command_internal, const void *memory);
void read_struct_version_min_command (struct version_min_command_internal *version_min_command_internal, const void *memory);
void read_struct_function_starts_command (struct function_starts_command_internal *function_starts_command_internal, const void *memory);
void read_struct_main_command (struct main_command_internal *main_command_internal, const void *memory);
void read_struct_data_in_code_command (struct data_in_code_command_internal *data_in_code_command_internal, const void *memory);
void read_struct_source_version_command (struct source_version_command_internal *source_version_command_internal, const void *memory);
void read_struct_build_version_command (struct build_version_command_internal *build_version_command_internal, const void *memory);
void read_struct_dyld_exports_trie_command (struct dyld_exports_trie_command_internal *dyld_exports_trie_command_internal, const void *memory);
void read_struct_dyld_chained_fixups_command (struct dyld_chained_fixups_command_internal *dyld_chained_fixups_command_internal, const void *memory);
void read_struct_dyld_chained_fixups_header (struct dyld_chained_fixups_header_internal *dyld_chained_fixups_header_internal, const void *memory);
void read_struct_dyld_chained_starts_in_segment (struct dyld_chained_starts_in_segment_internal *dyld_chained_starts_in_segment_internal, const void *memory);

void write_struct_mach_header_64 (void *memory, const struct mach_header_64_internal *mach_header_64_internal);
void write_struct_load_command (void *memory, const struct load_command_internal *load_command_internal);
void write_struct_segment_command_64 (void *memory, const struct segment_command_64_internal *segment_command_64_internal);
void write_struct_section_64 (void *memory, const struct section_64_internal *section_64_internal);
void write_struct_relocation_info (void *memory, const struct relocation_info_internal *relocation_info_internal);
void write_struct_symtab_command (void *memory, const struct symtab_command_internal *symtab_command_internal);
void write_struct_nlist_64 (void *memory, const struct nlist_64_internal *nlist_64_internal);
void write_struct_unixthread_command (void *memory, const struct unixthread_command_internal *unixthread_command_internal);
void write_struct_x86_thread_state64_t (void *memory, const struct x86_thread_state64_t_internal *x86_thread_state64_t_internal);
void write_struct_dysymtab_command (void *memory, const struct dysymtab_command_internal *dysymtab_command_internal);
void write_struct_dylib_command (void *memory, const struct dylib_command_internal *dylib_command_internal);
void write_struct_dylinker_command (void *memory, const struct dylinker_command_internal *dylinker_command_internal);
void write_struct_version_min_command (void * memory, const struct version_min_command_internal *version_min_command_internal);
void write_struct_function_starts_command (void *memory, const struct function_starts_command_internal *function_starts_command_internal);
void write_struct_main_command (void *memory, const struct main_command_internal *main_command_internal);
void write_struct_data_in_code_command (void *memory, const struct data_in_code_command_internal *data_in_code_command_internal);
void write_struct_source_version_command (void *memory, const struct source_version_command_internal *source_version_command_internal);
void write_struct_build_version_command (void *memory, const struct build_version_command_internal *build_version_command_internal);
void write_struct_dyld_exports_trie_command (void *memory, const struct dyld_exports_trie_command_internal *dyld_exports_trie_command_internal);
void write_struct_dyld_chained_fixups_command (void *memory, const struct dyld_chained_fixups_command_internal *dyld_chained_fixups_command_internal);
void write_struct_dyld_chained_fixups_header (void *memory, const struct dyld_chained_fixups_header_internal *dyld_chained_fixups_header_internal);
void write_struct_dyld_chained_starts_in_segment (void *memory, const struct dyld_chained_starts_in_segment_internal *dyld_chained_starts_in_segment_internal);

