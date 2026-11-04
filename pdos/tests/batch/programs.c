/* SPDX-License-Identifier: MIT; actual Programs writer and full-field decoder. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "twospace_programs.h"
static TJFSTATE jobs;static TDVSTATE dev;
static unsigned int frame(const T27GEOMETRY *g,unsigned char *raw,unsigned int aid)
{raw[0]=(unsigned char)aid;assert(!T27ADDR(g,3U*g->columns+12U,raw+1U));return 3U;}
/* Independent visible-line oracle: unique record labels, real blank records,
 * and meaningful 132-column lines exercise every page seam and wrap. */
static void mixed_pages(const T27GEOMETRY *g,unsigned char *screen,unsigned int cap)
{
 unsigned char expected[100][130],saved[6500],*p;char label[40];
 unsigned int i,j,len,visible,count=0U,width=g->columns-2U,body=g->rows-5U,page,row,index,n,part,take;
 dev.output_bytes=0U;memset(expected,0x40,sizeof expected);
 for(i=0U;i<40U;++i){p=jobs.output+dev.output_bytes;TJBPUT(p+8U,132U);memset(p+24U,0x40,132U);
  if(i%5U==0U)visible=1U;
  else if(i%5U==2U){for(j=0U;j<132U;++j)p[24U+j]=(unsigned char)(0xc1U+i%9U);visible=132U;}
  else{sprintf(label,"Unique record %02u",i);len=(unsigned int)strlen(label);assert(!TJCENC(label,len,p+24U));visible=len;}
  for(part=0U;part<visible;part+=take){take=visible-part;if(take>width)take=width;assert(count<100U);memcpy(expected[count++],p+24U+part,take);}
  dev.output_bytes+=156U;
 }
 assert(dev.output_bytes<=sizeof saved);memcpy(saved,jobs.output,dev.output_bytes);
 assert(TDVPAGES(&dev,g)==(count+body-1U)/body);
 for(page=0U;page<TDVPAGES(&dev,g);++page){assert(!TDVPAINT(&dev,g,screen,cap,&n,TDV_VIEW_OUTPUT,page)&&!T27WRITE(g,screen,n));
  for(row=0U;row<body;++row){index=page*body+row;p=screen+4U+(row+2U)*(g->columns+2U)+2U;
   if(index<count)assert(!memcmp(p,expected[index],width));
   else for(j=0U;j<width;++j)assert(p[j]==0x40U);
  }
 }
 assert(!memcmp(saved,jobs.output,dev.output_bytes));dev.output_bytes=0U;
}
static void padded_output(const T27GEOMETRY *g,unsigned char *screen,unsigned int cap)
{
 unsigned char saved[780],expected[132];unsigned int j,n,width=g->columns-2U,stride=g->columns+2U;
 dev.output_bytes=0U;
 for(j=0U;j<5U;++j){unsigned char *p=jobs.output+dev.output_bytes;TJBPUT(p+8U,132U);memset(p+24U,0x40,132U);
  if(j==0U)assert(!TJCENC("FIRST",5U,p+24U));
  if(j==2U)assert(!TJCENC("SECOND",6U,p+24U));
  if(j==3U)memset(p+24U,0xc1,132U);
  if(j==4U)assert(!TJCENC("LAST",4U,p+24U));
  dev.output_bytes+=156U;
 }
 memcpy(saved,jobs.output,sizeof saved);assert(TDVPAGES(&dev,g)==1U);
 assert(!TDVPAINT(&dev,g,screen,cap,&n,1U,0U)&&!T27WRITE(g,screen,n));
 memset(expected,0x40,width);assert(!TJCENC("FIRST",5U,expected));assert(!memcmp(screen+4U+2U*stride+2U,expected,width));
 memset(expected,0x40,width);assert(!memcmp(screen+4U+3U*stride+2U,expected,width));
 assert(!TJCENC("SECOND",6U,expected));assert(!memcmp(screen+4U+4U*stride+2U,expected,width));
 memset(expected,0xc1,width);assert(!memcmp(screen+4U+5U*stride+2U,expected,width));
 memset(expected,0x40,width);memset(expected,0xc1,132U-width);assert(!memcmp(screen+4U+6U*stride+2U,expected,width));
 memset(expected,0x40,width);assert(!TJCENC("LAST",4U,expected));assert(!memcmp(screen+4U+7U*stride+2U,expected,width));
 assert(!memcmp(saved,jobs.output,sizeof saved));
 dev.output_bytes=0U;
 for(j=0U;j<30U;++j){unsigned char *p=jobs.output+dev.output_bytes;TJBPUT(p+8U,132U);memset(p+24U,0x40,132U);p[24U]=0xc2;dev.output_bytes+=156U;}
 assert(TDVPAGES(&dev,g)==(30U+g->rows-6U)/(g->rows-5U));
 assert(!TDVPAINT(&dev,g,screen,cap,&n,1U,TDVPAGES(&dev,g)-1U)&&!T27WRITE(g,screen,n));
 assert(screen[4U+2U*stride+2U]==0xc2U);
 dev.output_bytes=0U;
}
int main(void)
{
 unsigned char screen[40000],full[40000],raw[200];T27GEOMETRY g;unsigned int models[4][2]={{24U,80U},{32U,80U},{43U,80U},{27U,132U}},i,n,at,j,cell,aid,r,c;char command[199];
 dev.jobs=&jobs;strcpy(dev.notice,"Ready");
 for(i=0U;i<4U;++i){assert(!T27GEOM(&g,models[i][0],models[i][1],T27_CODED12));
  assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,0U,0U)&&!T27WRITE(&g,screen,n));
  at=frame(&g,full,0x60U);cell=0U;
  for(j=4U;j<n;++j){if(screen[j]==0x13U)continue;
   if(screen[j]==0x1dU){full[at++]=screen[j++];full[at++]=screen[j];++cell;}
   else{r=cell/g.columns;c=cell%g.columns;
    if(r==3U&&c>=12U&&c<17U)assert(!TJCENC(&"HELLO"[c-12U],1U,full+at));
    else if(r==5U&&c>=12U&&c<19U)assert(!TJCENC(&"one two"[c-12U],1U,full+at));
    else if(r==g.rows-3U&&c>=7U&&c<14U)assert(!TJCENC(&"DETAILS"[c-7U],1U,full+at));
    else full[at]=screen[j];++at;++cell;}}
  assert(cell==g.cells);n=frame(&g,raw,0xf5U);raw[n++]=0x11U;assert(!T27ADDR(&g,3U*g.columns+12U,raw+n));n+=2U;raw[n++]=0xc8U;
  raw[n++]=0x11U;assert(!T27ADDR(&g,5U*g.columns+12U,raw+n));n+=2U;raw[n++]=0x96U;
  assert(!TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)&&!strcmp(command,"RUN HELLO one two"));
  raw[0]=0xf6U;assert(!TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)&&!strcmp(command,"OUTPUT"));
  raw[0]=0x7dU;raw[n++]=0x11U;assert(!T27ADDR(&g,(g.rows-3U)*g.columns+7U,raw+n));n+=2U;raw[n++]=0xc4U;
  assert(!TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)&&!strcmp(command,"DETAILS"));
  assert(TDVPAGES(&dev,&g)==1U);dev.output_bytes=0U;
  for(j=0U;j<30U;++j){TJBPUT(jobs.output+dev.output_bytes+8U,132U);memset(jobs.output+dev.output_bytes+24U,0xc1,132U);dev.output_bytes+=156U;}
  assert(TDVPAGES(&dev,&g)==(60U+g.rows-6U)/(g.rows-5U));assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,1U,TDVPAGES(&dev,&g)-1U)&&!T27WRITE(&g,screen,n));dev.output_bytes=0U;
  assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,TDV_VIEW_HELP,0U)&&!T27WRITE(&g,screen,n));
  dev.detail_valid=1U;TJBPUT(dev.detail+16U,1U);TJBPUT(dev.detail+TJB_STEP_BASE+148U,TJB_NORMAL);
  dev.reviewed=0U;assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,TDV_VIEW_DETAILS,0U)&&!T27WRITE(&g,screen,n)&&!dev.reviewed);
  padded_output(&g,screen,sizeof screen);
  mixed_pages(&g,screen,sizeof screen);
  n=frame(&g,raw,0xf3U);assert(TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)==1);
 }
 return 0;
}
