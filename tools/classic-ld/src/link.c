/******************************************************************************
 * @file            link.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ld.h"
#include "xmalloc.h"
#include "bytearray.h"

static void reloc_section_index (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol);

static void reloc_i386_secrel (struct section_part *part,
                               struct reloc_entry *rel,
                               struct symbol *symbol);

static void reloc_arm_26_pcrel (struct section_part *part,
                                struct reloc_entry *rel,
                                struct symbol *symbol);
static void reloc_arm_mov32 (struct section_part *part,
                             struct reloc_entry *rel,
                             struct symbol *symbol);
static void reloc_arm_thumb_mov32 (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol);
static void reloc_arm_thumb_branch20 (struct section_part *part,
                                      struct reloc_entry *rel,
                                      struct symbol *symbol);
static void reloc_arm_thumb_blx23 (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol);

static void reloc_aarch64_hi21_page_pcrel (struct section_part *part,
                                           struct reloc_entry *rel,
                                           struct symbol *symbol);
static void reloc_aarch64_generic (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol);

static void reloc_loongarch_b16 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol);
static void reloc_loongarch_b21 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol);
static void reloc_loongarch_b26 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol);
static void reloc_loongarch_pcala_hi20 (struct section_part *part,
                                        struct reloc_entry *rel,
                                        struct symbol *symbol);
static void reloc_loongarch_pcala_lo12 (struct section_part *part,
                                        struct reloc_entry *rel,
                                        struct symbol *symbol);

static void reloc_generic (struct section_part *part,
                           struct reloc_entry *rel,
                           struct symbol *symbol);

static void reloc_generic_add (struct section_part *part,
                               struct reloc_entry *rel,
                               struct symbol *symbol);
static void reloc_generic_subtract (struct section_part *part,
                                    struct reloc_entry *rel,
                                    struct symbol *symbol);

const struct reloc_howto reloc_howtos[RELOC_TYPE_END] = {
    { 0, 0, 0, 0, NULL, "RELOC_TYPE_IGNORED" },
    
    { 8, 0, 0, 0, NULL, "RELOC_TYPE_64" },
    { 4, 0, 0, 0, NULL, "RELOC_TYPE_32" },
    { 3, 0, 0, 0, NULL, "RELOC_TYPE_24" },
    { 2, 0, 0, 0, NULL, "RELOC_TYPE_16" },
    { 1, 0, 0, 0, NULL, "RELOC_TYPE_8" },

    { 8, 1, 0, 0, NULL, "RELOC_TYPE_PC64" },
    { 4, 1, 0, 0, NULL, "RELOC_TYPE_PC32" },
    { 2, 1, 0, 0, NULL, "RELOC_TYPE_PC16" },
    { 1, 1, 0, 0, NULL, "RELOC_TYPE_PC8" },

    { 4, 0, 1, 0, NULL, "RELOC_TYPE_32_NO_BASE" },

    { 8, 0, 0, 0, &reloc_generic_add, "RELOC_TYPE_64_ADD" },
    { 4, 0, 0, 0, &reloc_generic_add, "RELOC_TYPE_32_ADD" },
    { 3, 0, 0, 0, &reloc_generic_add, "RELOC_TYPE_24_ADD" },
    { 2, 0, 0, 0, &reloc_generic_add, "RELOC_TYPE_16_ADD" },
    { 1, 0, 0, 0, &reloc_generic_add, "RELOC_TYPE_8_ADD" },

    { 8, 0, 0, 0, &reloc_generic_subtract, "RELOC_TYPE_64_SUB" },
    { 4, 0, 0, 0, &reloc_generic_subtract, "RELOC_TYPE_32_SUB" },
    { 3, 0, 0, 0, &reloc_generic_subtract, "RELOC_TYPE_24_SUB" },
    { 2, 0, 0, 0, &reloc_generic_subtract, "RELOC_TYPE_16_SUB" },
    { 1, 0, 0, 0, &reloc_generic_subtract, "RELOC_TYPE_8_SUB" },

    { 2, 0, 1, 0, &reloc_section_index, "RELOC_TYPE_16_SECTION_INDEX" },

    { 4, 1, 0, 1, NULL, "RELOC_TYPE_PC32_FRSHIFT1" },
    { 2, 1, 0, 1, NULL, "RELOC_TYPE_PC16_FRSHIFT1" },
    
    { 4, 0, 1, 0, &reloc_i386_secrel, "RELOC_TYPE_I386_SECREL" },

    { 4, 0, 0, 0, NULL, "RELOC_TYPE_ARM_32" },
    { 3, 1, 0, 2, &reloc_arm_26_pcrel, "RELOC_TYPE_ARM_PC26" },
    { 4, 0, 0, 0, &reloc_arm_mov32, "RELOC_TYPE_ARM_MOV32" },
    { 4, 0, 0, 0, &reloc_arm_thumb_mov32, "RELOC_TYPE_ARM_THUMB_MOV32" },
    { 4, 1, 0, 0, &reloc_arm_thumb_branch20, "RELOC_TYPE_ARM_THUMB_BRANCH20" },
    { 4, 1, 0, 0, &reloc_arm_thumb_blx23, "RELOC_TYPE_ARM_THUMB_BLX23" },

    { 4, 1, 0, 9, &reloc_aarch64_hi21_page_pcrel, "RELOC_TYPE_AARCH64_ADR_PREL_PG_HI21", 0x60ffffe0 },
    { 4, 0, 0, 0, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_ADD_ABS_LO12_NC", 0x3ffc00, 10 },
    { 4, 0, 0, 0, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_LDST8_ABS_LO12_NC", 0x3ffc00, 10 },
    { 4, 0, 0, 1, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_LDST16_ABS_LO12_NC", 0x1ffc00, 10 },
    { 4, 0, 0, 2, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_LDST32_ABS_LO12_NC", 0xffc00, 10 },
    { 4, 0, 0, 3, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_LDST64_ABS_LO12_NC", 0x7fc00, 10 },
    { 4, 0, 0, 4, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_LDST128_ABS_LO12_NC", 0x3fc00, 10 },
    { 4, 1, 0, 2, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_JUMP26", 0x3ffffff },
    { 4, 1, 0, 2, &reloc_aarch64_generic, "RELOC_TYPE_AARCH64_CALL26", 0x3ffffff },

    { 4, 1, 0, 2, &reloc_loongarch_b16, "RELOC_TYPE_LOONGARCH_B16" },
    { 4, 1, 0, 2, &reloc_loongarch_b21, "RELOC_TYPE_LOONGARCH_B21" },
    { 4, 1, 0, 2, &reloc_loongarch_b26, "RELOC_TYPE_LOONGARCH_B26" },
    { 4, 1, 0, 12, &reloc_loongarch_pcala_hi20, "RELOC_TYPE_LOONGARCH_PCALA_HI20" },
    { 4, 1, 0, 0, &reloc_loongarch_pcala_lo12, "RELOC_TYPE_LOONGARCH_PCALA_LO12" },
};

static void reloc_section_index (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol)
{
    address_type result;
    unsigned short field2;

    if (!symbol->part) {
        ld_error ("relocation target '%s' of section index relocation has no section",
                  symbol->name);
        return;
    }

    bytearray_read_2_bytes (&field2, part->content + rel->offset, LITTLE_ENDIAN);
    result = field2;
    
    result += rel->addend;
    result += symbol->part->section->target_index;
    
    field2 = result;
    bytearray_write_2_bytes (part->content + rel->offset, field2, LITTLE_ENDIAN);
}

static void reloc_i386_secrel (struct section_part *part,
                               struct reloc_entry *rel,
                               struct symbol *symbol)
{
    address_type result;
    unsigned long field;

    if (!symbol->part) {
        ld_error ("relocation target '%s' of section relative relocation has no section",
                  symbol->name);
        return;
    }
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    result = field;
    
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= symbol->part->section->rva;
    
    field = result;
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_arm_26_pcrel (struct section_part *part,
                                struct reloc_entry *rel,
                                struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_3_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    result = field;
    result <<= 2;
    /* The field is signed, so sign extend it. */
    result = (result ^ 0x2000000) - 0x2000000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    /* The size of the field must not be subtracted
     * even though it is PC relative relocation.
     */
    result -= part->rva + rel->offset;
    result >>= 2;
    bytearray_write_3_bytes (part->content + rel->offset, result, LITTLE_ENDIAN);
}

