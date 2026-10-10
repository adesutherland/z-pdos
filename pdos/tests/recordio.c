/* SPDX-License-Identifier: MIT; complete service boundary with mock channel.
 * Real guest/disk checks remain separate. The tests verify refusal before
 * mutation, exact FB/VB frames, empty records and checked completion errors.
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_recordio.h"
#include "media_dscb.h"
typedef struct {unsigned int hi,lo;} TSPADDR;
typedef struct {unsigned int length,direction;TSPADDR address;} TSGREQUEST;
typedef struct {unsigned int token;} TSVFRAME;
#define TSG_READ 0U
#define TSG_WRITE 1U
static TSVFRAME frame={1U};static int native_invocations;
typedef struct {int device;char volser[7];} VOL;
typedef struct {int curdev,volume_count;VOL volumes[4];} MEDIA;
static MEDIA media_state={2,2,{{1,"PDOS00"},{2,"TEST01"},{0,""},{0,""}}};
static DSCB1 fixture;
static unsigned char header[96],payload[TRI_LIMIT],staging[TRI_LIMIT*2U];
static unsigned char disk[66][32768];static unsigned int sizes[66];
static unsigned int reads,writes,bad_span,fail_write,busy;
static unsigned int media_scratch_real;
static const TSVFRAME *TSVTOP(const int *v){(void)v;return &frame;}
static unsigned int cms_word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void cms_put_word(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int sf_half(const unsigned char *p){return ((unsigned int)p[0]<<8)|p[1];}
static void sf_put_half(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static unsigned int tc_span(TSPADDR address,unsigned int n,unsigned int direction,unsigned char *p,unsigned int copy)
{unsigned char *target;if(bad_span||address.hi)return 8U;
 target=address.lo==0x1000U?header:address.lo==0x10000U?payload:0;
 if(n&&(!target||n>(target==header?96U:TRI_LIMIT)))return 8U;
 if(copy&&n){if(direction==TSG_READ)memcpy(p,target,n);else memcpy(target,p,n);}return 0U;}
static int media_init(void){return 0;}
static int media_alpha(int c){return(c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static int media_digit(int c){return c>='0'&&c<='9';}
static int media_dscb(int d,const char *n,DSCB1 *out,int *c,int *h,int *r)
{(void)d;(void)n;*out=fixture;*c=1;*h=0;*r=3;return 1;}
static int media_busy(unsigned int d){(void)d;return (int)busy;}
static int pdosVolumeIndex(const MEDIA *m,int d){int i;for(i=0;i<m->volume_count;++i)if(m->volumes[i].device==d)return i;return -1;}
static void *media_malloc(unsigned int n){assert(n<=sizeof staging);media_scratch_real=1U;return staging;}
static void media_free(void *p){assert(p==staging);media_scratch_real=0U;}
static int media_cmp(const void *a,const void *b,unsigned int n){return memcmp(a,b,n);}
static void *media_copy(void *a,const void *b,unsigned int n){return memcpy(a,b,n);}
static int media_rdblock(int d,int c,int h,int r,void *out,int cap,int cmd)
{(void)d;(void)h;(void)cmd;++reads;if(c==1){memcpy(out,&fixture,140U);return 140;}
 if(r<1||r>65||sizes[r]>(unsigned int)cap)return -1;
 memcpy(out,disk[r],sizes[r]);return (int)sizes[r];}
static int media_wrblock(int d,int c,int h,int r,const void *in,int n,int cmd)
{const unsigned char *p=(const unsigned char *)in;(void)d;(void)c;(void)h;++writes;
 if(fail_write)return -1;if(cmd==0x05){memcpy((unsigned char *)&fixture+44U,in,96U);return n;}assert(r+1<66);
 sizes[r+1]=sf_half(p+6U);memcpy(disk[r+1],p+8U,sizes[r+1]);return n;}
/* The recipe changes only the two privileged high-register reads to zero;
 * framing, probing, destination admission and writes are the actual source. */
#include "recordio-under-test.inc"
static void init(unsigned int op,unsigned int n)
{memset(header,0,sizeof header);cms_put_word(header,1U);cms_put_word(header+4,96U);
 cms_put_word(header+8,op);cms_put_word(header+12,n);memcpy(header+40,"SOURCE",6U);
 cms_put_word(header+84,6U);cms_put_word(header+92,0x10000U);reads=writes=bad_span=fail_write=busy=0U;}
