# z/PDOS 0.1 hardware setup

The [recorded qualification](../../pdos/doc/qualification/QUALIFICATION.md)
uses standard z/Architecture under the z900 instruction ceiling, one CPU,
4,096 MiB real storage, a 100-cylinder 3390 at 01B9 and an IBM1047 3278-2
console at 0009. The recorded emulator is Hercules 4.9.1.0-SDL.

The kernel is separately C32/AMODE31/RMODE24; PCOMM is AMODE31/RMODE ANY.
Native AMODE64 application contexts do not turn that kernel into a 64-bit
C backend or qualify an unrelated named Classic selector. The pdos-zarch
PDPCLIB selection is an OS/service configuration, not a community S/380 ISA.
The [OS backlog](../../pdos/doc/BACKLOG.md) owns future kernel work.
