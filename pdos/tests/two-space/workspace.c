/* SPDX-License-Identifier: MIT; actual shared frame and native busy rendering. */
#include <assert.h>
#include <string.h>
#include "twospace_workspace.h"
#include "twospace_display.h"
#include "twospace_job.h"
static unsigned char job[TJB_SLOT_BYTES],out[40000];
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void equal(const unsigned char *p,const char *text)
{unsigned char native[132];unsigned int n;assert(!TWDLabel(text,native,sizeof native,&n));assert(!memcmp(p,native,n));}
int main(void)
{
 unsigned int models[4][2]={{24U,80U},{32U,80U},{43U,80U},{27U,132U}},i,n,j,at,cells,attrs;
 unsigned char row[132],context[68],before[TJB_SLOT_BYTES];T27GEOMETRY g;
 memset(context,0xc1U,sizeof context);
 for(i=0U;i<4U;++i){assert(!T27GEOM(&g,models[i][0],models[i][1],T27_CODED12));
  assert(!TWPFRAME(&g,0U,TWP_SOURCE,context,8U,"UNSAVED","saved notice","PF3 Back",row));equal(row+1U,"z/PDOS / cREXX / Source");
  assert(!TWPFRAME(&g,1U,TWP_SOURCE,context,68U,"UNSAVED","","",row));
  if(g.columns==80U){assert(row[53U]==0x4bU&&row[54U]==0x4bU&&row[55U]==0x4bU);equal(row+59U,"UNSAVED");}
  assert(!TWPFRAME(&g,g.rows-3U,TWP_SOURCE,context,8U,"","","",row));equal(row+1U,"===> ");
  assert(!TWPFRAME(&g,g.rows-2U,TWP_SOURCE,context,8U,"","saved notice","",row));equal(row+1U,"saved notice");
  assert(!TWPFRAME(&g,g.rows-1U,TWP_SOURCE,context,8U,"","","PF3 Back",row));equal(row+1U,"PF3 Back");
  assert(!TWPFRAME(&g,2U,TWP_SOURCE,context,8U,"","","",row));for(j=0U;j<g.columns;++j)assert(row[j]==0x40U);
  assert(!TWPBUSY(&g,context,8U,TWP_BUILD,0,out,sizeof out,&n)&&!T27WRITE(&g,out,n));
  equal(out+4U+(g.columns+2U)+2U+11U,"Preparing");assert(out[0]==0xc1U);
  assert(!TWPBUSY(&g,context,8U,TWP_SOURCE,0,out,sizeof out,&n));
  equal(out+4U+3U*(g.columns+2U)+2U,"Please wait; the workspace will return automatically.");
  memset(job,0,sizeof job);put(job,42U);put(job+8U,TJB_RUNNING);put(job+16U,2U);
  memset(job+TJB_STEP_BASE,0x40U,16U);put(job+TJB_STEP_BASE+148U,TJB_NORMAL);put(job+TJB_STEP_BASE+128U,1U);put(job+TJB_STEP_BASE+132U,0xffffffffU);
  memset(job+TJB_STEP_BASE+TJB_STEP_BYTES,0x40U,16U);put(job+TJB_STEP_BASE+TJB_STEP_BYTES+148U,TJB_ACTIVE);
  memcpy(before,job,sizeof job);
  assert(!TWPBUSY(&g,context,8U,TWP_BUILD,job,out,sizeof out,&n)&&!T27WRITE(&g,out,n));
  assert(!memcmp(before,job,sizeof job));assert(!TWPJOBROW(&g,4U,job,row));equal(row+21U,"Completed");equal(row+34U,"0 / -1");
  assert(!TWPJOBROW(&g,5U,job,row));equal(row+21U,"Running");equal(row+34U,"pending");
  cells=attrs=0U;for(at=4U;at<n;++at){assert(out[at]!=0x13U);if(out[at]==0x1dU){assert(out[++at]==0xf0U);++attrs;}++cells;}
  assert(cells==g.cells&&attrs==2U*g.rows);
  put(job+8U,TJB_DONE);assert(!TWPBUSY(&g,context,8U,TWP_BUILD,job,out,sizeof out,&n));equal(out+4U+(g.columns+2U)+2U+11U,"Finalising");
  put(job+16U,TJB_STEPS+1U);assert(TWPBUSY(&g,context,8U,TWP_BUILD,job,out,sizeof out,&n)==8);
  assert(TWPBUSY(&g,context,8U,TWP_BUILD,0,out,3U,&n)==8);
  assert(TWPFRAME(&g,g.rows,TWP_HELP,context,8U,"","","",row)==8);
  context[0]=0U;assert(TWPFRAME(&g,0U,TWP_HELP,context,8U,"","","",row)==8);context[0]=0xc1U;
 }
 return 0;
}
