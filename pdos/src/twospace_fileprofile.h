/* SPDX-License-Identifier: MIT
 * The disposable large-file profile is selected at compile and media-build
 * time. It does not reinterpret the released/default STORE layout. */
#ifndef PDOS_TWO_SPACE_FILEPROFILE_H
#define PDOS_TWO_SPACE_FILEPROFILE_H
#if defined(TSF_LARGE_FILES)
#define TSFP_OBJECT_LIMIT 0x01000000U
#define TSFP_CMS_INPUT_LIMIT 0x01000000U
#define TSFP_LAYOUT 2U
#define TSFP_CYLINDERS 512U
#else
#define TSFP_OBJECT_LIMIT 0x00200000U
#define TSFP_CMS_INPUT_LIMIT 0x00300000U
#define TSFP_LAYOUT 1U
#if defined(TSF_BATCH_PACKAGE)
#define TSFP_CYLINDERS 150U
#else
#define TSFP_CYLINDERS 100U
#endif
#endif
#endif