static void reloc_arm_mov32 (struct section_part *part,
                             struct reloc_entry *rel,
                             struct symbol *symbol)
{
    address_type result;
    unsigned long field, extracted;

    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    extracted = field & 0xfff;
    extracted |= ((field & 0xf0000) >> 16) << 12;
    result = extracted;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset + 4, LITTLE_ENDIAN);
    extracted = field & 0xfff;
    extracted |= ((field & 0xf0000) >> 16) << 12;
    result |= extracted << 16;
    
    result += rel->addend;
    result += symbol_get_value_with_base (symbol);

    extracted = result & 0xffff;
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    field &= ~0xf0fff;
    field |= extracted & 0xfff;
    field |= ((extracted >> 12) << 16) & 0xf0000;
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);

    extracted = (result >> 16) & 0xffff;
    bytearray_read_4_bytes (&field, part->content + rel->offset + 4, LITTLE_ENDIAN);
    field &= ~0xf0fff;
    field |= extracted & 0xfff;
    field |= ((extracted >> 12) << 16) & 0xf0000;
    bytearray_write_4_bytes (part->content + rel->offset + 4, field, LITTLE_ENDIAN);
}

static void reloc_arm_thumb_mov32 (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol)
{
    address_type result;
    unsigned long field, extracted;

    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    extracted = (field & 0xf) << 12;
    extracted |= ((field & 0x400) >> 10) << 11;
    extracted |= ((field & 0x70000000) >> 28) << 8;
    extracted |= (field & 0xff0000) >> 16;
    result = extracted;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset + 4, LITTLE_ENDIAN);
    extracted = (field & 0xf) << 12;
    extracted |= ((field & 0x400) >> 10) << 11;
    extracted |= ((field & 0x70000000) >> 28) << 8;
    extracted |= (field & 0xff0000) >> 16;
    result |= extracted << 16;
    
    result += rel->addend;
    result += symbol_get_value_with_base (symbol);

    extracted = result & 0xffff;
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    field &= ~0x70ff040f;
    field |= (extracted >> 12) & 0xf;
    field |= ((extracted >> 11) << 10) & 0x400;
    field |= ((extracted >> 8) << 28) & 0x70000000;
    field |= (extracted << 16) & 0xff0000;
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);

    extracted = (result >> 16) & 0xffff;
    bytearray_read_4_bytes (&field, part->content + rel->offset + 4, LITTLE_ENDIAN);
    field &= ~0x70ff040f;
    field |= (extracted >> 12) & 0xf;
    field |= ((extracted >> 11) << 10) & 0x400;
    field |= ((extracted >> 8) << 28) & 0x70000000;
    field |= (extracted << 16) & 0xff0000;
    bytearray_write_4_bytes (part->content + rel->offset + 4, field, LITTLE_ENDIAN);
}

