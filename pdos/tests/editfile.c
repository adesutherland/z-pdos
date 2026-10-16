/* SPDX-License-Identifier: MIT; actual checked gate + actual two-bank store.
 * Privileged register/boot-device reads alone are adapted by the recipe. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_editfile.h"
#include "twospace_store.h"
#include "twospace_dataset.h"
#include "twospace_jobdd.h"
typedef struct {unsigned int hi,lo;} TSPADDR;
typedef struct {unsigned int length;TSPADDR address;} TSGREQUEST;
typedef struct {unsigned int token;} TSVFRAME;
typedef struct {unsigned int device,bytes;unsigned char dsn[44];TSKEXTENT extent;} SFDD;
typedef struct {unsigned int token,dd,member,cylinder,head,record;unsigned char key[52];} SFFILE;
#define TSG_READ 0U
#define TSG_WRITE 1U
static int native_invocations,channel;
static TSVFRAME frame={1U};static SFDD sf_dd[1];static SFFILE sf_file[1];
typedef struct {unsigned int device;unsigned char volser[6];} VOL;
static struct {int curdev,volume_count;VOL volumes[1];} media_state={1,1,{{1,{0xd7,0xc4,0xd6,0xe2,0xf0,0xf0}}}};
static unsigned char header[TEF_BYTES],payload[TEF_LIMIT],physical[1024];
static unsigned char *disk;static TSSSTATE native_store;
static unsigned int media_scratch_real,bad_span,busy,dataset=1U,member=1U,physical_n,io_calls,fail_at,writes,reads;
static TEFATTR attr={0x90U,4U,8U};
static const TSVFRAME *TSVTOP(const int *v){(void)v;return frame.token?&frame:0;}
static unsigned int cms_word(const unsigned char *p){return TJDWORD(p);}
static void cms_put_word(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int sf_half(const unsigned char *p){return ((unsigned int)p[0]<<8)|p[1];}
static void sf_put_half(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static int record_before(TSPADDR a,TSPADDR b){return a.hi<b.hi||(a.hi==b.hi&&a.lo<b.lo);}
static unsigned int tc_span(TSPADDR a,unsigned int n,unsigned int dir,unsigned char *p,unsigned int copy)
{unsigned char *t;if(bad_span||a.hi)return 8U;t=a.lo==0x1000U?header:a.lo==0x10000U?payload:0;
 if(n&&(!t||n>(t==header?TEF_BYTES:TEF_LIMIT)))return 8U;
 if(copy&&n){if(dir==TSG_READ)memcpy(p,t,n);else memcpy(t,p,n);}return 0U;}
static int media_init(void){return 0;}
static void *media_set(void *p,int c,unsigned int n){return memset(p,c,n);}
static void *media_copy(void *p,const void *q,unsigned int n){return memcpy(p,q,n);}
static int media_cmp(const void *a,const void *b,unsigned int n){return memcmp(a,b,n);}
static void *media_malloc(unsigned int n){void *p=malloc(n);if(p)media_scratch_real=1U;return p;}
static void media_free(void *p){free(p);media_scratch_real=0U;}
static unsigned int job_active(void){return busy;}
static int sf_volume_record(void *c,unsigned int a,unsigned int b,unsigned int d,unsigned int e,const unsigned char **p)
{(void)c;(void)a;(void)b;(void)d;(void)e;(void)p;return 0;}
/* Stub VTOC/physical read only; journal publication is the real store. */
int TSKDSCB(TSKREAD r,void *c,const unsigned char *name,unsigned int n,TSKEXTENT *e,unsigned char data[96])
{(void)r;(void)c;(void)name;(void)n;memset(data,0,96U);if(!dataset)return TSK_ABSENT;
 memset(e,0,sizeof *e);e->start_cylinder=3U;e->end_cylinder=3U;e->end_head=14U;e->organisation=dataset==1U?0x0200U:0x4000U;
 e->record_format=attr.format;e->logical_length=attr.lrecl;e->block_length=attr.blksize;return 0;}
