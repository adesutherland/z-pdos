/* SPDX-License-Identifier: MIT; common deck admission and fixed expectations.
 * Catalog callbacks model the explicitly selected fixture, never execution. */
#include <stdio.h>
#include <string.h>
#include "twospace_jcl.h"
static TJCWORK work;
static TJCRESULT result;
static unsigned char deck[40960],proc[40960];
static const char *root;
static unsigned int read_file(const char *name,unsigned char *out,unsigned int cap)
{FILE *f;char path[256];size_t n;int extra;
 if(strlen(root)+strlen(name)+2U>=sizeof path)return 0U;
 strcpy(path,root);strcat(path,"/");strcat(path,name);f=fopen(path,"rb");if(!f)return 0U;
 n=fread(out,1U,cap,f);extra=fgetc(f);if(ferror(f)||extra!=EOF)n=0U;fclose(f);return (unsigned int)n;}
static int procedure(void *ctx,const char *name,const TJCLIB *libs,unsigned int count,const unsigned char **p,unsigned int *n)
{char file[32];(void)ctx;if(count!=1U||strcmp(libs[0].dsn,"LABA01.B33.PROCLIB"))return 4;
 strcpy(file,name);strcat(file,".fb80");*n=read_file(file,proc,sizeof proc);*p=proc;return *n?0:4;}
static int program(void *ctx,const TJCSTEP *s,unsigned char out[8])
{char alias[9];(void)ctx;if(s->library_count!=1U||strcmp(s->libraries[0].dsn,"LABA01.B33.LOAD"))return 4;
 if(!strcmp(s->program,"RXC"))strcpy(alias,"BRXC");else if(!strcmp(s->program,"RXAS"))strcpy(alias,"BRXAS");
 else if(!strcmp(s->program,"RXLINK"))strcpy(alias,"BRXLINK");else if(!strcmp(s->program,"RXVM"))strcpy(alias,"BRXVM");
 else if(!strcmp(s->program,"BTEST31"))strcpy(alias,"BTEST31");else return 4;
 memset(out,0x40,8U);return TJCENC(alias,(unsigned int)strlen(alias),out);}
static int allocation(void *ctx,const TJCSTEP *s,const TJCDECL *d,TJCATTR *a)
{(void)ctx;
 a->direction=!strcmp(d->name,"SYSPRINT")||!strcmp(d->name,"SYSTERM")||!strcmp(d->name,"RESULT")||
  (!strcmp(s->program,"RXC")&&!strcmp(d->name,"ASM"))||(!strcmp(s->program,"RXAS")&&!strcmp(d->name,"SYSGO"))||
  (!strcmp(s->program,"RXLINK")&&!strcmp(d->name,"RXBIN"))?TJD_OUTPUT:TJD_INPUT;
 if(d->kind==TJD_DATASET){a->recfm=TJD_VB;a->lrecl=260U;a->blksize=!strcmp(d->name,"LIBRARY")?2640U:264U;
  if(!strcmp(d->name,"SRC")||!strcmp(d->name,"SYSIN")){a->recfm=TJD_FB;a->lrecl=80U;a->blksize=3120U;}
  if(!strcmp(d->name,"BYTES")){a->recfm=TJD_FB;a->lrecl=a->blksize=256U;}}
 else{if(!a->recfm)a->recfm=TJD_FB;if(!a->lrecl)a->lrecl=d->kind==TJD_SYSOUT?132U:80U;
  if(!a->blksize)a->blksize=a->recfm==TJD_VB?a->lrecl+4U:a->lrecl;}
 return 0;}
int main(int argc,char **argv)
{static const char *names[]={"C","CG","CL","LGO","LGODEF","BAD","RC8","COND","PARSE","INLINE","CPROBE","SPINBLD","SPIN","NOPROC","NOLIB","WORK7","WORK8","QUOTA"};
 static const unsigned int steps[]={2U,3U,3U,4U,4U,3U,3U,4U,3U,3U,1U,2U,1U,0U,3U,3U,0U,0U};
 unsigned int i,n;TJCSITE site;TJCERROR error;char file[40];int rc;
 if(argc!=2)return 2;root=argv[1];memset(&site,0,sizeof site);site.program=program;site.dd=allocation;site.procedure=procedure;
 for(i=0U;i<sizeof names/sizeof names[0];++i){strcpy(file,names[i]);strcat(file,".fb80");n=read_file(file,deck,sizeof deck);if(!n)return 2;
  rc=TJCCONV(&work,deck,n,&site,&result,&error);
  /* NOLIB passes syntax here: live U site admission rejects its missing prefix. */
  if((steps[i]&&rc)||(!steps[i]&&!rc)||(steps[i]&&result.steps!=steps[i])){
   printf("CASE %s rc=%d card=%u column=%u reason=%u steps=%u\n",names[i],rc,error.card,error.column,error.reason,result.steps);return 1;}
 }
 puts("PD033 common qualification deck admission/limits PASS");return 0;}