static void reloc_arm_thumb_branch20 (struct section_part *part,
                                      struct reloc_entry *rel,
                                      struct symbol *symbol)
{
    address_type result;
    unsigned long field;

    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    result = (field & 0x20000000) >> 11;
    result |= ((field & 0x08000000) >> 27) << 19;
    result |= ((field & 0x07ff0000) >> 16) << 1;
    result |= (field & 0x3f) << 12;
    result |= ((field & 0x400) >> 10) << 20;

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x100000) - 0x100000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= part->rva + rel->offset;
    /* Unlike other ARM PC relative relocations,
     * the size of the field should be subtracted.
     */
    result -= rel->howto->size;

    field &= ~0x2fff043f;
    field |= (result << 11) & 0x20000000;
    field |= ((result >> 19) << 27) & 0x08000000;
    field |= ((result >> 1) << 16) & 0x07ff0000;
    field |= (result >> 12) & 0x3f;
    field |= ((result >> 20) << 10) & 0x400;
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_arm_thumb_blx23 (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    result = ((field & 0x7ff) << 12) | ((field & 0x07ff0000) >> 15);

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x400000) - 0x400000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= part->rva + rel->offset;
    /* Unlike other ARM PC relative relocations,
     * the size of the field should be subtracted.
     */
    result -= rel->howto->size;

    field &= ~0x07ff07ff;
    field |= ((result & 0xffe) << 15) | ((result >> 12) & 0x7ff);    
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_aarch64_hi21_page_pcrel (struct section_part *part,
                                           struct reloc_entry *rel,
                                           struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    address_type bottom_2_bits;

    /* The lowest 2 bits of immediate operand for ADRP are 0x60000000
     * while the rest of immediate starts at 0x20.
     * (adrp x0, 0x0000 is 0x90000000
     *  adrp x0, 0x1000 is 0xb0000000,
     *  adrp x0, 0x2000 is 0xd0000000,
     *  adrp x0, 0x3000 is 0xf0000000,
     *  adrp x0, 0x4000 is 0x90000020,
     *  adrp x0, 0x5000 is 0xb0000020,
     *  adrp x0, 0x6000 is 0xd0000020,
     *  adrp x0, 0x7000 is 0xf0000020...)
     */
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
    if (!rel->addend) {
        /* Confusingly, PE/COFF does not use regular implicit addend
         * for this relocation and instead it uses the value bits
         * of the instruction as RELA addend field to store absolute addend.
         * First the value is extracted, pretending it is regular value.
         */
        result = field & rel->howto->dst_mask;
        field &= ~(address_type)rel->howto->dst_mask;
        bottom_2_bits = (result >> 29) & 0x3;
        result &= ~(((address_type)0x3) << 29);
        result <<= 9;
        result |= bottom_2_bits << 12;

        /* The field is signed 21-bit integer shifted 12 bits left,
         * so shift it 12 bits right to convert it
         * to absolute addend and sign extend it.
         */
        result >>= 12;
        result = (result ^ (0x100000000 >> 12)) - (0x100000000 >> 12);
    } else {
        result = rel->addend;
    }
    
    result = (result + symbol_get_value_with_base (symbol)) & ~(address_type)0xfff;
    result -= (ld_state->base_address + part->rva + rel->offset) & ~(address_type)0xfff;
    
    if ((bottom_2_bits = result & 0x3000)) {
        result &= ~bottom_2_bits;
        bottom_2_bits >>= 12;
    }
    result >>= 9;
    /* If the result is negative, those 2 bits are already set,
     * so they must be cleared before putting the real bottom 2 bits there.
     */
    result &= ~(((address_type)0x3) << 29);
    result |= bottom_2_bits << 29;

    field = ((field & ~(address_type)rel->howto->dst_mask)
             | (((field & rel->howto->dst_mask) + result) & rel->howto->dst_mask));
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_aarch64_generic (struct section_part *part,
                                   struct reloc_entry *rel,
                                   struct symbol *symbol)
{
    address_type result, field;

    switch (rel->howto->size) {
        case 8:
            {
                uint_fast64_t field8;
                bytearray_read_8_bytes (&field8, part->content + rel->offset, LITTLE_ENDIAN);
                field = field8;
            }
            break;

        case 4:
            {
                unsigned long field4;
                bytearray_read_4_bytes (&field4, part->content + rel->offset, LITTLE_ENDIAN);
                field = field4;
            }
            break;

        case 2:
            {
                unsigned short field2;
                bytearray_read_2_bytes (&field2, part->content + rel->offset, LITTLE_ENDIAN);
                field = field2;
            }
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }

    if (!rel->addend) {
        result = field & rel->howto->dst_mask;
        field &= ~(address_type)rel->howto->dst_mask;
        result <<= rel->howto->final_right_shift;
        result >>= rel->howto->final_left_shift;
    } else {
        result = rel->addend;
    }
    
    result += symbol_get_value_with_base (symbol);
    if (rel->howto->pc_relative) {
        result -= ld_state->base_address + part->rva + rel->offset;
    }
    
    result >>= rel->howto->final_right_shift;
    result <<= rel->howto->final_left_shift;

    field = ((field & ~(address_type)rel->howto->dst_mask)
             | (((field & rel->howto->dst_mask) + result) & rel->howto->dst_mask));

    switch (rel->howto->size) {
        case 8:
            bytearray_write_8_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
            break;

        case 4:
            bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
            break;

        case 2:
            bytearray_write_2_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }
    
}

static void reloc_loongarch_b16 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
#if 0
    result = (field & 0x3fffc00) >> 8;
#else
    /* With ELF RELA the value already encoded
     * in the field before relocation must not be used.
     */
    result = 0;
#endif

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x20000) - 0x20000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= part->rva + rel->offset;

    field &= ~0x3fffc00;
    field |= (result & 0x3fffc) << 8;    
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_loongarch_b21 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
#if 0
    result = ((field & 0x1f) << 18) | ((field & 0x3fffc00) >> 8);