static int sf_member(SFDD *d,const unsigned char *m,unsigned char *ttr){(void)d;(void)m;ttr[0]=ttr[1]=0;ttr[2]=1U;return (int)member;}
static int sf_disk_record(SFDD *d,SFFILE *f,unsigned int cap,unsigned int *got)
{(void)d;(void)cap;++reads;if(f->record++)return 1;*got=physical_n;return physical_n?0:1;}
static unsigned char *TSCDATA(const int *c){(void)c;return physical;}
static unsigned int sf_store_profile(const SFDD *d){(void)d;return 0x54534fU;}
static int store_io(void *c,unsigned int record,unsigned int write,unsigned char *block)
{(void)c;assert(record>=TSS_FIRST_BLOCK&&record<TSS_RECORDS);++io_calls;
 if(io_calls==fail_at)return -1;
 if(write){memcpy(disk+(record-TSS_FIRST_BLOCK)*TSS_BLOCK,block,TSS_BLOCK);++writes;}
 else memcpy(block,disk+(record-TSS_FIRST_BLOCK)*TSS_BLOCK,TSS_BLOCK);return 0;}
static int store_attach(void){return 0;}
#include "editfile-under-test.inc"
static void init(unsigned int action,unsigned int cap,const unsigned char *token)
{static const unsigned char dsn[]={0xe2,0xd6,0xe4,0xd9,0xc3,0xc5};
 memset(header,0,sizeof header);memset(header+40U,0x40U,58U);memcpy(header+40U,dsn,sizeof dsn);header[84U]=0xc1U;
 cms_put_word(header,1U);cms_put_word(header+4U,TEF_BYTES);cms_put_word(header+8U,action);cms_put_word(header+12U,cap);cms_put_word(header+140U,0x10000U);
 if(action!=TEF_LOAD){cms_put_word(header+20U,attr.format);cms_put_word(header+24U,attr.lrecl);cms_put_word(header+28U,attr.blksize);}
 if(token)memcpy(header+104U,token,32U);bad_span=busy=reads=writes=io_calls=fail_at=0U;}
