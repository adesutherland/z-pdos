/* SPDX-License-Identifier: MIT
 * Reusable U/C command dispatcher. Supplied services own I/O and native calls;
 * parsing/presentation own no privileged instructions or mutable shell state. */
#include "twospace_command_router.h"
#include "zpdos-version.h"
static unsigned char upper(unsigned char c)
{
    if(c>='a'&&c<='i')return (unsigned char)(c-'a'+'A');
    if(c>='j'&&c<='r')return (unsigned char)(c-'j'+'J');
    if(c>='s'&&c<='z')return (unsigned char)(c-'s'+'S');
    return c;
}
static unsigned int size(const char *p)
{unsigned int n=0U;while(p[n])++n;return n;}
static int equal(const unsigned char *p,unsigned int n,const char *s)
{unsigned int i;if(n!=size(s))return 0;for(i=0U;i<n;++i)if(upper(p[i])!=(unsigned char)s[i])return 0;return 1;}
static unsigned int skip(const unsigned char *p,unsigned int n,unsigned int at)
{while(at<n&&(p[at]==' '||p[at]=='\t'))++at;return at;}
static unsigned int token(const unsigned char *p,unsigned int n,unsigned int at)
{unsigned int end=at;while(end<n&&p[end]!=' '&&p[end]!='\t')++end;return end;}
static unsigned int put(TCMHOST host,const char *s)
{return host(TCM_WRITE,(unsigned char *)s,size(s));}
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static unsigned int clock_command(TCMHOST host)
{
    unsigned char raw[8],out[18];unsigned int hi,lo,i,r=0U,seconds=0U,bit,rc;
    const char *label="TIME 00:00:00 UTC";
    rc=host(TCM_CLOCK,raw,8U);if(rc)return rc;
    hi=word(raw)>>12;lo=(word(raw)<<20)|(word(raw+4U)>>12);
    /* Divide 64-bit microseconds since 1900 by 1e6, retaining seconds mod day.
     * This does not truncate the clock to 32-bit seconds. */
    for(i=0U;i<64U;++i){
        r=(r<<1)|((i<32U?hi:lo)>>(31U-i%32U)&1U);
        bit=r>=1000000U;if(bit)r-=1000000U;
        seconds=(seconds*2U+bit)%86400U;
    }
    for(i=0U;i<17U;++i)out[i]=(unsigned char)label[i];
    out[5]=(unsigned char)('0'+seconds/3600U/10U);out[6]=(unsigned char)('0'+seconds/3600U%10U);
    out[8]=(unsigned char)('0'+seconds/60U%60U/10U);out[9]=(unsigned char)('0'+seconds/60U%10U);
    out[11]=(unsigned char)('0'+seconds%60U/10U);out[12]=(unsigned char)('0'+seconds%10U);
    return host(TCM_WRITE,out,17U);
}
unsigned int TCMDISP(unsigned int env,const unsigned char *p,unsigned int n,
                     TCMHOST host,int *result)
{
    unsigned int at,end,i,j,rc;unsigned char id[64];
    if(!p||!host||!result||!n||n>TCM_MAX||env<TCM_CMS||env>TCM_PDOS)return 8U;
    for(i=0U;i<n;++i)if(!p[i]||p[i]=='\n'||p[i]=='\r')return 8U;
    at=skip(p,n,0U);end=token(p,n,at);*result=0;
    if(env==TCM_CMS&&equal(p+at,end-at,"QUERY")){
        at=skip(p,n,end);end=token(p,n,at);
        if(equal(p+at,end-at,"TIME")&&skip(p,n,end)==n)return clock_command(host);
        *result=24;return put(host,"CMS: unsupported QUERY operand");
    }
    if(env!=TCM_CMS&&equal(p+at,end-at,"TIME")&&skip(p,n,end)==n)return clock_command(host);
    if(env==TCM_CMS&&equal(p+at,end-at,"STATE")){
        at=skip(p,n,end);
        for(i=0U;i<18U;++i)id[i]=' ';
        for(i=0U;i<3U;++i){
            end=token(p,n,at);j=end-at;
            if(!j||j>(i==2U?2U:8U)){*result=24;return put(host,"CMS: STATE filename filetype filemode");}
            while(j){--j;id[8U*i+j]=upper(p[at+j]);}
            at=skip(p,n,end);
        }
        if(at!=n){*result=24;return put(host,"CMS: excessive STATE operands");}
        if(id[17]==' ')id[17]='1';
        rc=host(TCM_STAT_CMS,id,18U);
        if(rc==28U){*result=28;return put(host,"CMS: file not found");}
        if(rc)return rc;
        return put(host,"CMS: file exists");
    }
    if(env==TCM_TSO&&equal(p+at,end-at,"LISTDS")){
        at=skip(p,n,end);
        if(at==n||p[at]!='\''){*result=12;return put(host,"TSO: LISTDS requires a quoted full dataset name");}
        ++at;end=at;while(end<n&&p[end]!='\'')++end;
        if(end==n||end==at||end-at>44U||skip(p,n,end+1U)!=n){*result=12;return put(host,"TSO: invalid LISTDS operand");}
        for(i=0U;i<end-at;++i)id[i]=upper(p[at+i]);
        rc=host(TCM_STAT_TSO,id,end-at);
        if(rc==28U){*result=8;return put(host,"TSO: DATA SET NOT IN CATALOG");}
        if(rc)return rc;
        return put(host,"TSO: dataset exists");
    }
    if(env==TCM_PDOS){
        if(equal(p+at,end-at,"VERSION")&&skip(p,n,end)==n){
            rc=put(host,"z/PDOS " ZPDOS_VERSION "; PDIO1; PCOMM operator interface 1");
            return rc?rc:put(host,"Exact image build: see the host image receipt.");
        }
        if(equal(p+at,end-at,"ECHO")){
            at=end<n?end+1U:end;
            if(equal(p+at,n-at,"ON")||equal(p+at,n-at,"OFF"))return TCM_UNHANDLED;
            return host(TCM_WRITE,(unsigned char *)p+at,n-at);
        }
        if(equal(p+at,end-at,"DIR")||equal(p+at,end-at,"DEVICES")||
           equal(p+at,end-at,"VOLUMES")){
            if(n>198U)return 8U;
            rc=host(TCM_MEDIA,(unsigned char *)p,n);
            *result=(int)rc;return 0U;
        }
        return TCM_UNHANDLED;
    }
    *result=-3;return put(host,env==TCM_CMS?"CMS: unknown command":"TSO: unknown command");
}
/* Native resident entry receives a K-built U block: env,length,text.
 * Its C stack and arguments are private to this synchronous call. */
int TCMROUTE(unsigned int *block)
{
    int rc=0;unsigned int status,n=block[1];
    status=TCMDISP(block[0],(const unsigned char *)(block+2),block[1],TCMIO,&rc);
    if(status==TCM_UNHANDLED){
        block[0]=12U;block[1]=0xffffffffU;
        TCMIO(TCM_RUN,(unsigned char *)(block+2),n);
        status=block[0];rc=(int)block[1];
    }
    block[0]=status;block[1]=(unsigned int)rc;
    return (int)status;
}