#else
    /* With ELF RELA the value already encoded
     * in the field before relocation must not be used.
     */
    result = 0;
#endif

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x400000) - 0x400000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= part->rva + rel->offset;

    field &= ~0x3fffc1f;
    field |= ((result >> 18) & 0x1f) | ((result & 0x3fffc) << 8);    
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_loongarch_b26 (struct section_part *part,
                                 struct reloc_entry *rel,
                                 struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
#if 0
    result = ((field & 0x3ff) << 18) | ((field & 0x3fffc00) >> 8);
#else
    /* With ELF RELA the value already encoded
     * in the field before relocation must not be used.
     */
    result = 0;
#endif

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x8000000) - 0x8000000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result -= part->rva + rel->offset;

    field &= ~0x3ffffff;
    field |= ((result >> 18) & 0x3ff) | ((result & 0x3fffc) << 8);
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_loongarch_pcala_hi20 (struct section_part *part,
                                        struct reloc_entry *rel,
                                        struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
#if 0
    result = (field & 0x1ffffe0) << 7;
#else
    /* With ELF RELA the value already encoded
     * in the field before relocation must not be used.
     */
    result = 0;
#endif

    /* The field is signed, so sign extend it. */
    result = (result ^ 0x80000000) - 0x80000000;
    result += rel->addend;
    result += symbol_get_value_no_base (symbol);
    result = (result + 0x800) & ~0xfff;
    result -= (part->rva + rel->offset) & ~0xfff;

    field &= ~0x1ffffe0;
    field |= (result >> 7) & 0x1ffffe0;    
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}
    
