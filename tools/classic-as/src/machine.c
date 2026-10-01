/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original machine facts and encoder. Sources: IBM System/360 Principles
 * of Operation, A22-6821-6 (January 1967), Instruction Formats and individual
 * instruction descriptions; IBM System/370 Principles of Operation,
 * GA22-7000-6 (March 1980), Instruction Formats and corresponding descriptions.
 * BAS/BASR: GA22-7000-7 (March 1981), Branch and Save facility and instruction
 * descriptions. Our selected S370 subset includes this optional facility;
 * support is not a claim that every historical S370 machine provides it.
 * IBM bit positions count from the most significant bit, numbered zero.
 * Profile S360 is the common architecture, excluding Model 67 extensions.
 * Mnemonics are numeric ASCII octets, independent of execution character set.
 */
#include "mf_classic.h"
static const mf_octet name_LPDR[] = { 0x4c, 0x50, 0x44, 0x52 };
static const mf_octet name_LTDR[] = { 0x4c, 0x54, 0x44, 0x52 };
static const mf_octet name_LCDR[] = { 0x4c, 0x43, 0x44, 0x52 };
static const mf_octet name_LDR[] = { 0x4c, 0x44, 0x52 };
static const mf_octet name_CDR[] = { 0x43, 0x44, 0x52 };
static const mf_octet name_ADR[] = { 0x41, 0x44, 0x52 };
static const mf_octet name_SDR[] = { 0x53, 0x44, 0x52 };
static const mf_octet name_MDR[] = { 0x4d, 0x44, 0x52 };
static const mf_octet name_DDR[] = { 0x44, 0x44, 0x52 };
static const mf_octet name_LRER[] = { 0x4c, 0x52, 0x45, 0x52 };
static const mf_octet name_STD[] = { 0x53, 0x54, 0x44 };
static const mf_octet name_LD[] = { 0x4c, 0x44 };
static const mf_octet name_CD[] = { 0x43, 0x44 };
static const mf_octet name_AE[] = { 0x41, 0x45 };
static const mf_octet name_AD[] = { 0x41, 0x44 };
static const mf_octet name_SD[] = { 0x53, 0x44 };
static const mf_octet name_MD[] = { 0x4d, 0x44 };
static const mf_octet name_DD[] = { 0x44, 0x44 };
static const mf_octet name_STE[] = { 0x53, 0x54, 0x45 };
static const mf_octet name_LE[] = { 0x4c, 0x45 };
static const mf_octet name_BALR[] = { 0x42, 0x41, 0x4c, 0x52 };
static const mf_octet name_BCTR[] = { 0x42, 0x43, 0x54, 0x52 };
static const mf_octet name_BCR[] = { 0x42, 0x43, 0x52 };
static const mf_octet name_SVC[] = { 0x53, 0x56, 0x43 };
static const mf_octet name_LPR[] = { 0x4c, 0x50, 0x52 };
static const mf_octet name_LNR[] = { 0x4c, 0x4e, 0x52 };
static const mf_octet name_LTR[] = { 0x4c, 0x54, 0x52 };
static const mf_octet name_LCR[] = { 0x4c, 0x43, 0x52 };
static const mf_octet name_NR[] = { 0x4e, 0x52 };
static const mf_octet name_CLR[] = { 0x43, 0x4c, 0x52 };
static const mf_octet name_OR[] = { 0x4f, 0x52 };
static const mf_octet name_XR[] = { 0x58, 0x52 };
static const mf_octet name_LR[] = { 0x4c, 0x52 };
static const mf_octet name_CR[] = { 0x43, 0x52 };
static const mf_octet name_AR[] = { 0x41, 0x52 };
static const mf_octet name_SR[] = { 0x53, 0x52 };
static const mf_octet name_MR[] = { 0x4d, 0x52 };
static const mf_octet name_DR[] = { 0x44, 0x52 };
static const mf_octet name_ALR[] = { 0x41, 0x4c, 0x52 };
static const mf_octet name_SLR[] = { 0x53, 0x4c, 0x52 };
static const mf_octet name_STH[] = { 0x53, 0x54, 0x48 };
static const mf_octet name_LA[] = { 0x4c, 0x41 };
static const mf_octet name_STC[] = { 0x53, 0x54, 0x43 };
static const mf_octet name_IC[] = { 0x49, 0x43 };
static const mf_octet name_EX[] = { 0x45, 0x58 };
static const mf_octet name_BAL[] = { 0x42, 0x41, 0x4c };
static const mf_octet name_BCT[] = { 0x42, 0x43, 0x54 };
static const mf_octet name_BC[] = { 0x42, 0x43 };
static const mf_octet name_LH[] = { 0x4c, 0x48 };
static const mf_octet name_CH[] = { 0x43, 0x48 };
static const mf_octet name_AH[] = { 0x41, 0x48 };
static const mf_octet name_SH[] = { 0x53, 0x48 };
static const mf_octet name_MH[] = { 0x4d, 0x48 };
static const mf_octet name_CVD[] = { 0x43, 0x56, 0x44 };
static const mf_octet name_CVB[] = { 0x43, 0x56, 0x42 };
static const mf_octet name_ST[] = { 0x53, 0x54 };
static const mf_octet name_N[] = { 0x4e };
static const mf_octet name_CL[] = { 0x43, 0x4c };
static const mf_octet name_O[] = { 0x4f };
static const mf_octet name_X[] = { 0x58 };
static const mf_octet name_L[] = { 0x4c };
static const mf_octet name_C[] = { 0x43 };
static const mf_octet name_A[] = { 0x41 };
static const mf_octet name_S[] = { 0x53 };
static const mf_octet name_M[] = { 0x4d };
static const mf_octet name_D[] = { 0x44 };
static const mf_octet name_AL[] = { 0x41, 0x4c };
static const mf_octet name_SL[] = { 0x53, 0x4c };
static const mf_octet name_SRL[] = { 0x53, 0x52, 0x4c };
static const mf_octet name_SLL[] = { 0x53, 0x4c, 0x4c };
static const mf_octet name_SRA[] = { 0x53, 0x52, 0x41 };
static const mf_octet name_SLA[] = { 0x53, 0x4c, 0x41 };
static const mf_octet name_SRDL[] = { 0x53, 0x52, 0x44, 0x4c };
static const mf_octet name_SLDL[] = { 0x53, 0x4c, 0x44, 0x4c };
static const mf_octet name_SRDA[] = { 0x53, 0x52, 0x44, 0x41 };
static const mf_octet name_SLDA[] = { 0x53, 0x4c, 0x44, 0x41 };
static const mf_octet name_STM[] = { 0x53, 0x54, 0x4d };
static const mf_octet name_LM[] = { 0x4c, 0x4d };
static const mf_octet name_BXH[] = { 0x42, 0x58, 0x48 };
static const mf_octet name_BXLE[] = { 0x42, 0x58, 0x4c, 0x45 };
static const mf_octet name_TM[] = { 0x54, 0x4d };
static const mf_octet name_MVI[] = { 0x4d, 0x56, 0x49 };
static const mf_octet name_NI[] = { 0x4e, 0x49 };
static const mf_octet name_CLI[] = { 0x43, 0x4c, 0x49 };
static const mf_octet name_OI[] = { 0x4f, 0x49 };
static const mf_octet name_XI[] = { 0x58, 0x49 };
static const mf_octet name_MVC[] = { 0x4d, 0x56, 0x43 };
static const mf_octet name_NC[] = { 0x4e, 0x43 };
static const mf_octet name_CLC[] = { 0x43, 0x4c, 0x43 };
static const mf_octet name_OC[] = { 0x4f, 0x43 };
static const mf_octet name_XC[] = { 0x58, 0x43 };
static const mf_octet name_TR[] = { 0x54, 0x52 };
static const mf_octet name_TRT[] = { 0x54, 0x52, 0x54 };
static const mf_octet name_ED[] = { 0x45, 0x44 };
static const mf_octet name_EDMK[] = { 0x45, 0x44, 0x4d, 0x4b };
static const mf_octet name_MVO[] = { 0x4d, 0x56, 0x4f };
static const mf_octet name_PACK[] = { 0x50, 0x41, 0x43, 0x4b };
static const mf_octet name_UNPK[] = { 0x55, 0x4e, 0x50, 0x4b };
static const mf_octet name_ZAP[] = { 0x5a, 0x41, 0x50 };
static const mf_octet name_CP[] = { 0x43, 0x50 };
static const mf_octet name_AP[] = { 0x41, 0x50 };
static const mf_octet name_SP[] = { 0x53, 0x50 };
static const mf_octet name_MP[] = { 0x4d, 0x50 };
static const mf_octet name_DP[] = { 0x44, 0x50 };
static const mf_octet name_BASR[] = { 0x42, 0x41, 0x53, 0x52 };
static const mf_octet name_BAS[] = { 0x42, 0x41, 0x53 };
static const mf_octet name_STCM[] = { 0x53, 0x54, 0x43, 0x4d };
static const mf_octet name_CLM[] = { 0x43, 0x4c, 0x4d };
static const mf_octet name_ICM[] = { 0x49, 0x43, 0x4d };
static const mf_octet name_CS[] = { 0x43, 0x53 };
static const mf_octet name_CDS[] = { 0x43, 0x44, 0x53 };
static const mf_octet name_MVCL[] = { 0x4d, 0x56, 0x43, 0x4c };
static const mf_octet name_CLCL[] = { 0x43, 0x4c, 0x43, 0x4c };

