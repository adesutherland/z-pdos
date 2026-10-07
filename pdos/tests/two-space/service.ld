OUTPUT_FORMAT("elf64-s390")
OUTPUT_ARCH(s390:64-bit)
ENTRY(boot)
PHDRS {
 low PT_LOAD FLAGS(6); boot PT_LOAD FLAGS(5); result PT_LOAD FLAGS(6);
 control PT_LOAD FLAGS(6); app PT_LOAD FLAGS(5); apphigh PT_LOAD FLAGS(5);
 appdata PT_LOAD FLAGS(6); kerneldata PT_LOAD FLAGS(6);
 kcore PT_LOAD FLAGS(5); ktramp PT_LOAD FLAGS(5);
 appb PT_LOAD FLAGS(5); apphighdata PT_LOAD FLAGS(6);
 apphighdata2 PT_LOAD FLAGS(6); apphelper PT_LOAD FLAGS(5);
}
SECTIONS {
 .lowcore 0 : { KEEP(*(.lowcore)) } :low
 .boot 0x1000 : { *(.boot) } :boot
 .result 0x2000 : { *(.result) } :result
 .control 0x4000 : { *(.control) } :control
 .app 0x20000 : AT(0x5000) { *(.app) } :app
 .apphigh 0x110000000 : AT(0x6000) { *(.apphigh) } :apphigh
 .appdata 0x21000 : AT(0x7000) { *(.appdata) } :appdata
 .kerneldata 0x22000 : AT(0x8000) { *(.kerneldata) } :kerneldata
 .kcore 0x0100000000000000 : AT(0x9000) { *(.kcore) } :kcore
 .ktramp 0x02060000 : AT(0xf000) { *(.ktramp) } :ktramp
 .appb 0x02000000 : AT(0x11000) { *(.appb) } :appb
 .apphighdata 0x110001000 : AT(0x12000) { *(.apphighdata) } :apphighdata
 .apphighdata2 0x110002000 : AT(0x13000) { *(.apphighdata2) } :apphighdata2
 .apphelper 0x110003000 : AT(0x1f000) { *(.apphelper) } :apphelper
 /DISCARD/ : { *(.comment) *(.note*) }
 ASSERT(SIZEOF(.boot) <= 0x1000, "real entry exceeds one page")
 ASSERT(SIZEOF(.kcore) <= 0x1000, "K64 core exceeds one page")
 ASSERT(SIZEOF(.ktramp) <= 0x1000, "C31 trampoline exceeds one page")
 ASSERT(SIZEOF(.app) <= 0x1000, "24-bit app exceeds one page")
}
