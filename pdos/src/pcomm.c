/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcomm - command processor for pdos                               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

static char buf[200];
static size_t len;
static char drive[7] = "PDOS00";
static char cwd[65];
static char prompt[50] = ">";
static int singleCommand = 0;
static int primary = 0;
static int term = 0;
static int showrc = 0;
static int echo = 1;
static unsigned int commandNumber = 0;
static int batchDepth = 0;

static int parseArgs(int argc, char **argv);
static void readAutoExec(void);
static int readPhysicalLine(FILE *fp, const char *source);
static int readConsoleCommand(void);
static void runBatch(FILE *fp, const char *source);
static void processInput(void);
static void putPrompt(void);
static void dotype(char *file);
static void docopy(char *p);
static void dofill(char *p);
static void mkiplmem(char *p);
static void domemdump(char *p);
static void dodir(char *pattern);
static void dohelp(char *topic);
static void changedir(char *to);
static void changedisk(int drive);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);

int main(int argc, char **argv)
{
    if (!parseArgs(argc, argv)) return 2;
    if (singleCommand)
    {
        processInput();
        return (0);
    }
    if (primary)
    {
        printf("welcome to pcomm\n");
        readAutoExec();
    }
    else
    {
        printf("welcome to pcomm - exit to return\n");
    }
    while (!term)
    {
        putPrompt();
        if (!readConsoleCommand()) break;
        processInput();
    }
    printf("thankyou for using pcomm!\n");
    return (0);
}

static int parseArgs(int argc, char **argv)
{
    int x;
    size_t used;
    size_t part;
    
    if (argc > 1)
    {
        if ((argv[1][0] == '-') || (argv[1][0] == '/'))
        {
            if ((argv[1][1] == 'C') || (argv[1][1] == 'c'))
            {
                singleCommand = 1;
            }            
            if ((argv[1][1] == 'P') || (argv[1][1] == 'p'))
            {
                primary = 1;
            }            
        }
    }
    if (singleCommand)
    {
        used = 0;
        buf[0] = '\0';
        for (x = 2; x < argc; x++)
        {
            part = strlen(argv[x]);
            if (used + part + (x > 2 ? 1 : 0) > sizeof buf - 2)
            {
                printf("PCOMM: command exceeds 198 characters\n");
                return 0;
            }
            if (x > 2) buf[used++] = ' ';
            memcpy(buf + used, argv[x], part);
            used += part;
        }
        buf[used] = '\0';
    }
    return 1;
}

/* A physical batch record must fit whole. Never execute a truncated prefix. */
static int readPhysicalLine(FILE *fp, const char *source)
{
    size_t count;
    int c;

    if (fgets(buf, sizeof buf, fp) == NULL) return 0;
    count = strlen(buf);
    if (count == 0 || buf[count - 1] != '\n' || count > sizeof buf - 1)
    {
        while ((c = fgetc(fp)) != EOF && c != '\n') ;
        printf("PCOMM: %s command exceeds 198 characters or lacks a delimiter\n",
               source);
        buf[0] = '\0';
        return -1;
    }
    if (count - 1 > sizeof buf - 2)
    {
        printf("PCOMM: %s command exceeds 198 characters\n", source);
        buf[0] = '\0';
        return -1;
    }
    return 1;
}

/* The 3270 field is shorter than a PCOMM command. A trailing & joins the
   next entered fragment without adding or removing whitespace. */
static int readConsoleCommand(void)
{
    char part[sizeof buf];
    size_t used = 0;
    size_t count;
    int more;
    int c;

    buf[0] = '\0';
    for (;;)
    {
        if (fgets(part, sizeof part, stdin) == NULL) return 0;
        count = strlen(part);
        if (count == 0 || part[count - 1] != '\n')
        {
            while ((c = fgetc(stdin)) != EOF && c != '\n') ;
            printf("PCOMM: console fragment too long; command not run\n");
            buf[0] = '\0';
            return 1;
        }
        count--;
        more = count != 0 && part[count - 1] == '&';
        if (more) count--;
        if (used + count > sizeof buf - 2)
        {
            printf("PCOMM: command exceeds 198 characters; not run\n");
            buf[0] = '\0';
            return 1;
        }
        memcpy(buf + used, part, count);
        used += count;
        if (!more)
        {
            buf[used++] = '\n';
            buf[used] = '\0';
            return 1;
        }
        printf("MORE> ");
        fflush(stdout);
    }
}