static void reloc_loongarch_pcala_lo12 (struct section_part *part,
                                        struct reloc_entry *rel,
                                        struct symbol *symbol)
{
    address_type result;
    unsigned long field;
    
    bytearray_read_4_bytes (&field, part->content + rel->offset, LITTLE_ENDIAN);
#if 0
    result = (field & 0x3ffc00) >> 10;
#else
    /* With ELF RELA the value already encoded
     * in the field before relocation must not be used.
     */
    result = 0;
#endif

    result += rel->addend;
    result += symbol_get_value_with_base (symbol);

    field &= ~0x3ffc00;
    field |= (result & 0xfff) << 10;    
    bytearray_write_4_bytes (part->content + rel->offset, field, LITTLE_ENDIAN);
}

static void reloc_generic (struct section_part *part,
                           struct reloc_entry *rel,
                           struct symbol *symbol)
{
    address_type result;
    int endianess;

    if (ld_state->target_machine == LD_TARGET_MACHINE_M68K
        || ld_state->target_machine == LD_TARGET_MACHINE_MAINFRAME) {
        endianess = BIG_ENDIAN;
    } else {
        endianess = LITTLE_ENDIAN;
    }
    
    switch (rel->howto->size) {
        case 8:
            {
                uint_fast64_t field8;
                bytearray_read_8_bytes (&field8, part->content + rel->offset, endianess);
                result = field8;
            }
            break;

        case 4:
            {
                unsigned long field4;
                bytearray_read_4_bytes (&field4, part->content + rel->offset, endianess);
                result = field4;
            }
            break;

        case 3:
            {
                unsigned long field3;
                bytearray_read_3_bytes (&field3, part->content + rel->offset, endianess);
                result = field3;
            }
            break;

        case 2:
            {
                unsigned short field2;
                bytearray_read_2_bytes (&field2, part->content + rel->offset, endianess);
                result = field2;
            }
            break;

        case 1:
            {
                unsigned char field1;
                bytearray_read_1_bytes (&field1, part->content + rel->offset, endianess);
                result = field1;
            }
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }

    result += rel->addend;
        
    if (rel->howto->pc_relative
        || rel->howto->no_base) {
        result += symbol_get_value_no_base (symbol);
    } else {
        result += symbol_get_value_with_base (symbol);
    }

    if (rel->howto->pc_relative) {
        result -= part->rva + rel->offset;
        result -= rel->howto->size;
    }

    if (rel->howto->size < sizeof result) {
        address_type mask = (((address_type)1) << (CHAR_BIT * rel->howto->size)) - 1;
        result &= mask;
    }
    if (rel->howto->final_right_shift) {
        /* If the result was negative, it should remain negative even after the final right shift. */
        address_type sign_bit = result & (((address_type)1) << (rel->howto->size * 8 - 1));
        result >>= rel->howto->final_right_shift;
        result |= sign_bit;
    }
    
    switch (rel->howto->size) {
        case 8:
            bytearray_write_8_bytes (part->content + rel->offset, result, endianess);
            break;

        case 4:
            bytearray_write_4_bytes (part->content + rel->offset, result, endianess);
            break;

        case 3:
            bytearray_write_3_bytes (part->content + rel->offset, result, endianess);
            break;

        case 2:
            bytearray_write_2_bytes (part->content + rel->offset, result, endianess);
            break;

        case 1:
            bytearray_write_1_bytes (part->content + rel->offset, result, endianess);
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }
}

static address_type generic_read (struct section_part *part,
                                  struct reloc_entry *rel)
{
    address_type result;
    int endianess;

