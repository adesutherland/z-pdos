/* SPDX-License-Identifier: MIT; pure shared C90 frame, no execution policy. */
#include "twospace_workspace.h"
#include "twospace_display.h"
#include "twospace_job.h"
static unsigned int word(const unsigned char *p)
{return (unsigned int)p[0]*16777216U+(unsigned int)p[1]*65536U+(unsigned int)p[2]*256U+p[3];}
static int geometry(const T27GEOMETRY *g)
{return g&&g->rows>=24U&&g->rows<=43U&&g->columns>=80U&&g->columns<=132U&&g->cells==g->rows*g->columns;}
static unsigned int text(unsigned char *p,unsigned int cap,const char *s)
{
 char clipped[133];unsigned int n=0U,used;
 if(!s)return 0U;while(n<cap&&s[n]){clipped[n]=s[n];++n;}clipped[n]=0;
 if(TWDLabel(clipped,p,cap,&used))return 0U;return used;
}
static unsigned int number(unsigned char *p,unsigned int cap,unsigned int value)
{
 unsigned char digits[10];unsigned int n=0U,i;
 do{digits[n++]=(unsigned char)(0xf0U+value%10U);value/=10U;}while(value);
 if(n>cap)return 0U;for(i=0U;i<n;++i)p[i]=digits[n-i-1U];return n;
}
int TWPFRAME(const T27GEOMETRY *g,unsigned int r,const char *panel,
 const unsigned char *context,unsigned int context_bytes,const char *status,
 const char *notice,const char *keys,unsigned char *row)
{
 unsigned int c,n=1U,width,visible;
 if(!geometry(g)||r>=g->rows||!row||!panel||context_bytes>TWP_CONTEXT_MAX||(context_bytes&&!context))return 8;
 for(c=0U;c<context_bytes;++c)if(context[c]<0x40U||context[c]==0xffU)return 8;
 for(c=0U;c<g->columns;++c)row[c]=0x40U;width=g->columns-2U;
 if(!r){n+=text(row+n,width,"z/PDOS / cREXX / ");text(row+n,g->columns-1U-n,panel);}
 else if(r==1U){visible=context_bytes;if(status&&*status&&visible>width-23U)visible=width-23U;
  for(c=0U;c<visible;++c)row[n++]=context[c];
  if(visible<context_bytes&&visible>=3U)row[n-1U]=row[n-2U]=row[n-3U]=0x4bU;
  if(context_bytes&&status&&*status)n+=text(row+n,g->columns-1U-n," / ");
  text(row+n,g->columns-1U-n,status);
 }else if(r==g->rows-3U)text(row+1U,5U,"===> ");
 else if(r==g->rows-2U)text(row+1U,width,notice);
 else if(r==g->rows-1U)text(row+1U,width,keys);
 return 0;
}
static const char *stepstate(unsigned int state)
{
 return state==TJB_PENDING?"Pending":state==TJB_ACTIVE?"Running":state==TJB_NORMAL?"Completed":state==TJB_SKIP?"Skipped":state==TJB_ERROR?"Failed":state==TJB_CANCEL?"Cancelled":state==TJB_INTERRUPT?"Interrupted":"Unknown";
}
int TWPJOBROW(const T27GEOMETRY *g,unsigned int r,const unsigned char *job,unsigned char *row)
{
 const unsigned char *step;unsigned int c,n,count,value;
 if(!geometry(g)||!row||r<2U||r>=g->rows-3U)return 8;
 count=job?word(job+16U):0U;if(count>TJB_STEPS)return 8;
 for(c=0U;c<g->columns;++c)row[c]=0x40U;
 if(r==2U&&job){n=1U;n+=text(row+n,20U,"Job ");n+=number(row+n,g->columns-1U-n,word(job));n+=text(row+n,g->columns-1U-n," / native state ");number(row+n,g->columns-1U-n,word(job+8U));}
 else if(r==3U)text(row+1U,g->columns-2U,"Step      Program   State       OS / RC");
 else if(r>=4U&&r<4U+count){step=job+TJB_STEP_BASE+(r-4U)*TJB_STEP_BYTES;
  for(c=0U;c<8U;++c){row[1U+c]=step[c]<0x40U||step[c]==0xffU?0x6fU:step[c];row[11U+c]=step[8U+c]<0x40U||step[8U+c]==0xffU?0x6fU:step[8U+c];}
  value=word(step+148U);text(row+21U,12U,stepstate(value));n=34U;
  if(value==TJB_NORMAL||value>=TJB_ERROR){n+=number(row+n,g->columns-1U-n,word(step+124U));n+=text(row+n,g->columns-1U-n," / ");
   if(word(step+128U)){value=word(step+132U);if(value&0x80000000U){n+=text(row+n,1U,"-");value=(~value)+1U;}number(row+n,g->columns-1U-n,value);}
   else text(row+n,g->columns-1U-n,"unavailable");
  }else text(row+n,g->columns-1U-n,value==TJB_SKIP?"not run":"pending");
 }
 return 0;
}
#ifdef TWP_BUSY
int TWPBUSY(const T27GEOMETRY *g,const unsigned char *context,unsigned int context_bytes,
 const char *task,const unsigned char *job,unsigned char *out,unsigned int cap,unsigned int *used)
{
 unsigned char row[132];unsigned int at=0U,r,c,count,state;
 const char *status;
 if(!geometry(g)||!out||!used||!task||cap<g->cells+2U*g->rows+4U)return 8;
 count=job?word(job+16U):0U;if(count>TJB_STEPS)return 8;
 state=job?word(job+8U):0U;
 status=!job?"Preparing":state>=TJB_DONE?"Finalising":state==TJB_RUNNING?"Running":state==TJB_HELD?"Held":"Starting";
 /* No restore-keyboard WCC bit. Every field is protected. */
 out[at++]=0xc1U;out[at++]=0x11U;if(T27ADDR(g,0U,out+at))return 8;at+=2U;
 for(r=0U;r<g->rows;++r){
  if(TWPFRAME(g,r,task,context,context_bytes,status,"Input resumes when the action returns.","",row))return 8;
  if(r>=2U&&r<g->rows-3U){
   if(job){if(TWPJOBROW(g,r,job,row))return 8;
    if(r==14U)text(row+1U,g->columns-2U,"Execution is synchronous; displayed steps come from the native job.");
   }else if(r==3U)text(row+1U,g->columns-2U,"Please wait; the workspace will return automatically.");
  }
  else if(r==g->rows-3U)for(c=1U;c<g->columns-1U;++c)row[c]=0x40U;
  for(c=0U;c<g->columns;++c){if(!c||c==g->columns-1U){out[at++]=0x1dU;out[at++]=0xf0U;}else out[at++]=row[c];}
 }
 *used=at;return 0;
}
#endif
