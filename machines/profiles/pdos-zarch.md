# z/PDOS hardware setup

The 0.2 K/U [P6 qualification](../../pdos/doc/qualification/TWO-SPACE-P6-2026-10-07.md)
uses standard z/Architecture under the z900 instruction ceiling, Hercules
4.9.1.0-SDL model 2064, one CPU and **256 MiB real storage**. The IPL device
is a 100-cylinder 3390 at 01B9. Primary 0009 is a configured 3270 model 2–5
or line device; optional 000A is a Telnet 3215 monitor. Native console text
uses IBM1047 with Hercules `CODEPAGE 819/1047`.

The kernel has an AMODE64 assembler nucleus and protected Classic C31
services in K. U PCOMM is AMODE31; unchanged CMS/TSO applications use their
declared AMODE24/31/64 in one shared U ASCE. These are separate ABI and
runtime choices, not a community S/380 ISA or a native 64-bit C backend.
The [0.2.0 record](../../pdos/doc/qualification/RELEASE-0.2.0.md) names exact
release/operator inputs. Earlier 0.1 qualification used a one-space
C32/AMODE31 kernel and 4096 MiB; that remains historical evidence.
