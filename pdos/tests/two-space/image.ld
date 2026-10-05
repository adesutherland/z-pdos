OUTPUT_FORMAT("elf64-s390")
OUTPUT_ARCH(s390:64-bit)
ENTRY(boot)
PHDRS {
 low PT_LOAD FLAGS(6);
 boot PT_LOAD FLAGS(5);
 result PT_LOAD FLAGS(6);
 control PT_LOAD FLAGS(6);
 app PT_LOAD FLAGS(5);
 apphigh PT_LOAD FLAGS(5);
 appdata PT_LOAD FLAGS(6);
 kerneldata PT_LOAD FLAGS(6);
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
 /DISCARD/ : { *(.comment) *(.note*) }
 ASSERT(SIZEOF(.boot) <= 0x1000, "entry exceeds one page")
 ASSERT(SIZEOF(.app) <= 0x1000, "24-bit app exceeds one page")
 ASSERT(SIZEOF(.apphigh) <= 0x1000, "high app exceeds one page")
}