int main(void)
{
 TSGREQUEST r;unsigned char token[32],newtoken[32],original[6]={0,4,0xc1,0x40,0x40,0x40};
 unsigned char changed[6]={0,4,0xc2,0x40,0x40,0x40},out[TEF_LIMIT],key[52];unsigned int n,count,calls,i,successful;
 disk=(unsigned char *)calloc(TSS_RECORDS-TSS_FIRST_BLOCK,TSS_BLOCK);assert(disk&&!TSSINIT(&native_store,store_io,0));
 r.length=TEF_BYTES;r.address.hi=0;r.address.lo=0x1000U;
 memcpy(physical,original+2U,4U);physical_n=4U;
 init(TEF_LOAD,TEF_LIMIT,0);assert(!edit_file_service(&r));assert(cms_word(header+16U)==6U&&!memcmp(payload,original,6U));memcpy(token,header+104U,32U);
 init(TEF_SAVE,6U,token);memcpy(payload,original,6U);assert(!edit_file_service(&r)&&!writes&&!(cms_word(header+36U)&TEF_JOURNAL));
 init(TEF_SAVE,6U,token);memcpy(payload,original,6U);attr.blksize=16U;assert(edit_file_service(&r)==16U&&!reads&&!writes);attr.blksize=8U;
 init(TEF_SAVE,6U,token);memcpy(payload,changed,6U);assert(!edit_file_service(&r)&&writes);memcpy(newtoken,header+104U,32U);
 init(TEF_LOAD,TEF_LIMIT,0);assert(!edit_file_service(&r)&&!reads&&!memcmp(payload,changed,6U));
 init(TEF_SAVE,6U,token);memcpy(payload,original,6U);assert(edit_file_service(&r)==16U&&!writes);
 init(TEF_SAVE,6U,token);memcpy(payload,changed,6U);assert(!edit_file_service(&r)&&!writes); /* identical-content retry */
 init(TEF_NEW,6U,newtoken);assert(edit_file_service(&r)==16U&&!writes);
 init(TEF_SAVE,6U,newtoken);bad_span=1;assert(edit_file_service(&r)==8U&&!reads&&!writes);
 init(TEF_SAVE,6U,newtoken);busy=1;assert(edit_file_service(&r)==16U&&!reads&&!writes);
 init(TEF_SAVE,6U,newtoken);cms_put_word(header+140U,0x1000U);assert(edit_file_service(&r)==8U&&!reads&&!writes);
 init(TEF_LOAD,0U,0);header[100U]=1U;assert(edit_file_service(&r)==8U&&!reads&&!writes);
 init(TEF_LOAD,0U,0);header[40U]=0x40U;assert(edit_file_service(&r)==8U&&!reads&&!writes);
 init(TEF_LOAD,0U,0);media_state.curdev=2;assert(edit_file_service(&r)==20U&&!reads&&!writes);media_state.curdev=1;
 init(TEF_SAVE,1U,newtoken);assert(edit_file_service(&r)==8U&&!writes);
 /* A new member in an existing PDS is an empty buffer, then journal-backed. */
 init(TEF_LOAD,TEF_LIMIT,0);header[84U]=0xc2U;member=0U;assert(!edit_file_service(&r)&&!cms_word(header+16U)&&!(cms_word(header+36U)&TEF_EXISTS));memcpy(token,header+104U,32U);
 init(TEF_SAVE,6U,token);header[84U]=0xc2U;memcpy(payload,changed,6U);assert(!edit_file_service(&r));
 init(TEF_LOAD,TEF_LIMIT,0);header[84U]=0xc2U;assert(!edit_file_service(&r)&&!memcmp(payload,changed,6U));member=1U;
 /* Every failed write/readback stage retains an authoritative old or complete
  * new value. Restart plus identical-content retry always converges. */
 init(TEF_SAVE,6U,newtoken);memcpy(payload,original,6U);assert(!edit_file_service(&r));successful=io_calls;
 for(i=1U;i<=successful;++i){
  init(TEF_LOAD,TEF_LIMIT,0);assert(!edit_file_service(&r));memcpy(token,header+104U,32U);
  init(TEF_SAVE,6U,token);memcpy(payload,(i&1U)?changed:original,6U);fail_at=i;
  calls=edit_file_service(&r);assert(!calls||calls==12U);fail_at=0U;
  assert(!TSSINIT(&native_store,store_io,0));
  init(TEF_LOAD,TEF_LIMIT,0);assert(!edit_file_service(&r));assert(!memcmp(payload,original,6U)||!memcmp(payload,changed,6U));
  init(TEF_SAVE,6U,token);memcpy(payload,(i&1U)?changed:original,6U);assert(!edit_file_service(&r));
 }
 /* Slot exhaustion admits no write and cannot overwrite another key. */
 memset(key,0x40U,sizeof key);key[0]=0xc6U;
 for(i=0U;i<TSS_SLOTS-2U;++i){key[1]=(unsigned char)(0xf0U+i);assert(!TSSPUT(&native_store,0x54534fU,key,52U,physical,4U));}
 init(TEF_LOAD,TEF_LIMIT,0);header[84U]=0xc3U;member=0U;assert(!edit_file_service(&r));memcpy(token,header+104U,32U);
 init(TEF_SAVE,6U,token);header[84U]=0xc3U;memcpy(payload,changed,6U);assert(edit_file_service(&r)==4U&&!writes);member=1U;
 /* Full native-byte boundary and record-count limits. */
 attr.format=0x90U;attr.lrecl=attr.blksize=256U;
 for(i=0U;i<254U;++i){sf_put_half(payload+i*258U,256U);memset(payload+i*258U+2U,0xc1U,256U);}
 assert(!TEFPACK(&attr,payload,65532U,out,sizeof out,&n,&count)&&n==65532U&&count==254U);
 assert(!TEFUNPK(&attr,out,n,payload,sizeof payload,&n,&count)&&n==65532U);
 assert(TEFPACK(&attr,payload,65533U,out,sizeof out,&n,&count));
 attr.format=0x50U;attr.lrecl=4U;attr.blksize=264U;memset(payload,0,8194U);
 assert(!TEFPACK(&attr,payload,8192U,out,sizeof out,&n,&count)&&count==4096U);
 assert(TEFPACK(&attr,payload,8194U,out,sizeof out,&n,&count));
 /* Native pack/unpack preserves VB empty records and FB exact spaces. */
 attr.format=0x50U;attr.lrecl=260U;attr.blksize=264U;
 memcpy(payload,"\0\0\0\3\201\100\100",7U);
 assert(!TEFPACK(&attr,payload,7U,out,sizeof out,&n,&count)&&count==2U);
 assert(!TEFUNPK(&attr,out,n,payload,sizeof payload,&n,&count)&&n==7U&&count==2U);
 out[4U]=1U;assert(TEFUNPK(&attr,out,17U,payload,sizeof payload,&n,&count));
 free(disk);return 0;
}