int main(void)
{
    TSGREQUEST request;unsigned int i;
    unsigned char records[]={0,3,'a',' ',' ',0,0,0,1,'z'};
    request.length=96U;request.direction=0U;request.address.hi=0U;request.address.lo=0x1000U;
    memset(&fixture,0,sizeof fixture);fixture.ds1dsorg=0x4000;fixture.ds1noepv=1;
    fixture.startcchh[1]=fixture.endcchh[1]=3;fixture.endcchh[3]=14;
    fixture.ds1recfm=0x50;fixture.ds1lrecl=8;fixture.ds1blkl=64;
    /* Low real register reads are replaced in the test copy by the recipe. */
    init(TRI_SAVE_EMPTY,sizeof records);memcpy(payload,records,sizeof records);
    assert(!record_file_service(&request));assert(writes==3U);
    assert(sf_half(disk[1])==20U&&sf_half(disk[1]+4)==7U&&sf_half(disk[1]+11)==4U);
    init(TRI_LOAD,TRI_LIMIT);busy=1U;assert(!record_file_service(&request));
    assert(cms_word(header+16)==sizeof records&&!memcmp(payload,records,sizeof records));
    init(TRI_SAVE_EMPTY,sizeof records);busy=1U;
    assert(record_file_service(&request)==16U);assert(!reads&&!writes);
    init(TRI_SAVE_EMPTY,sizeof records);assert(record_file_service(&request)==16U);assert(!writes);
    init(TRI_SAVE_EMPTY,1U);assert(record_file_service(&request));assert(!writes&&!reads);
    init(TRI_LOAD,TRI_LIMIT);bad_span=1;assert(record_file_service(&request));assert(!reads&&!writes);
    init(TRI_LOAD,TRI_LIMIT);header[0]=2;assert(record_file_service(&request)==20U);assert(!reads);
    sizes[1]=0;init(TRI_SAVE_EMPTY,sizeof records);memcpy(payload,records,sizeof records);fail_write=1;
    assert(record_file_service(&request)==12U);assert(writes==1U);
    init(TRI_SAVE_EMPTY,sizeof records);media_state.curdev=1;busy=1U;
    assert(record_file_service(&request)==20U);assert(!writes);media_state.curdev=2;
    init(TRI_SAVE_EMPTY,64U*258U);fixture.ds1lrecl=260;fixture.ds1blkl=264;
    for(i=0U;i<64U;++i){sf_put_half(payload+i*258U,256U);memset(payload+i*258U+2U,'x',256U);}
    assert(record_file_service(&request)==20U);assert(!writes&&!reads);
    init(TRI_LOAD,TRI_LIMIT);fixture.ds1recfm=(char)0xc0;
    assert(record_file_service(&request)==20U);assert(!writes&&!reads);
    fixture.ds1recfm=0x50;fixture.ds1lrecl=8;fixture.ds1blkl=64;
    init(TRI_SAVE_EMPTY,sizeof records);cms_put_word(header+36,1U);memcpy(header+20,"ABSENT",6U);
    assert(record_file_service(&request)==4U);assert(!writes&&!reads);
    init(TRI_LOAD,12U);cms_put_word(header+92,0x1000U);
    assert(record_file_service(&request)==8U);assert(!reads&&!writes);
    init(TRI_LOAD,TRI_LIMIT);fixture.ds1recfm=0x80;
    assert(record_file_service(&request)==20U);assert(!reads&&!writes);
    /* Exact FB padding is part of every logical record, including blanks. */
    fixture.ds1recfm=(char)0x90;fixture.ds1lrecl=4;fixture.ds1blkl=8;sizes[1]=0;
    init(TRI_SAVE_EMPTY,12U);memcpy(payload,"\0\4ab  \0\4    ",12U);
    assert(!record_file_service(&request));assert(writes==3U);
    init(TRI_LOAD,TRI_LIMIT);assert(!record_file_service(&request));
    assert(cms_word(header+16)==12U&&!memcmp(payload,"\0\4ab  \0\4    ",12U));
    /* A valid >50KB track must remain loadable after a successful save. */
    fixture.ds1lrecl=26000;fixture.ds1blkl=26000;sizes[1]=0;
    init(TRI_SAVE_EMPTY,52004U);
    for(i=0U;i<2U;++i){sf_put_half(payload+i*26002U,26000U);memset(payload+i*26002U+2U,'x',26000U);}
    assert(!record_file_service(&request));assert(writes==4U);
    init(TRI_LOAD,TRI_LIMIT);assert(!record_file_service(&request));
    assert(cms_word(header+16)==52004U&&payload[52003U]=='x');
    return 0;
}
