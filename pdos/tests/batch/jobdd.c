/* SPDX-License-Identifier: MIT; independent byte and ownership controls. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "twospace_job.h"
#include "twospace_jobdd.h"
static unsigned char pool[TJB_BYTES],saved[TJB_BYTES],d[TJD_BYTES],job[TJB_DESC_BYTES];
static unsigned char got[512];static unsigned int used,visits;
static void put(unsigned char *p,unsigned int n){TJBPUT(p,n);}
static void ebname(unsigned char *p,unsigned int c)
{memset(p,0x40,8U);p[0]=(unsigned char)c;}
static unsigned char *entry(unsigned int kind,unsigned int fmt,unsigned int lr,unsigned int bl)
{unsigned char *e=d+32U;memset(d,0,sizeof d);put(d,1U);put(d+4U,TJD_BYTES);put(d+8U,1U);put(d+12U,2U);put(d+16U,1U);
 put(e,1U);put(e+4U,kind);put(e+8U,kind==TJD_SYSOUT?TJD_OUTPUT:TJD_INPUT);
 put(e+12U,fmt);put(e+16U,lr);put(e+20U,bl);ebname(e+32U,0xe2U);return e;}
static int visit(void *unused,const unsigned char *p,unsigned int n)
{(void)unused;++visits;got[used++]=(unsigned char)n;memcpy(got+used,p,n);used+=n;return 0;}
int main(void)
{
 unsigned char *e,block[512],*dd;unsigned int cursor,n,cost,id,i;TJBPOOL p;
 const unsigned char vb[]={0,17,0,0,0,4,0,0,0,9,0,0,0,0xff,0x40,0x15,0x7f};
 e=entry(TJD_INLINE,TJD_VB,9U,32U);put(e+24U,TJD_DATA);put(e+28U,9U);
 d[TJD_DATA+3U]=5U;d[TJD_DATA+5U]=0xffU;d[TJD_DATA+6U]=0x40U;d[TJD_DATA+7U]=0x15U;d[TJD_DATA+8U]=0x7fU;
 assert(!TJDCHK(d,2U));cursor=0U;n=999U;
 assert(!TJDBLOCK(e,d+TJD_DATA,9U,&cursor,block,sizeof block,&n)&&cursor==9U&&n==sizeof vb&&!memcmp(block,vb,n));
 assert(TJDBLOCK(e,d+TJD_DATA,9U,&cursor,block,sizeof block,&n)==1&&n==0U);
 cursor=1U;n=777U;assert(TJDBLOCK(e,d+TJD_DATA,9U,&cursor,block,sizeof block,&n)==8&&cursor==1U&&n==777U);
 cursor=0U;assert(TJDBLOCK(e,d+TJD_DATA,9U,&cursor,block,31U,&n)==8&&cursor==0U);
 used=visits=0U;assert(!TJDOUT(e,vb,sizeof vb,&cost,visit,0)&&visits==2U&&cost==54U);
 assert(used==7U&&got[0]==0U&&got[1]==5U&&got[2]==0U&&got[3]==0xffU);
 memcpy(block,vb,sizeof vb);block[11]=1U;used=visits=0U;assert(TJDOUT(e,block,sizeof vb,&cost,visit,0)==8&&visits==0U);
 put(e+20U,1U);assert(TJDCHK(d,2U)==20);put(e+20U,32U);
 put(e+24U,TJD_BYTES);assert(TJDCHK(d,2U)==8);put(e+24U,TJD_DATA);
 put(e+28U,8U);assert(TJDCHK(d,2U)==8);put(e+28U,9U);
 d[TJD_BYTES-1U]=1U;assert(TJDCHK(d,2U)==8);d[TJD_BYTES-1U]=0U;
 /* Same name may be bound in another step, but never twice in one step. */
 memcpy(d+32U+TJD_ENTRY,e,TJD_ENTRY);put(d+8U,2U);put(d+32U+TJD_ENTRY+4U,TJD_DUMMY);
 put(d+32U+TJD_ENTRY+24U,0U);put(d+32U+TJD_ENTRY+28U,0U);assert(TJDCHK(d,2U)==8);
 put(d+32U+TJD_ENTRY,2U);assert(!TJDCHK(d,2U));
 /* Removing a DD must also clear its unused descriptor slot. */
 memcpy(block,d+32U+TJD_ENTRY,TJD_ENTRY);put(d+8U,1U);
 assert(TJDCHK(d,2U)==8);
 memset(d+32U+TJD_ENTRY,0,TJD_ENTRY);assert(!TJDCHK(d,2U));
 memcpy(d+32U+TJD_ENTRY,block,TJD_ENTRY);put(d+8U,2U);
 assert(!TJBINIT(&p,pool,sizeof pool));memset(job,0,sizeof job);put(job,1U);put(job+4U,sizeof job);ebname(job+8U,0xd1U);put(job+16U,2U);put(job+20U,4096U);
 for(i=0U;i<2U;++i){ebname(job+32U+i*TJB_STEP_BYTES,0xc1U+i);ebname(job+40U+i*TJB_STEP_BYTES,0xd7U);}
 assert(!TJBSUB(&p,7U,job,sizeof job,&id));assert(TJBBIND(&p,8U,id,d,sizeof d)==8);
 assert(!TJBBIND(&p,7U,id,d,sizeof d)&&TJBWORD(pool+4U)==4U);assert(TJBBIND(&p,7U,id,d,sizeof d)==16);
 put(pool+4U,1U);assert(TJBLOAD(&p,pool,sizeof pool)==8);put(pool+4U,4U);
 memcpy(saved,pool,sizeof pool);dd=TJBDDS(&p,id);dd[92U+32U]=1U;
 assert(TJBLOAD(&p,pool,sizeof pool)==8&&!memcmp(pool+TJB_BASE,saved+TJB_BASE,TJB_SLOT_BYTES));
 memcpy(pool,saved,sizeof pool);assert(!TJBLOAD(&p,pool,sizeof pool));assert(TJBWORD(TJBFIND(&p,7U,id)+8U)==TJB_HELD);
 assert(!TJBACT(&p,7U,id,2U)&&!TJBSTART(&p,7U,id));assert(TJBBIND(&p,7U,id,d,sizeof d)==16);
 assert(!TJBACT(&p,7U,id,3U)&&TJBNEXT(&p,7U,id)==1);assert(!TJBACT(&p,7U,id,4U));
 for(i=TJB_DD_BASE;i<TJB_BYTES;++i)assert(pool[i]==0U);
 /* FB blocking and binary order, including all 256 octet values. */
 e=entry(TJD_INLINE,TJD_FB,128U,256U);put(e+24U,TJD_DATA);put(e+28U,260U);
 d[TJD_DATA]=0U;d[TJD_DATA+1U]=128U;d[TJD_DATA+130U]=0U;d[TJD_DATA+131U]=128U;
 for(i=0U;i<128U;++i){d[TJD_DATA+2U+i]=(unsigned char)i;d[TJD_DATA+132U+i]=(unsigned char)(128U+i);}
 assert(!TJDCHK(d,2U));cursor=0U;assert(!TJDBLOCK(e,d+TJD_DATA,260U,&cursor,block,sizeof block,&n)&&n==256U);
 for(i=0U;i<256U;++i)assert(block[i]==(unsigned char)i);
 assert(!TJDOUT(e,block,n,&cost,0,0)&&cost==304U);assert(TJDOUT(e,block,n-1U,&cost,0,0)==8);
 put(e+28U,0U);memset(d+TJD_DATA,0,TJD_BYTES-TJD_DATA);assert(!TJDCHK(d,2U));cursor=0U;assert(TJDBLOCK(e,d+TJD_DATA,0U,&cursor,block,sizeof block,&n)==1);
 e=entry(TJD_SYSOUT,TJD_FB,133U,133U);assert(TJDCHK(d,2U)==8);
 e=entry(TJD_DUMMY,TJD_FB,80U,80U);assert(!TJDCHK(d,2U));
 /* Version-2 like concatenation: ordinal components, input only, exact
  * RECFM/LRECL but independent block size. Output/unlike are refused. */
 put(d,2U);memcpy(d+32U+TJD_ENTRY,e,TJD_ENTRY);put(d+8U,2U);
 put(d+32U+TJD_ENTRY+92U,1U);put(d+32U+TJD_ENTRY+20U,160U);assert(!TJDCHK(d,2U));
 put(d,1U);assert(TJDCHK(d,2U)==8);put(d,2U);
 put(d+32U+TJD_ENTRY+8U,TJD_OUTPUT);assert(TJDCHK(d,2U)==8);put(d+32U+TJD_ENTRY+8U,TJD_INPUT);
 put(d+32U+TJD_ENTRY+16U,40U);assert(TJDCHK(d,2U)==8);put(d+32U+TJD_ENTRY+16U,80U);
 put(d+32U+TJD_ENTRY+92U,2U);assert(TJDCHK(d,2U)==8);
 e=entry(TJD_TEMP,TJD_FB,4U,8U);put(d,2U);put(e+8U,TJD_OUTPUT);put(e+24U,64U);
 memset(e+40U,0x40,44U);e[40U]=0xe3U;put(e+92U,TJD_NEW<<16|TJD_KEEP<<8);assert(!TJDCHK(d,2U));
 put(e+24U,TJD_TEMP_LIMIT+1U);assert(TJDCHK(d,2U)==8);put(e+24U,64U);
 put(e+8U,TJD_INPUT);assert(TJDCHK(d,2U)==8);put(e+92U,TJD_OLD<<16|TJD_DELETE<<8);assert(!TJDCHK(d,2U));
 assert(!TJBSUB(&p,7U,job,sizeof job,&id)&&!TJBBIND(&p,7U,id,d,sizeof d)&&TJBWORD(pool+4U)==4U);
 assert(!TJBLOAD(&p,pool,sizeof pool));assert(!TJBACT(&p,7U,id,3U)&&!TJBACT(&p,7U,id,4U));
 e=entry(TJD_DUMMY,TJD_FB,80U,80U);assert(!TJBSUB(&p,7U,job,sizeof job,&id)&&!TJBBIND(&p,7U,id,d,sizeof d)&&TJBWORD(pool+4U)==4U);
 puts("batch DD descriptor, FB/VB, binary, empty, persistence and ownership controls passed");return 0;
}