    if (ld_state->target_machine == LD_TARGET_MACHINE_M68K
        || ld_state->target_machine == LD_TARGET_MACHINE_MAINFRAME) {
        endianess = BIG_ENDIAN;
    } else {
        endianess = LITTLE_ENDIAN;
    }
    
    switch (rel->howto->size) {
        case 8:
            {
                uint_fast64_t field8;
                bytearray_read_8_bytes (&field8, part->content + rel->offset, endianess);
                result = field8;
            }
            break;

        case 4:
            {
                unsigned long field4;
                bytearray_read_4_bytes (&field4, part->content + rel->offset, endianess);
                result = field4;
            }
            break;

        case 3:
            {
                unsigned long field3;
                bytearray_read_3_bytes (&field3, part->content + rel->offset, endianess);
                result = field3;
            }
            break;

        case 2:
            {
                unsigned short field2;
                bytearray_read_2_bytes (&field2, part->content + rel->offset, endianess);
                result = field2;
            }
            break;

        case 1:
            {
                unsigned char field1;
                bytearray_read_1_bytes (&field1, part->content + rel->offset, endianess);
                result = field1;
            }
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }

    return result;
}

static void generic_write (struct section_part *part,
                           struct reloc_entry *rel,
                           address_type result)
{
    int endianess;

    if (ld_state->target_machine == LD_TARGET_MACHINE_M68K
        || ld_state->target_machine == LD_TARGET_MACHINE_MAINFRAME) {
        endianess = BIG_ENDIAN;
    } else {
        endianess = LITTLE_ENDIAN;
    }

    switch (rel->howto->size) {
        case 8:
            bytearray_write_8_bytes (part->content + rel->offset, result, endianess);
            break;

        case 4:
            bytearray_write_4_bytes (part->content + rel->offset, result, endianess);
            break;

        case 3:
            bytearray_write_3_bytes (part->content + rel->offset, result, endianess);
            break;

        case 2:
            bytearray_write_2_bytes (part->content + rel->offset, result, endianess);
            break;

        case 1:
            bytearray_write_1_bytes (part->content + rel->offset, result, endianess);
            break;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "invalid relocation size");
    }
}

static void reloc_generic_add (struct section_part *part,
                               struct reloc_entry *rel,
                               struct symbol *symbol)
{
    address_type result, saved;
    
    saved = generic_read (part, rel);
    result = rel->addend;
        
    if (rel->howto->pc_relative
        || rel->howto->no_base) {
        result += symbol_get_value_no_base (symbol);
    } else {
        result += symbol_get_value_with_base (symbol);
    }

    if (rel->howto->pc_relative) {
        result -= part->rva + rel->offset;
        result -= rel->howto->size;
    }

    if (rel->howto->size < sizeof result) {
        address_type mask = (((address_type)1) << (CHAR_BIT * rel->howto->size)) - 1;
        result &= mask;
    }
    if (rel->howto->final_right_shift) {
        /* If the result was negative, it should remain negative even after the final right shift. */
        address_type sign_bit = result & (((address_type)1) << (rel->howto->size * 8 - 1));
        result >>= rel->howto->final_right_shift;
        result |= sign_bit;
    }
    
    generic_write (part, rel, saved + result);
}

static void reloc_generic_subtract (struct section_part *part,
                                    struct reloc_entry *rel,
                                    struct symbol *symbol)
{
    address_type result, saved;
    
    saved = generic_read (part, rel);
    result = rel->addend;
        
    if (rel->howto->pc_relative
        || rel->howto->no_base) {
        result += symbol_get_value_no_base (symbol);
    } else {
        result += symbol_get_value_with_base (symbol);
    }

    if (rel->howto->pc_relative) {
        result -= part->rva + rel->offset;
        result -= rel->howto->size;
    }

    if (rel->howto->size < sizeof result) {
        address_type mask = (((address_type)1) << (CHAR_BIT * rel->howto->size)) - 1;
        result &= mask;
    }
    if (rel->howto->final_right_shift) {
        /* If the result was negative, it should remain negative even after the final right shift. */
        address_type sign_bit = result & (((address_type)1) << (rel->howto->size * 8 - 1));
        result >>= rel->howto->final_right_shift;
        result |= sign_bit;
    }
    
    generic_write (part, rel, saved - result);
}