static void runBatch(FILE *fp, const char *source)
{
    int status;

    while ((status = readPhysicalLine(fp, source)) != 0)
    {
        if (status > 0) processInput();
        if (term) break;
    }
}

static void readAutoExec(void)
{
    FILE *fp;

    fp = fopen("AUTOEXEC.BAT", "r");
    if (fp != NULL)
    {
        runBatch(fp, "AUTOEXEC.BAT");
        fclose(fp);
    }
    return;
}

static void processInput(void)
{
    char *p;
    int rc;
    char fnm[FILENAME_MAX];
    FILE *fp;    

    if (echo)
    {
        printf("%s", buf);
    }
    len = strlen(buf);
    if ((len > 0) && (buf[len - 1] == '\n'))
    {
        len--;
        buf[len] = '\0';
    }
    p = strchr(buf, ' ');
    if (p != NULL)
    {
        *p++ = '\0';
    }
    else
    {
        p = buf + len;
    }
    len -= (size_t)(p - buf);
    if (ins_strcmp(buf, "exit") == 0)
    {
#ifdef CONTINUOUS_LOOP
        primary = 0;
#endif
        if (1) /* (!primary) */
        {
            term = 1;
        }
    }
    else if (ins_strcmp(buf, "type") == 0)
    {
        dotype(p);
    }
    else if (ins_strcmp(buf, "copy") == 0)
    {
        docopy(p);
    }
    else if (ins_strcmp(buf, "fill") == 0)
    {
        dofill(p);
    }
    else if (ins_strcmp(buf, "mkiplmem") == 0)
    {
        mkiplmem(p);
    }
    else if (ins_strcmp(buf, "memdump") == 0)
    {
        domemdump(p);
    }
/* for now, let PDOS handle this */
/*    else if (ins_strcmp(buf, "dir") == 0)
    {
        dodir(p);
    } */
    else if (ins_strcmp(buf, "echo") == 0)
    {
        if (ins_strcmp(p, "off") == 0)
        {
            echo = 0;
        }
        else if (ins_strcmp(p, "on") == 0)
        {
            echo = 1;
        }
        else
        {
            printf("%s\n", p);
        }
    }
    else if (p == buf)
    {
        /* do nothing if blank line */
    }
    else if (ins_strcmp(buf, "rem") == 0)
    {
        /* ignore comments */
    }
    else if (ins_strcmp(buf, "cd") == 0)
    {
        changedir(p);
    }
    else if (ins_strncmp(buf, "cd.", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if (ins_strncmp(buf, "cd\\", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if (ins_strcmp(buf, "reboot") == 0)
    {
        /* PosReboot(); */
    }
    else if (ins_strcmp(buf, "showrc") == 0)
    {
        showrc = (showrc == 0);
    }
    else if (ins_strcmp(buf, "help") == 0)
    {
        dohelp(p);
    }
    else if (ins_strcmp(buf, "version") == 0)
    {
        printf("z/PDOS PDIO1; PCOMM operator interface 1\n");
        printf("Exact image build: see the host image receipt.\n");
    }
    else if (ins_strcmp(buf, "tso") == 0 ||
             ins_strcmp(buf, "cms") == 0 ||
             ins_strcmp(buf, "cp") == 0)
    {
        printf("%s command environment is not provided by PCOMM.\n", buf);
        printf("Use HELP TSO or HELP CMS for the supported boundary.\n");
    }
#if 0
    else if ((strlen(buf) == 2) && (buf[1] == ':'))
    {
        changedisk(buf[0]);
    }
#endif
    else
    {
        /* see if batch file exists */
        strcpy(fnm, buf);
        strcat(fnm, ".BAT");
        fp = fopen(fnm, "r");
        if (fp != NULL)
        {
            if (batchDepth >= 8)
            {
                printf("PCOMM: nested batch limit reached: %s\n", fnm);
            }
            else
            {
                batchDepth++;
                runBatch(fp, fnm);
                batchDepth--;
            }
            fclose(fp);
            return;
        }
        
        /* restore parameter if there is one*/
        if (*p != '\0')
        {
            p--;
            *p = ' ';
        }
        /* printf("pcomm is calling %s\n", buf); */
        commandNumber++;
        printf("PCOMM BEGIN %u %s\n", commandNumber, buf);
        rc = system(buf);
        printf("PCOMM END %u RC=%d\n", commandNumber, rc);
        if (showrc)
        {
            printf("rc from program is %d\n", rc);
        }
    }
    return;
}

static void putPrompt(void)
{
    printf("\n%s:\\%s%s", drive, cwd, prompt);
    fflush(stdout);
    return;
}

static void dotype(char *file)
{
    FILE *fp;
    
    fp = fopen(file, "r");
    if (fp != NULL)
    {
        while (fgets(buf, sizeof buf, fp) != NULL)
        {
            fputs(buf, stdout);
        }
        fclose(fp);
    }
    else
    {
       printf("file not found: %s\n", file);
    }
    return;
}

static void docopy(char *p)
{
                FILE *fp;
                FILE *fq;
                char *q;

                q = strchr(p, ' ');
                if (q == NULL)
                {
                    printf("two files needed\n");
                }
                else
                {
                    *q = '\0';
                    q++;
                    fp = fopen(p, "rb");
                    if (fp == NULL)
                    {
                        printf("failed to open input file\n");
                    }
                    else
                    {
                        fq = fopen(q, "wb");
                        if (fq == NULL)
                        {
                            printf("failed to open output file\n");
                            fclose(fp);
                        }
                        else
                        {
                            int c;

                            while ((c = fgetc(fp)) != EOF)
                            {
                                fputc(c, fq);
                            }
                            fclose(fp);
                            fclose(fq);
                        }
                    }
                }
    return;
}

static void dofill(char *p)
{
    FILE *fq;
    char *q;
    unsigned long max = 0;
    int infinite = 0;

    if (*p == '\0')
    {
        printf("enter filename and number of bytes to generate\n");
        printf("leave number of bytes blank or zero for infinity\n");
        return;
    }
    q = strchr(p, ' ');
    if (q != NULL)
    {
        *q++ = '\0';
        max = strtoul(q, NULL, 0);
    }
    if (max == 0) infinite = 1;
    fq = fopen(p, "wb");
    if (fq == NULL)
    {
        printf("failed to open %s for output\n", p);
        return;
    }
    while ((max > 0) || infinite)
    {
        putc(0x00, fq);
        if (ferror(fq))
        {
            printf("write error\n");
            break;
        }
        max--;
    }
    fclose(fq);
    return;
}

static void mkiplmem(char *p)
{
    FILE *fp;
    FILE *fq;
    char buf[18452];
    size_t cnt;

    if (*p == '\0')
    {
        printf("usage: mkiplmem <output file>\n");
        printf("produce a pdos.img suitable for direct memory load\n");
        printf("e.g. mkiplmem dev1c2:\n");
        return;
    }
    fp = fopen("pdos.img", "rb");
    if (fp == NULL)
    {
        printf("can't open pdos.img for reading\n");
        return;
    }
    fq = fopen(p, "wb");
    if (fq == NULL)
    {
        printf("can't open %s for writing\n", p);
        fclose(fp);
        return;
    }
    cnt = fread(buf, 1, sizeof buf, fp);
    *(int *)(buf + 4) = *(int *)(buf + 8192 + 12);
    while (cnt != 0)
    {
        fwrite(buf, 1, cnt, fq);
        cnt = fread(buf, 1, sizeof buf, fp);
    }
    if (ferror(fp))
    {
        printf("read error\n");
    }
    if (ferror(fq))
    {
        printf("write error\n");
    }
    fclose(fp);
    fclose(fq);
    return;
}

static void domemdump(char *p)
{
    unsigned char *addr;
    unsigned char *endaddr;
    char prtln[100];
    size_t x;
    int c;
    int pos1;
    int pos2;

    if (*p == '\0')
    {
        printf("usage: memdump address or memdump address1-address2\n");
        return;
    }

        sscanf(p, "%p", &addr);
        endaddr = addr;
        p = strchr(p, '-');
        if (p != NULL)
        {
            sscanf(p + 1, "%p", &endaddr);
        }

        x = 0;
        do
        {
            c = *addr;
            if (x % 16 == 0)
            {
                memset(prtln, ' ', sizeof prtln);
                sprintf(prtln, "%p ", addr);
                prtln[strlen(prtln)] = ' ';
                pos1 = 10;
                pos2 = 47;
            }
            sprintf(prtln + pos1, "%0.2X", c);
            if (isprint((unsigned char)c))
            {
                sprintf(prtln + pos2, "%c", c);
            }
            else
            {
                sprintf(prtln + pos2, ".");
            }
            pos1 += 2;
            *(prtln + pos1) = ' ';
            pos2++;
            if (x % 4 == 3)
            {
                *(prtln + pos1++) = ' ';
            }
            if (x % 16 == 15)
            {
                printf("%s\n", prtln);
            }
            x++;
          /* the while condition takes into account segmented memory where
             the ++ could have caused a wrap back to 0 */
        } while (addr++ != endaddr);
        if (x % 16 != 0)
        {
            printf("%s\n", prtln);
        }

    return;
}

static void dodir(char *pattern)
{
    return;
}

static void dohelp(char *topic)
{
    if (ins_strcmp(topic, "TSO") == 0 || ins_strcmp(topic, "CMS") == 0 ||
        ins_strcmp(topic, "CP") == 0)
    {
        printf("PCOMM is z/PDOS, not a TSO, CMS or CP command environment.\n");
        printf("Native TSO-style load modules may run when their mode and\n");
        printf("services are supported. CMS MODULE execution is not yet supported.\n");
        printf("Datasets are shown with DIR; host tools prepare and check disks.\n");
        return;
    }
    if (ins_strcmp(topic, "ADVANCED") == 0)
    {
        printf("TYPE/COPY/FILL access files. CD and REBOOT are placeholders.\n");
        printf("DUMPBLK, ZAPBLK, NEWBLK, DISKINIT, FIL2DSK, DSK2FIL,\n");
        printf("RAMDISK, MKIPLMEM, MEMDUMP and MEMTEST are development tools.\n");
        printf("Use them only on a disposable disk; see the operator guide.\n");
        return;
    }
    printf("z/PDOS PCOMM: first steps\n");
    printf("VERSION  show interface and build-identity location\n");
    printf("DIR      list datasets, dates, formats and extents\n");
    printf("NAME     run installed NAME.EXE or NAME.BAT\n");
    printf("SHOWRC   toggle extra return-code display\n");
    printf("EXIT     end PCOMM (not a restart)\n");
    printf("Long command: end each field fragment with &; MORE> asks for next.\n");
    printf("Each external run prints PCOMM BEGIN and PCOMM END with its RC.\n");
    printf("HELP TSO, HELP CMS: compatibility boundary. HELP ADVANCED: tools.\n");
    return;
}

static void changedir(char *to)
{
    /* PosChangeDir(to); */
    return;
}

static void changedisk(int drive)
{
    /* PosSelectDisk(toupper(drive) - 'A'); */
    return;
}

static int ins_strcmp(char *one, char *two)
{
    while (toupper(*one) == toupper(*two))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
    }
    if (toupper(*one) < toupper(*two))
    {
        return (-1);
    }
    return (1);
}

static int ins_strncmp(char *one, char *two, size_t len)
{
    size_t x = 0;
    
    if (len == 0) return (0);
    while ((x < len) && (toupper(*one) == toupper(*two)))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
        x++;
    }
    if (x == len) return (0);
    return (toupper(*one) - toupper(*two));
}