static const struct mf_instruction instructions[] = {
    { { name_BALR, 4 }, MF_S360, MF_RR, 0x05, 2 },
    { { name_BCTR, 4 }, MF_S360, MF_RR, 0x06, 2 },
    { { name_BCR, 3 }, MF_S360, MF_RR, 0x07, 2 },
    { { name_SVC, 3 }, MF_S360, MF_RR, 0x0a, 2 },
    { { name_LPR, 3 }, MF_S360, MF_RR, 0x10, 2 },
    { { name_LNR, 3 }, MF_S360, MF_RR, 0x11, 2 },
    { { name_LTR, 3 }, MF_S360, MF_RR, 0x12, 2 },
    { { name_LCR, 3 }, MF_S360, MF_RR, 0x13, 2 },
    { { name_NR, 2 }, MF_S360, MF_RR, 0x14, 2 },
    { { name_CLR, 3 }, MF_S360, MF_RR, 0x15, 2 },
    { { name_OR, 2 }, MF_S360, MF_RR, 0x16, 2 },
    { { name_XR, 2 }, MF_S360, MF_RR, 0x17, 2 },
    { { name_LR, 2 }, MF_S360, MF_RR, 0x18, 2 },
    { { name_CR, 2 }, MF_S360, MF_RR, 0x19, 2 },
    { { name_AR, 2 }, MF_S360, MF_RR, 0x1a, 2 },
    { { name_SR, 2 }, MF_S360, MF_RR, 0x1b, 2 },
    { { name_MR, 2 }, MF_S360, MF_RR, 0x1c, 2 },
    { { name_DR, 2 }, MF_S360, MF_RR, 0x1d, 2 },
    { { name_ALR, 3 }, MF_S360, MF_RR, 0x1e, 2 },
    { { name_SLR, 3 }, MF_S360, MF_RR, 0x1f, 2 },
    { { name_STH, 3 }, MF_S360, MF_RX, 0x40, 4 },
    { { name_LA, 2 }, MF_S360, MF_RX, 0x41, 4 },
    { { name_STC, 3 }, MF_S360, MF_RX, 0x42, 4 },
    { { name_IC, 2 }, MF_S360, MF_RX, 0x43, 4 },
    { { name_EX, 2 }, MF_S360, MF_RX, 0x44, 4 },
    { { name_BAL, 3 }, MF_S360, MF_RX, 0x45, 4 },
    { { name_BCT, 3 }, MF_S360, MF_RX, 0x46, 4 },
    { { name_BC, 2 }, MF_S360, MF_RX, 0x47, 4 },
    { { name_LH, 2 }, MF_S360, MF_RX, 0x48, 4 },
    { { name_CH, 2 }, MF_S360, MF_RX, 0x49, 4 },
    { { name_AH, 2 }, MF_S360, MF_RX, 0x4a, 4 },
    { { name_SH, 2 }, MF_S360, MF_RX, 0x4b, 4 },
    { { name_MH, 2 }, MF_S360, MF_RX, 0x4c, 4 },
    { { name_CVD, 3 }, MF_S360, MF_RX, 0x4e, 4 },
    { { name_CVB, 3 }, MF_S360, MF_RX, 0x4f, 4 },
    { { name_ST, 2 }, MF_S360, MF_RX, 0x50, 4 },
    { { name_N, 1 }, MF_S360, MF_RX, 0x54, 4 },
    { { name_CL, 2 }, MF_S360, MF_RX, 0x55, 4 },
    { { name_O, 1 }, MF_S360, MF_RX, 0x56, 4 },
    { { name_X, 1 }, MF_S360, MF_RX, 0x57, 4 },
    { { name_L, 1 }, MF_S360, MF_RX, 0x58, 4 },
    { { name_C, 1 }, MF_S360, MF_RX, 0x59, 4 },
    { { name_A, 1 }, MF_S360, MF_RX, 0x5a, 4 },
    { { name_S, 1 }, MF_S360, MF_RX, 0x5b, 4 },
    { { name_M, 1 }, MF_S360, MF_RX, 0x5c, 4 },
    { { name_D, 1 }, MF_S360, MF_RX, 0x5d, 4 },
    { { name_AL, 2 }, MF_S360, MF_RX, 0x5e, 4 },
    { { name_SL, 2 }, MF_S360, MF_RX, 0x5f, 4 },
    { { name_SRL, 3 }, MF_S360, MF_RS, 0x88, 4 },
    { { name_SLL, 3 }, MF_S360, MF_RS, 0x89, 4 },
    { { name_SRA, 3 }, MF_S360, MF_RS, 0x8a, 4 },
    { { name_SLA, 3 }, MF_S360, MF_RS, 0x8b, 4 },
    { { name_SRDL, 4 }, MF_S360, MF_RS, 0x8c, 4 },
    { { name_SLDL, 4 }, MF_S360, MF_RS, 0x8d, 4 },
    { { name_SRDA, 4 }, MF_S360, MF_RS, 0x8e, 4 },
    { { name_SLDA, 4 }, MF_S360, MF_RS, 0x8f, 4 },
    { { name_STM, 3 }, MF_S360, MF_RS, 0x90, 4 },
    { { name_LM, 2 }, MF_S360, MF_RS, 0x98, 4 },
    { { name_BXH, 3 }, MF_S360, MF_RS, 0x86, 4 },
    { { name_BXLE, 4 }, MF_S360, MF_RS, 0x87, 4 },
    { { name_TM, 2 }, MF_S360, MF_SI, 0x91, 4 },
    { { name_MVI, 3 }, MF_S360, MF_SI, 0x92, 4 },
    { { name_NI, 2 }, MF_S360, MF_SI, 0x94, 4 },
    { { name_CLI, 3 }, MF_S360, MF_SI, 0x95, 4 },
    { { name_OI, 2 }, MF_S360, MF_SI, 0x96, 4 },
    { { name_XI, 2 }, MF_S360, MF_SI, 0x97, 4 },
    { { name_MVC, 3 }, MF_S360, MF_SS, 0xd2, 6 },
    { { name_NC, 2 }, MF_S360, MF_SS, 0xd4, 6 },
    { { name_CLC, 3 }, MF_S360, MF_SS, 0xd5, 6 },
    { { name_OC, 2 }, MF_S360, MF_SS, 0xd6, 6 },
    { { name_XC, 2 }, MF_S360, MF_SS, 0xd7, 6 },
    { { name_TR, 2 }, MF_S360, MF_SS, 0xdc, 6 },
    { { name_TRT, 3 }, MF_S360, MF_SS, 0xdd, 6 },
    { { name_ED, 2 }, MF_S360, MF_SS, 0xde, 6 },
    { { name_EDMK, 4 }, MF_S360, MF_SS, 0xdf, 6 },
    { { name_MVO, 3 }, MF_S360, MF_SS, 0xf1, 6 },
    { { name_PACK, 4 }, MF_S360, MF_SS, 0xf2, 6 },
    { { name_UNPK, 4 }, MF_S360, MF_SS, 0xf3, 6 },
    { { name_ZAP, 3 }, MF_S360, MF_SS, 0xf8, 6 },
    { { name_CP, 2 }, MF_S360, MF_SS, 0xf9, 6 },
    { { name_AP, 2 }, MF_S360, MF_SS, 0xfa, 6 },
    { { name_SP, 2 }, MF_S360, MF_SS, 0xfb, 6 },
    { { name_MP, 2 }, MF_S360, MF_SS, 0xfc, 6 },
    { { name_DP, 2 }, MF_S360, MF_SS, 0xfd, 6 },
    { { name_LPDR, 4 }, MF_S360, MF_RR, 0x20, 2 },
    { { name_LTDR, 4 }, MF_S360, MF_RR, 0x22, 2 },
    { { name_LCDR, 4 }, MF_S360, MF_RR, 0x23, 2 },
    { { name_LDR, 3 }, MF_S360, MF_RR, 0x28, 2 },
    { { name_CDR, 3 }, MF_S360, MF_RR, 0x29, 2 },
    { { name_ADR, 3 }, MF_S360, MF_RR, 0x2a, 2 },
    { { name_SDR, 3 }, MF_S360, MF_RR, 0x2b, 2 },
    { { name_MDR, 3 }, MF_S360, MF_RR, 0x2c, 2 },
    { { name_DDR, 3 }, MF_S360, MF_RR, 0x2d, 2 },
    { { name_LRER, 4 }, MF_S370, MF_RR, 0x35, 2 },
    { { name_STD, 3 }, MF_S360, MF_RX, 0x60, 4 },
    { { name_LD, 2 }, MF_S360, MF_RX, 0x68, 4 },
    { { name_CD, 2 }, MF_S360, MF_RX, 0x69, 4 },
    { { name_AE, 2 }, MF_S360, MF_RX, 0x7a, 4 },
    { { name_AD, 2 }, MF_S360, MF_RX, 0x6a, 4 },
    { { name_SD, 2 }, MF_S360, MF_RX, 0x6b, 4 },
    { { name_MD, 2 }, MF_S360, MF_RX, 0x6c, 4 },
    { { name_DD, 2 }, MF_S360, MF_RX, 0x6d, 4 },
    { { name_STE, 3 }, MF_S360, MF_RX, 0x70, 4 },
    { { name_LE, 2 }, MF_S360, MF_RX, 0x78, 4 },
    { { name_BASR, 4 }, MF_S370, MF_RR, 0x0d, 2 },
    { { name_BAS, 3 }, MF_S370, MF_RX, 0x4d, 4 },
    { { name_STCM, 4 }, MF_S370, MF_RS, 0xbe, 4 },
    { { name_CLM, 3 }, MF_S370, MF_RS, 0xbd, 4 },
    { { name_ICM, 3 }, MF_S370, MF_RS, 0xbf, 4 },
    { { name_CS, 2 }, MF_S370, MF_RS, 0xba, 4 },
    { { name_CDS, 3 }, MF_S370, MF_RS, 0xbb, 4 },
    { { name_MVCL, 4 }, MF_S370, MF_RR, 0x0e, 2 },
    { { name_CLCL, 4 }, MF_S370, MF_RR, 0x0f, 2 },
};