static void relocate_part (struct section_part *part)
{
    struct reloc_entry *relocs;
    size_t i;
    
    relocs = part->relocation_array;
    for (i = 0; i < part->relocation_count; i++) {
        struct symbol *symbol;

        if (relocs[i].howto->size == 0) continue;

        symbol = relocs[i].symbol;
        if (symbol_is_undefined (symbol)) {
            if ((symbol = symbol_find (symbol->name)) == NULL) {
                symbol = relocs[i].symbol;
                ld_internal_error_at_source (__FILE__, __LINE__,
                                             "external symbol '%s' not found in hashtab",
                                             symbol->name);
            }
            if (less_strict_mainframe_matching
                && symbol_is_undefined (symbol)) {
                symbol = mainframe_symbol_find (symbol->name);
            }
            if (symbol_is_undefined (symbol)) {
                if (ld_state->oformat == LD_OFORMAT_MTS
                    || ld_state->relocatable) {
                    /* With MTS output some undefined symbols
                     * reference system routines
                     * and should not cause error when undefined.
                     * When doing partial link (relocatable),
                     * undefined symbols should be allowed.
                     */
                    continue;
                }
                ld_error ("%s:(%s+%#"PRIxADDRESS"): undefined reference to '%s'",
                          part->of->filename,
                          part->section->name,
                          relocs[i].offset,
                          symbol->name);
                continue;
            }
        }
        
        if (relocs[i].howto->special_function) {
            (*relocs[i].howto->special_function) (part, &relocs[i], symbol);
            continue;
        }

        reloc_generic (part, &relocs[i], symbol);
    }
}

static void collapse_subsections (void)
{
    struct section *section;

    for (section = all_sections; section; section = section->next) {
        struct subsection *subsection;

        for (subsection = section->all_subsections; subsection; subsection = subsection->next) {
            if (subsection->first_part) {
                *section->last_part_p = subsection->first_part;
                section->last_part_p = subsection->last_part_p;
            }

        }
    }
}

static void calculate_section_sizes_and_rvas (void)
{
    struct section *section;
    address_type rva = 0;

    if (ld_state->oformat == LD_OFORMAT_COFF) {
        rva = coff_get_first_section_rva ();
    } else if (ld_state->oformat == LD_OFORMAT_ELF) {
        rva = elf_get_first_section_rva ();
    } else if (ld_state->oformat == LD_OFORMAT_MACHO) {
        rva = macho_get_first_section_rva ();
    }

    for (section = all_sections; section; section = section->next) {
        struct section_part *part;

        /* The MVS/XMIT writer pads every section to a doubleword. Keep
         * symbol RVAs and relocation values at those written positions. */
        if (ld_state->oformat == LD_OFORMAT_MVS
            || ld_state->oformat == LD_OFORMAT_XMIT) {
            rva = ALIGN (rva, 8);
        }
        rva = ALIGN (rva, section->section_alignment);
        section->rva = rva;
        
        section->total_size = 0;
        for (part = section->first_part; part; part = part->next) {

            if (part->next && part->next->alignment > 1) {
                address_type new_rva;

                new_rva = ALIGN (rva + part->content_size, part->next->alignment);
                if (new_rva != rva + part->content_size) {
                    part->content = xrealloc (part->content, new_rva - rva);
                    memset (part->content + part->content_size,
                            0,
                            new_rva - rva - part->content_size);
                    part->content_size = new_rva - rva;
                }
            }
            
            part->rva = rva;
            section->total_size += part->content_size;
            rva += part->content_size;
        }
    }
}

static void relocate_sections (void)
{
    struct section *section;

    for (section = all_sections; section; section = section->next) {
        struct section_part *part;
        
        for (part = section->first_part; part; part = part->next) {
            relocate_part (part);
        }
    }
}

static void calculate_entry_point (void)
{
    const struct section *section;

    if (ld_state->entry_local_symbol) {
        ld_state->entry_point = symbol_get_value_no_base (ld_state->entry_local_symbol);
        return;
    }

    if (ld_state->entry_symbol_name) {
        if (ld_state->entry_symbol_name[0] == '\0') {
            ld_state->entry_point -= ld_state->base_address;
            return;
        } else {
            const struct symbol *symbol;

            symbol = symbol_find (ld_state->entry_symbol_name);
            if (less_strict_mainframe_matching
                && (!symbol || symbol_is_undefined (symbol))) {
                symbol = mainframe_symbol_find (ld_state->entry_symbol_name);
            }
            if (symbol && !symbol_is_undefined (symbol)) {
                ld_state->entry_point = symbol_get_value_no_base (symbol);
                return;
            }
        }
    }

    if (ld_state->entry_symbol_name == NULL) {
        if ((ld_state->entry_point = coff_calculate_entry_point ())) return;
    }

    section = section_find (".text");
    if (section) ld_state->entry_point = section->rva;

    if (ld_state->entry_symbol_name) {
        ld_warn ("cannot find entry symbol '%s'; defaulting to 0x%08"PRIxADDRESS,
                 ld_state->entry_symbol_name,
                 ld_state->base_address + ld_state->entry_point);
    }
}

