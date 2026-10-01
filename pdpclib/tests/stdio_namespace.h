/* Host QA namespace: never replace the host C library stdio symbols. */
#ifndef PDPCLIB_QA_STDIO_NAMESPACE_H
#define PDPCLIB_QA_STDIO_NAMESPACE_H
#define printf pdpqa_printf
#define fprintf pdpqa_fprintf
#define vfprintf pdpqa_vfprintf
#define vprintf pdpqa_vprintf
#define fopen pdpqa_fopen
#define fclose pdpqa_fclose
#define fread pdpqa_fread
#define fwrite pdpqa_fwrite
#define fputc pdpqa_fputc
#define fputs pdpqa_fputs
#define remove pdpqa_remove
#define rename pdpqa_rename
#define sprintf pdpqa_sprintf
#define vsprintf pdpqa_vsprintf
#define fgets pdpqa_fgets
#define ungetc pdpqa_ungetc
#define fgetc pdpqa_fgetc
#define fseek pdpqa_fseek
#define ftell pdpqa_ftell
#define fsetpos pdpqa_fsetpos
#define fgetpos pdpqa_fgetpos
#define rewind pdpqa_rewind
#define clearerr pdpqa_clearerr
#define perror pdpqa_perror
#define setvbuf pdpqa_setvbuf
#define setbuf pdpqa_setbuf
#define freopen pdpqa_freopen
#define fflush pdpqa_fflush
#define tmpnam pdpqa_tmpnam
#define tmpfile pdpqa_tmpfile
#define fscanf pdpqa_fscanf
#define scanf pdpqa_scanf
#define sscanf pdpqa_sscanf
#define gets pdpqa_gets
#define puts pdpqa_puts
#endif