const struct mf_instruction *mf_machine_lookup(enum mf_profile profile,
                                                struct mf_span mnemonic)
{
    size_t i, j;
    mf_octet ch;
    if ((profile != MF_S360 && profile != MF_S370) ||
        (!mnemonic.data && mnemonic.length)) return 0;
    for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); ++i) {
        if (instructions[i].mnemonic.length != mnemonic.length) continue;
        for (j = 0; j < mnemonic.length; ++j) {
            ch = mnemonic.data[j];
            if (ch >= 0x61 && ch <= 0x7a) ch = (mf_octet)(ch - 0x20);
            if (ch != instructions[i].mnemonic.data[j]) break;
        }
        if (j == mnemonic.length) return &instructions[i];
    }
    return 0;
}

static void address(mf_octet *out, unsigned base, mf_u32 displacement)
{
    out[0] = (mf_octet)((base << 4) | (unsigned)(displacement >> 8));
    out[1] = (mf_octet)(displacement & 0xffUL);
}

enum mf_status mf_encode(enum mf_profile profile,
    const struct mf_instruction *instruction, const struct mf_operands *op,
    mf_octet *out, size_t capacity, unsigned *length)
{
    const struct mf_instruction *known;
    mf_octet bytes[6];
    unsigned i, code;
    if (length) *length = 0;
    if (!instruction || !op || !out || !length) return MF_SOURCE;
    if (profile != MF_S360 && profile != MF_S370) return MF_UNSUPPORTED;
    known = mf_machine_lookup(profile, instruction->mnemonic);
    if (!known || known->opcode != instruction->opcode ||
        known->format != instruction->format ||
        known->length != instruction->length ||
        known->minimum_profile != instruction->minimum_profile)
        return MF_UNSUPPORTED;
    if (profile < known->minimum_profile) return MF_UNSUPPORTED;
    if (capacity < known->length) return MF_LIMIT;
    for (i = 0; i < 6; ++i) bytes[i] = 0;
    code = known->opcode;
    bytes[0] = (mf_octet)code;
    switch (known->format) {
    case MF_RR:
        if (code == 0x0a) {
            if (op->immediate > 255 || op->r1 || op->r2) return MF_RANGE;
            bytes[1] = (mf_octet)op->immediate;
        } else {
            if (op->r1 > 15 || op->r2 > 15) return MF_RANGE;
            if (code >= 0x20 && code <= 0x3f &&
                (op->r1 > 6 || (op->r1 & 1) || op->r2 > 6 || (op->r2 & 1))) return MF_RANGE;
            if ((code == 0x1c || code == 0x1d) && (op->r1 & 1))
                return MF_RANGE;
            if ((code == 0x0e || code == 0x0f) &&
                ((op->r1 & 1) || (op->r2 & 1))) return MF_RANGE;
            bytes[1] = (mf_octet)((op->r1 << 4) | op->r2);
        }
        break;
    case MF_RX:
        if (op->r1 > 15 || op->x2 > 15 || op->b2 > 15 || op->d2 > 4095)
            return MF_RANGE;
        if (code >= 0x60 && code <= 0x7f && (op->r1 > 6 || (op->r1 & 1))) return MF_RANGE;
        if ((code == 0x5c || code == 0x5d) && (op->r1 & 1)) return MF_RANGE;
        bytes[1] = (mf_octet)((op->r1 << 4) | op->x2);
        address(bytes + 2, op->b2, op->d2);
        break;
    case MF_RS:
        if (op->r1 > 15 || op->r3 > 15 || op->b2 > 15 || op->d2 > 4095)
            return MF_RANGE;
        if (code >= 0x88 && code <= 0x8f) {
            if (op->r3) return MF_RANGE;
            if (code >= 0x8c && (op->r1 & 1)) return MF_RANGE;
        }
        if (code == 0xbb && ((op->r1 & 1) || (op->r3 & 1))) return MF_RANGE;
        bytes[1] = (mf_octet)((op->r1 << 4) | op->r3);
        address(bytes + 2, op->b2, op->d2);
        break;
    case MF_SI:
        if (op->immediate > 255 || op->b1 > 15 || op->d1 > 4095)
            return MF_RANGE;
        bytes[1] = (mf_octet)op->immediate;
        address(bytes + 2, op->b1, op->d1);
        break;
    case MF_SS:
        if (op->b1 > 15 || op->b2 > 15 || op->d1 > 4095 || op->d2 > 4095)
            return MF_RANGE;
        if (code >= 0xf0) {
            if (!op->length1 || op->length1 > 16 ||
                !op->length2 || op->length2 > 16) return MF_RANGE;
            if ((code == 0xfc || code == 0xfd) &&
                (op->length2 > 8 || op->length1 <= op->length2))
                return MF_RANGE;
            bytes[1] = (mf_octet)(((op->length1 - 1) << 4) |
                                 (op->length2 - 1));
        } else {
            if (!op->length1 || op->length1 > 256 || op->length2)
                return MF_RANGE;
            bytes[1] = (mf_octet)(op->length1 - 1);
        }
        address(bytes + 2, op->b1, op->d1);
        address(bytes + 4, op->b2, op->d2);
        break;
    default:
        return MF_UNSUPPORTED;
    }
    for (i = 0; i < known->length; ++i) out[i] = bytes[i];
    *length = known->length;
    return MF_OK;
}