static void add_automatic_symbols (void)
{
    struct object_file *of;
    struct symbol *symbol;

    if ((symbol = symbol_find ("__ImageBase"))
        && !symbol_is_undefined (symbol)) return;

    of = object_file_make (1, FAKE_LD_FILENAME);

    symbol = of->symbol_array;
    symbol->name = xstrdup ("__ImageBase");
    symbol->value = ld_state->base_address;
    symbol->part = NULL;
    symbol->section_number = ABSOLUTE_SECTION_NUMBER;
    symbol_record_external_symbol (symbol);
}

static void check_unresolved (void)
{
    struct object_file *of;
    unsigned long unresolved = 0;

    for (of = all_object_files; of; of = of->next) {
        size_t i;

        for (i = 0; i < of->symbol_count; i++) {
            struct symbol *symbol = of->symbol_array + i;

            /* Undefined section symbols (created by discarding COMDAT)
             * and undefined nameless ELF/MVS auxiliary symbols
             * should not be reported here.
             */
            if (symbol->auxiliary
                || !symbol_is_undefined (symbol)
                || (symbol->flags & SYMBOL_FLAG_SECTION_SYMBOL)
                || !symbol->name) continue;

            if ((symbol = symbol_find (symbol->name)) == NULL) {
                symbol = of->symbol_array + i;
                ld_internal_error_at_source (__FILE__, __LINE__,
                                             "external symbol '%s' not found in hashtab",
                                             symbol->name);
            }
            if (less_strict_mainframe_matching
                && symbol_is_undefined (symbol)) {
                symbol = mainframe_symbol_find (symbol->name);
            }
            if (symbol_is_undefined (symbol)) {
                ld_error ("%s: unresolved external symbol '%s'",
                          of->filename,
                          symbol->name);
                unresolved++;
            }
        }
    }

    if (unresolved) {
        ld_fatal_error ("%lu unresolved external%s",
                        unresolved,
                        unresolved != 1 ? "s" : "");
    }
}

void link (void)
{
    if (!ld_state->use_custom_base_address) {
        switch (ld_state->oformat) {
            case LD_OFORMAT_AMIGA:
                ld_state->base_address = amiga_get_base_address ();
                break;

            case LD_OFORMAT_AOUT:
                ld_state->base_address = aout_get_base_address ();
                break;
            
            case LD_OFORMAT_ATARI:
                ld_state->base_address = atari_get_base_address ();
                break;

            case LD_OFORMAT_BINARY:
                ld_state->base_address = binary_get_base_address ();
                break;

            case LD_OFORMAT_CMS:
                ld_state->base_address = cms_get_base_address ();
                break;
            
            case LD_OFORMAT_COFF:
                ld_state->base_address = coff_get_base_address ();
                break;

            case LD_OFORMAT_LX:
                ld_state->base_address = lx_get_base_address ();
                break;

            case LD_OFORMAT_MACHO:
                ld_state->base_address = macho_get_base_address ();
                break;

            case LD_OFORMAT_MVS:
                ld_state->base_address = mvs_get_base_address ();
                break;

            case LD_OFORMAT_MTS:
                ld_state->base_address = mts_get_base_address ();
                break;

            case LD_OFORMAT_MUSICSP:
                ld_state->base_address = musicsp_get_base_address ();
                break;

            case LD_OFORMAT_VSE:
                ld_state->base_address = vse_get_base_address ();
                break;

            case LD_OFORMAT_XMIT:
                ld_state->base_address = xmit_get_base_address ();
                break;

            case LD_OFORMAT_Z390:
                ld_state->base_address = z390_get_base_address ();
                break;

            default:
                ld_state->base_address = coff_get_base_address ();
                break;
        }
    }
    
    add_automatic_symbols ();
    
    /* In the future this might be controlled by command line option. */
    /* Some MTS undefined symbols reference system routines
     * and should not cause error when undefined.
     */
    if (ld_state->oformat != LD_OFORMAT_MTS
        && !ld_state->relocatable) check_unresolved ();
    
    collapse_subsections ();

    calculate_section_sizes_and_rvas ();

    relocate_sections ();

    calculate_entry_point ();
    
}
