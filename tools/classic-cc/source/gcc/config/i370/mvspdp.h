/* Definitions of target machine for GNU compiler.  System/370 version.
   Copyright (C) 1989, 1993, 1995, 1996, 1997 Free Software Foundation, Inc.
   Contributed by Jan Stein (jan@cd.chalmers.se).
   Modified for OS/390 LanguageEnvironment C by Dave Pitts (dpitts@cozx.com)

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#define TARGET_VERSION fprintf (stderr, " (370/MVS PDPMAC)");

/* Specify that we're generating code for MVS.  */

#define TARGET_MVS 1
#define TARGET_HLASM 1
#define TARGET_EBCDIC 1

/* Use the PDPCLIB-style macro prolog/epilog (PDPPRLG / PDPEPIL), as
   expected by the crent370 runtime.  The i370 back end emits these for
   TARGET_PDPMAC.  */

#define TARGET_PDPMAC 1

/* One-shot link (cc370 foo.c -o foo.lm): the driver invokes ld370 (the target
   `ld`).  Pull the crt0 startup as the first object (so the @@CRT0 startup is at
   module offset 0), set the entry to @@CRT0, and emit no -lgcc -- the crent libc
   (libc.a) carries the compiler-support routines, so there is no separate
   libgcc.  crt0.o / libc.a live in the sysroot lib, found via the -L paths the
   driver already passes.  */
#define STARTFILE_SPEC "crt0.o%s"
#undef  LIBGCC_SPEC
#define LIBGCC_SPEC ""
#undef  LINK_SPEC
/* -flinker-output=TYPE picks the MVS transport wrapper ld370 ALSO emits beside the
   load-module member: xmit (TSO TRANSMIT/NETDATA) -> OUT.xmit, iebcopy (unloaded
   PDS) -> OUT.iebcopy.  Default (no flag) = just the member. */
#define LINK_SPEC "--entry @@CRT0 %{flinker-output=xmit:-xmit} %{flinker-output=iebcopy:-iebcopy}"

/* Specify that we're using macro prolog/epilog.  */

#define TARGET_MACROS 1

/* Emit fixed-column, blank-delimited assembler text instead of the back
   end's tab-separated layout.  The crent370 / MVS 3.8j tool chain assembles
   the output with IFOX00 (Assembler XF), which does not treat the horizontal
   tab as a field separator, so tab-separated output fails with "IFO053 OP
   CODE NOT FOUND" on every statement.  The implementation lives in
   config/i370/i370.c, fenced by this same macro.  Remove this define once the
   tool chain emits object code (or feeds an assembler that accepts tabs) and
   the column text is no longer needed.  */

#define I370_IFOX_COLUMNS 1

/* Do not materialise string constants via store_by_pieces / move-by-pieces
   immediates.  The generic builtin expanders (memcpy / mempcpy / strncpy of a
   string literal in config/../builtins.c) can copy the literal's bytes
   straight into immediate operands (e.g. MVC 0(4,r),=F'<word>').  Those bytes
   are the host (ASCII) representation of the string, whereas the constant pool
   is assembled by IFOX00 from C'...' text and is therefore EBCDIC.  Inlining
   thus emits e.g. "ARRY" as ASCII 0x41525259 while every comparison against
   the pooled C'ARRY' sees EBCDIC 0xC1D9D9E8, so the two never match.

   Defining this macro fences off those store_by_pieces paths so the copy falls
   back to emit_block_move / a library call, which reads the in-memory (EBCDIC)
   literal -- matching the gccmvs 3.2.3 (c2asm370) behaviour the crent370
   runtime was built and validated against.  Fenced by this same macro in
   config/i370/builtins paths; defined only here (mvspdp).  */

#define I370_NO_INLINE_STRING_CONST 1

/* Options for the preprocessor for this target machine.  */

#define CPP_SPEC "-trigraphs"

/* Target OS preprocessor built-ins.  Match the predefines of the c2asm370
   (gccmvs 3.2.3) compiler, which crent370 and the other runtimes key on:
   originally CPP_PREDEFINES "-D__GCC__ -D__MVS__ -Asystem=mvs -Acpu=i370
   -Amachine=i370".  In particular crent370's headers gate size_t and much
   else on __MVS__.

   cpu=i370 / machine=i370 are CPU (not OS) attributes and are already asserted
   by TARGET_CPU_CPP_BUILTINS() in i370.h; asserting them again here made cpp
   warn "cpu/machine re-asserted" on every compile, so only system=mvs lives
   here.  */
#define TARGET_OS_CPP_BUILTINS()               \
    do {                                       \
       builtin_define ("__GCC__");             \
       builtin_define ("__MVS__");             \
       builtin_assert ("system=mvs");          \
    } while (0)
