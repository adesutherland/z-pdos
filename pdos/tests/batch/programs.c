/* SPDX-License-Identifier: MIT; actual Programs writer and full-field decoder. */
#include <assert.h>
#include <string.h>
#include "twospace_programs.h"
static TJFSTATE jobs;static TDVSTATE dev;
static unsigned int frame(const T27GEOMETRY *g,unsigned char *raw,unsigned int aid)
{raw[0]=(unsigned char)aid;assert(!T27ADDR(g,3U*g->columns+12U,raw+1U));return 3U;}
int main(void)
{
 unsigned char screen[40000],full[40000],raw[200];T27GEOMETRY g;unsigned int models[2][2]={{24U,80U},{27U,132U}},i,n,at,j,cell,aid,r,c;char command[199];
 dev.jobs=&jobs;strcpy(dev.notice,"Ready");
 for(i=0U;i<2U;++i){assert(!T27GEOM(&g,models[i][0],models[i][1],T27_CODED12));
  assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,0U,0U)&&!T27WRITE(&g,screen,n));
  at=frame(&g,full,0x60U);cell=0U;
  for(j=4U;j<n;++j){if(screen[j]==0x13U)continue;
   if(screen[j]==0x1dU){full[at++]=screen[j++];full[at++]=screen[j];++cell;}
   else{r=cell/g.columns;c=cell%g.columns;
    if(r==3U&&c>=12U&&c<17U)assert(!TJCENC(&"HELLO"[c-12U],1U,full+at));
    else if(r==5U&&c>=12U&&c<19U)assert(!TJCENC(&"one two"[c-12U],1U,full+at));
    else full[at]=screen[j];++at;++cell;}}
  assert(cell==g.cells);n=frame(&g,raw,0xf5U);raw[n++]=0x11U;assert(!T27ADDR(&g,3U*g.columns+12U,raw+n));n+=2U;raw[n++]=0xc8U;
  raw[n++]=0x11U;assert(!T27ADDR(&g,5U*g.columns+12U,raw+n));n+=2U;raw[n++]=0x96U;
  assert(!TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)&&!strcmp(command,"RUN HELLO one two"));
  raw[0]=0xf6U;assert(!TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)&&!strcmp(command,"OUTPUT"));
  assert(TDVPAGES(&dev,&g)==1U);dev.output_bytes=0U;
  for(j=0U;j<30U;++j){TJBPUT(jobs.output+dev.output_bytes+8U,132U);memset(jobs.output+dev.output_bytes+24U,0xc1,132U);dev.output_bytes+=156U;}
  assert(TDVPAGES(&dev,&g)==3U);assert(!TDVPAINT(&dev,&g,screen,sizeof screen,&n,1U,TDVPAGES(&dev,&g)-1U)&&!T27WRITE(&g,screen,n));dev.output_bytes=0U;
  n=frame(&g,raw,0xf3U);assert(TDVINPUT(&dev,&g,raw,n,full,at,command,&aid)==1);
 }
 return 0;
}
