/* SPDX-License-Identifier: MIT; actual neutral service, not simulated I/O. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_batch.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void init(unsigned char *r)
{memset(r,0,64U);put(r,1U);put(r+4U,64U);}
static void pointer(unsigned char *p,const void *v)
{put(p,0U);put(p+4U,(unsigned int)(unsigned long)v);}
static unsigned int open_device(unsigned char *r,unsigned int kind,unsigned int address)
{init(r);put(r+12U,kind);put(r+16U,address);return TUIIO(64U,r,TSA_IO_BATCH_OPEN);}
static unsigned int close_device(unsigned char *r,unsigned int handle)
{init(r);put(r+8U,handle);return TUIIO(64U,r,TSA_IO_BATCH_CLOSE);}
static unsigned int transfer(unsigned char *r,unsigned int handle,unsigned int op,unsigned int command,unsigned char *data,unsigned int n)
{init(r);put(r+8U,handle);put(r+20U,command);put(r+24U,n);pointer(r+32U,data);return TUIIO(64U,r,op);}
static unsigned int say(const char *text)
{return TUILINE((const unsigned char *)text,(unsigned int)strlen(text));}
static int wait_action(const char *prompt)
{unsigned char reply[256];unsigned int n=0U;return say(prompt)||TUIREAD(reply,sizeof reply,&n)||n!=2U||memcmp(reply,"GO",2U);}
static int unchanged(const unsigned char *p)
{unsigned int i;for(i=0U;i<80U;++i)if(p[i]!=0xa5U)return 0;return 1;}
static int padded(const unsigned char *p,const char *prefix)
{unsigned int i,n=(unsigned int)strlen(prefix);if(memcmp(p,prefix,n))return 0;for(i=n;i<80U;++i)if(p[i]!=0x40U)return 0;return 1;}
#define CHECK(x) do {if(!(x)){printf("PD033 FAILURE LINE %u\n",(unsigned int)__LINE__);return __LINE__;}} while(0)
int main(int argc,char **argv)
{
    unsigned char r[64],card[80],line[132],saved[64];unsigned int reader,printer,old,i,rc;
    int cleanup=argc>1&&!strcmp(argv[1],"cleanup");
    CHECK(!TUIOPEN((const unsigned char *)"PD033 device proof",sizeof "PD033 device proof"-1U));
    CHECK(open_device(r,TBC_READER,0xffffU)==20U);
    CHECK(open_device(r,TBC_READER,9U)==20U);
    CHECK(open_device(r,TBC_PRINTER,12U)==20U);
    CHECK(!open_device(r,TBC_READER,12U));reader=word(r+8U);
    CHECK(open_device(r,TBC_READER,12U)==16U);
    CHECK(!open_device(r,TBC_PRINTER,14U));printer=word(r+8U);
    if(cleanup){
        memset(card,0xa5,sizeof card);
        CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+60U)==TSA_BATCH_EOF&&unchanged(card));
        memcpy(line,"PD033 CLEANUP PASS",sizeof "PD033 CLEANUP PASS"-1U);
        CHECK(!transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,sizeof "PD033 CLEANUP PASS"-1U));
        CHECK(!say("PD033 CLEANUP PASS"));return 0;
    }
    memset(card,0xa5,sizeof card);
    CHECK(transfer(r,reader,TSA_IO_BATCH_READ,2U,card,79U)==8U&&unchanged(card));
    init(r);put(r+8U,reader);put(r+24U,80U);put(r+32U,1U);memcpy(saved,r,64U);
    CHECK(TUIIO(64U,r,TSA_IO_BATCH_READ)==8U&&!memcmp(saved,r,64U));
    init(r);put(r+8U,reader);put(r+24U,80U);pointer(r+32U,r);memcpy(saved,r,64U);
    CHECK(TUIIO(64U,r,TSA_IO_BATCH_READ)==8U&&!memcmp(saved,r,64U));
    CHECK(transfer(r,reader,TSA_IO_BATCH_READ,6U,card,80U)==20U);
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+28U)==80U&&padded(card,"PD033 FIRST DECK"));
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+28U)==80U);
    for(i=0U;i<80U;++i)CHECK(card[i]==i);
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U));
    for(i=0U;i<80U;++i)CHECK(card[i]==0x40U);
    memset(card,0xa5,sizeof card);
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+60U)==TSA_BATCH_EOF&&!word(r+28U)&&unchanged(card));
    old=reader;CHECK(!close_device(r,reader));CHECK(close_device(r,old)==8U);
    CHECK(!open_device(r,TBC_READER,12U));reader=word(r+8U);CHECK(reader!=old);
    memcpy(line,"PD033 PRINT FIRST",sizeof "PD033 PRINT FIRST"-1U);
    CHECK(!transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,sizeof "PD033 PRINT FIRST"-1U));
    memset(line,'X',sizeof line);CHECK(!transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,132U)&&word(r+28U)==132U);
    CHECK(transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,133U)==8U);
    memcpy(line,"PD033 PAGE END",sizeof "PD033 PAGE END"-1U);
    CHECK(!transfer(r,printer,TSA_IO_BATCH_WRITE,0x89U,line,sizeof "PD033 PAGE END"-1U));
    CHECK(!wait_action("PD033 RELOAD READY"));
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&padded(card,"PD033 RELOADED DECK"));
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+60U)==TSA_BATCH_EOF);
    CHECK(!wait_action("PD033 INTERVENTION READY"));memset(card,0xa5,sizeof card);
    rc=transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U);
    CHECK(rc==12U&&(word(r+40U)&2U)&&word(r+48U)&&!word(r+60U)&&!word(r+28U)&&unchanged(card));
    /* Host framing refusal leaves the intervention-required reader unchanged. */
    CHECK(!wait_action("PD033 DECK REJECT READY"));memset(card,0xa5,sizeof card);
    CHECK(transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)==12U&&!word(r+60U)&&!word(r+28U)&&unchanged(card));
    CHECK(!wait_action("PD033 PRINTER OFFLINE READY"));
    CHECK(transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,1U)==12U&&!word(r+28U));
    CHECK(!close_device(r,printer));
    CHECK(!wait_action("PD033 RESTORE READY"));
    CHECK(!open_device(r,TBC_PRINTER,14U));printer=word(r+8U);
    memcpy(line,"PD033 PRINT RECOVERED",sizeof "PD033 PRINT RECOVERED"-1U);
    CHECK(!transfer(r,printer,TSA_IO_BATCH_WRITE,9U,line,sizeof "PD033 PRINT RECOVERED"-1U));
    CHECK(!transfer(r,reader,TSA_IO_BATCH_READ,2U,card,80U)&&word(r+60U)==TSA_BATCH_EOF);
    CHECK(!say("PD033 DEVICE PASS"));
    /* Leave both endpoints owned: a second invocation proves native cleanup. */
    return 0;
}
