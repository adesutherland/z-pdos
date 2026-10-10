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
#include "zpdos-version.h"
#ifdef PDOS_TWO_SPACE
#include "twospace_ui.h"
#endif

static char buf[200];
static size_t len;
static char drive[7] = "PDOS00";
static char cwd[65];
static char prompt[50] = ">";
static int singleCommand = 0;
static int primary = 0;
static int term = 0;
#ifdef PDOS_TWO_SPACE
static int showrc = 1;
#else
static int showrc = 0;
#endif
static int echo = 1;
static unsigned int commandNumber = 0;
static int batchDepth = 0;

#ifdef PDOS_TWO_SPACE
static int commandRCValid;
static char recalled[16][199];
static unsigned int recallCount,recallNext,recallPosition;
static unsigned int resultWord(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void reportCommand(int *rc)
{
    unsigned char result[TSA_RESULT_BYTES];
    TUICTRL(1U);
    commandRCValid=0;
    if(TUIRESULT(result)==0U){
        unsigned int status=resultWord(result+TSA_RESULT_OS_STATUS);
        commandRCValid=resultWord(result+TSA_RESULT_RC_VALID)!=0U;
        if(status||!commandRCValid){
            printf("PCOMM END %u OS=%u RC=unavailable\n",commandNumber,status);
            fflush(stdout);TUICTRL(0U);return;
        }
        *rc=(int)resultWord(result+TSA_RESULT_APP_RC);
    }else commandRCValid=1;
    printf("PCOMM END %u RC=%d\n",commandNumber,*rc);fflush(stdout);TUICTRL(0U);
}
#endif

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
#ifdef PDOS_TWO_SPACE
    if (TUIOPEN((const unsigned char *)"z/PDOS PCOMM",12U)||TUISHELL()) return 2;
#endif
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
#ifdef PDOS_TWO_SPACE
    if (TUICLOSE()) return 2;
#endif
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
static void inputNotice(const char *message)
{
#ifdef PDOS_TWO_SPACE
    TUICTRL(2U);
#endif
    printf("%s\n",message);fflush(stdout);
#ifdef PDOS_TWO_SPACE
    TUICTRL(0U);
#endif
}
static int readConsoleCommand(void)
{
#ifdef PDOS_TWO_SPACE
    char part[257];
    unsigned int bytes,aid,index;
#else
    char part[sizeof buf];
#endif
    size_t used = 0;
    size_t count;
    int more;
#ifndef PDOS_TWO_SPACE
    int c;
#endif

    buf[0] = '\0';
#ifdef PDOS_TWO_SPACE
    if(TUIJOIN(0U,0U))return 0;
#endif
    for (;;)
    {
#ifdef PDOS_TWO_SPACE
        if(TUIKEY((unsigned char *)part,256U,&bytes,&aid))return 0;
        if(aid==0x6dU){used=0U;buf[0]='\0';if(TUIJOIN(0U,0U))return 0;continue;}
        if(aid==0xf5U||aid==0xf6U){
            if(recallCount){
                used=0U;buf[0]='\0';if(TUIJOIN(0U,0U))return 0;
                if(aid==0xf5U){if(recallPosition<recallCount)recallPosition++;}
                else if(recallPosition)recallPosition--;
                if(!recallPosition){part[0]='\0';bytes=0U;}
                else{index=(recallNext+16U-recallPosition)%16U;strcpy(part,recalled[index]);bytes=(unsigned int)strlen(part);}
                if(TUISET((unsigned char *)part,bytes,bytes))return 0;
            }
            continue;
        }
        if(aid!=0x7dU)continue;
        part[bytes]='\0';count=bytes;
        if(strlen(part)!=count){inputNotice("PCOMM: binary command input refused");buf[0]='\0';return 1;}
#else
        if (fgets(part, sizeof part, stdin) == NULL) return 0;
        count = strlen(part);
        if (count == 0 || part[count - 1] != '\n')
        {
            while ((c = fgetc(stdin)) != EOF && c != '\n') ;
            inputNotice("PCOMM: console fragment too long; command not run");
            buf[0] = '\0';
            return 1;
        }
        count--;
#endif
        more = count != 0 && part[count - 1] == '&';
        if (more) count--;
        if (used + count > sizeof buf - 2)
        {
            inputNotice("PCOMM: command exceeds 198 characters; not run");
            buf[0] = '\0';
            return 1;
        }
        memcpy(buf + used, part, count);
        used += count;
        if (!more)
        {
#ifdef PDOS_TWO_SPACE
            if(TUIJOIN(0U,0U))return 0;
            if(used){buf[used]='\0';strcpy(recalled[recallNext],buf);recallNext=(recallNext+1U)%16U;if(recallCount<16U)recallCount++;}recallPosition=0U;
#endif
            buf[used++] = '\n';
            buf[used] = '\0';
            return 1;
        }
#ifdef PDOS_TWO_SPACE
        if(TUIJOIN(1U,(unsigned int)used))return 0;TUICTRL(2U);
#endif
        printf("MORE> ");
        fflush(stdout);
#ifdef PDOS_TWO_SPACE
        TUICTRL(0U);
#endif
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
    int rc = 0;
    char fnm[FILENAME_MAX];
    FILE *fp;    

    if (echo)
    {
#ifdef PDOS_TWO_SPACE
        TUICTRL(1U);
#endif
        printf("%s", buf);
#ifdef PDOS_TWO_SPACE
        fflush(stdout);TUICTRL(0U);
#endif
    }
    len = strlen(buf);
    if ((len > 0) && (buf[len - 1] == '\n'))
    {
        len--;
        buf[len] = '\0';
    }
#ifdef PDOS_TWO_SPACE
    if(!len)return;
    if(len&&TUIJOB((const unsigned char *)buf,(unsigned int)len)){printf("PCOMM: console command state unavailable\n");return;}
#endif
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
        printf("z/PDOS %s; PDIO1; PCOMM operator interface 1\n", ZPDOS_VERSION);
        printf("Exact image build: see the host image receipt.\n");
    }
#ifdef PDOS_TWO_SPACE
    else if (ins_strcmp(buf, "console") == 0)
    {
        if(ins_strcmp(p,"status")==0){
            unsigned char caps[64],monitor[32];
            rc=(int)TUICAPS(caps);if(!rc)rc=(int)TUIMON(monitor);
            if(!rc){
                printf("CONSOLE: %s / device %04X / %ux%u\n",
                    resultWord(caps+8U)==TSA_DEVICE_3270?"3270":"line",
                    resultWord(caps+12U),resultWord(caps+16U),resultWord(caps+20U));
                printf("CONSOLE: address format %u / features %08X / generation %u\n",
                    resultWord(caps+40U),resultWord(caps+48U),resultWord(caps+60U));
                printf("CONSOLE: panels %s / colour %s / key events %s\n",
                    resultWord(caps+48U)&TSA_FEATURE_WORKBENCH?"yes":"no",
                    resultWord(caps+48U)&TSA_FEATURE_COLOUR?"yes":"no",
                    resultWord(caps+48U)&TSA_FEATURE_KEY_EVENTS?"yes":"no");
                printf("CONSOLE: monitor %04X state %u / input %s / capture gaps %u\n",
                    resultWord(monitor+8U),resultWord(monitor+12U),
                    resultWord(monitor+20U)?"monitor":"primary",resultWord(monitor+16U));
            }
        }
        else if(ins_strcmp(p,"input primary")==0)rc=(int)TUIHAND(0U);
        else if(ins_strcmp(p,"input monitor")==0)rc=(int)TUIHAND(1U);
        else {printf("CONSOLE STATUS | INPUT PRIMARY|MONITOR\n");rc=8;}
        if(rc)printf("PCOMM: input selection unavailable (%d)\n",rc);
    }
#endif
    else if (ins_strcmp(buf, "devices") == 0 ||
             ins_strcmp(buf, "volumes") == 0 ||
             ins_strcmp(buf, "mount") == 0 ||
             ins_strcmp(buf, "select") == 0 ||
             ins_strcmp(buf, "unmount") == 0 ||
             ins_strcmp(buf, "alloc") == 0 ||
             ins_strcmp(buf, "rcopy") == 0 ||
             ins_strcmp(buf, "tape") == 0 ||
             ins_strcmp(buf, "cms") == 0)
    {
        int select_volume = ins_strcmp(buf, "select") == 0;
        char selected[7];
        selected[0] = '\0';
        if (select_volume && strlen(p) == 6)
        {
            size_t i;
            memcpy(selected, p, 6);
            selected[6] = '\0';
            for (i = 0; i < 6; i++)
                selected[i] = toupper((unsigned char)selected[i]);
        }
        if (*p != '\0') p[-1] = ' ';
        commandNumber++;
#ifdef PDOS_TWO_SPACE
        TUIEXT();TUICTRL(1U);
#endif
        printf("PCOMM BEGIN %u %s\n", commandNumber, buf);
#ifdef PDOS_TWO_SPACE
        fflush(stdout);TUICTRL(0U);
#endif
        rc = system(buf);
#ifdef PDOS_TWO_SPACE
        reportCommand(&rc);
#else
        printf("PCOMM END %u RC=%d\n", commandNumber, rc);
#endif
        if (select_volume && rc == 0 && selected[0] != '\0')
            strcpy(drive, selected);
    }
    else if (ins_strcmp(buf, "tso") == 0 ||
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
#ifdef PDOS_TWO_SPACE
        TUIEXT();TUICTRL(1U);
#endif
        printf("PCOMM BEGIN %u %s\n", commandNumber, buf);
#ifdef PDOS_TWO_SPACE
        fflush(stdout);TUICTRL(0U);
#endif
        rc = system(buf);
#ifdef PDOS_TWO_SPACE
        reportCommand(&rc);
#else
        printf("PCOMM END %u RC=%d\n", commandNumber, rc);
#endif
        if (showrc
#ifdef PDOS_TWO_SPACE
            && commandRCValid
#endif
            )
        {
#ifdef PDOS_TWO_SPACE
            TUICTRL(1U);
#endif
            printf("rc from program is %d\n", rc);
#ifdef PDOS_TWO_SPACE
            fflush(stdout);TUICTRL(0U);
#endif
        }
    }
#ifdef PDOS_TWO_SPACE
    TUIEND(0U,1U,(unsigned int)rc);
    TUIVOL((const unsigned char *)drive,6U);
#endif
    return;
}

static void putPrompt(void)
{
#ifdef PDOS_TWO_SPACE
    TUICTRL(1U);
#endif
    printf("\n%s:\\%s%s", drive, cwd, prompt);
    fflush(stdout);
#ifdef PDOS_TWO_SPACE
    TUICTRL(0U);
#endif
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
#ifdef PDOS_TWO_SPACE
    if (ins_strcmp(topic,"EDIT")==0) {
        printf("EDIT dataset (optional): 4096 native FB/VB records, 256 bytes each.\n");
        printf("INPUT/INSERT, PRINT, DELETE, CHANGE, LOCATE, UNDO.\n");
        printf("SAVE AS separate empty exchange dataset; QUIT or QUIT DISCARD.\n");
        printf("Use VOLSER:dataset for a mounted volume. EDIT HELP lists syntax.\n");
        return;
    }
    if (ins_strcmp(topic,"UTILITIES")==0) {
        printf("DISKMAP: DASD graph; Enter refresh / N volume / Q return.\n");
        printf("FIND literal dataset; HEX dataset; CMP/FC first second.\n");
        printf("HELLO args, RECIO source target and PANEL are C examples.\n");
        printf("EXAMPLE.HELLO/RECIO/PANEL/REXX are native source datasets.\n");
        return;
    }
#endif
    if (ins_strcmp(topic, "CMS") == 0)
    {
        printf("CMS CHECK 31 RXVM|RXAS|RXC validates a staged MODULE.\n");
        printf("CMS CHECK 24 RXVM validates the fixed-origin format.\n");
        printf("CMS RUN 31 RXVM|RXAS|RXC args starts checked CMS31 modules.\n");
        printf("CMS RUN 24 RXVM args starts the fixed-origin CMS24 runtime.\n");
        printf("Arguments are blank-separated tokens of at most 8 characters.\n");
        printf("Select a checked exchange volume before CMS CHECK or RUN.\n");
        return;
    }
    if (ins_strcmp(topic, "TSO") == 0 || ins_strcmp(topic, "CP") == 0)
    {
        printf("PCOMM is z/PDOS, not a TSO, CMS or CP command environment.\n");
        printf("Native TSO-style load modules may run when their mode and\n");
        printf("services are supported. HELP CMS lists the CMS qualification.\n");
        printf("Datasets are shown with DIR; host tools prepare and check disks.\n");
#ifdef PDOS_TWO_SPACE
        printf("The normal development image bundles cREXX beta.3 TSO31.\n");
        printf("RXC, RXAS, RXVM; use -l CREXX for the bundled library.\n");
#endif
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
#ifdef PDOS_TWO_SPACE
    printf("CONSOLE STATUS; HELP EDIT; HELP UTILITIES; HELP TSO\n");
#endif
    printf("VERSION  show release version, interface and build-identity location\n");
    printf("DIR      list datasets, dates, formats and extents\n");
    printf("DEVICES  list attached addresses; VOLUMES lists mounted DASD\n");
    printf("MOUNT address volser; SELECT volser; UNMOUNT volser\n");
    printf("ALLOC name FB|VB|U lrecl blksize cylinders on selected disk\n");
    printf("RCOPY source target  preserve FB/VB logical records exactly\n");
    printf("TAPE STATUS, MOUNT address, REWIND, READ, SCAN\n");
    printf("TAPE MOUNT address WRITE for output; WRITE hex, MARK, OFF\n");
    printf("CMS CHECK validates staged MODULEs; HELP CMS shows the run gate.\n");
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
