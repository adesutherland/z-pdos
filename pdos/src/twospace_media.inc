/* SPDX-License-Identifier: MIT
 * K port of the maintained PCOMM media commands. All channel addresses and
 * command data belong to K; no one-space helper receives an unchecked U pointer.
 */
#include <stdarg.h>
#include "media_dscb.h"
#define PDOS_MAX_VOLUMES 4
typedef struct { int device,address;char volser[7]; } KMEDIA_VOLUME;
typedef struct {
    int curdev,volume_count,tape_device,tape_address,tape_writable;
    KMEDIA_VOLUME volumes[PDOS_MAX_VOLUMES];
} KMEDIA;
static KMEDIA media_state;
static unsigned int media_scratch_real;
static int media_busy(unsigned int device);

static unsigned int media_len(const char *p)
{unsigned int n=0U;while(p[n])++n;return n;}
static void *media_copy(void *to,const void *from,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)((unsigned char *)to)[i]=((const unsigned char *)from)[i];return to;}
static void *media_set(void *to,int value,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)((unsigned char *)to)[i]=(unsigned char)value;return to;}
static int media_cmp(const void *a,const void *b,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)if(((const unsigned char *)a)[i]!=((const unsigned char *)b)[i])return 1;return 0;}
static int media_strcmp(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
static char *media_chr(const char *p,int c)
{while(*p&&*p!=c)++p;return *p==c?(char *)p:0;}
static int media_upper(int c)
{
    if(c>='a'&&c<='i')return c-'a'+'A';
    if(c>='j'&&c<='r')return c-'j'+'J';
    if(c>='s'&&c<='z')return c-'s'+'S';return c;
}
static int media_alpha(int c)
{c=media_upper(c);return (c>='A'&&c<='I')||(c>='J'&&c<='R')||(c>='S'&&c<='Z');}
static int media_digit(int c)
{return c>='0'&&c<='9';}
static int media_space(int c)
{return c==' '||c=='\t'||c=='\n'||c=='\r';}

/* Only the bounded formats in media_commands.inc are admitted. */
static int media_scan(const char *input,const char *format,...)
{
    va_list args;unsigned int width,n,value,base;int count=0,digit,sign;
    char kind,*out;int *number;
    const char *digits="0123456789ABCDEF";
    va_start(args,format);
    while(*format){
        if(media_space(*format)){while(media_space(*input))++input;++format;continue;}
        if(*format++!='%')break;
        width=0U;while(media_digit(*format))width=width*10U+(unsigned int)(*format++-'0');
        kind=*format++;
        if(kind!='c')while(media_space(*input))++input;
        if(!*input)break;
        if(kind=='s'){
            out=va_arg(args,char *);n=0U;if(!width)break;
            while(*input&&!media_space(*input)&&n<width)out[n++]=*input++;
            out[n]=0;if(!n)break;
        }else if(kind=='c'){out=va_arg(args,char *);*out=*input++;}
        else if(kind=='d'||kind=='x'){
            number=va_arg(args,int *);value=n=0U;sign=1;base=kind=='x'?16U:10U;
            if(*input=='-'){sign=-1;++input;}
            if(base==16U&&input[0]=='0'&&(input[1]=='x'||input[1]=='X'))input+=2;
            while(*input){
                const char *at=media_chr(digits,media_upper(*input));
                if(!at)break;digit=(int)(at-digits);
                if((unsigned int)digit>=base)break;
                if(value>(0x7fffffffU-(unsigned int)digit)/base){n=0U;break;}
                value=value*base+(unsigned int)digit;++input;++n;
            }
            if(!n)break;*number=sign*(int)value;
        }else break;
        ++count;
    }
    va_end(args);return count;
}
static char media_print_buffer[256];
static unsigned int media_print_used;
static int media_printf(const char *format,...)
{
    va_list args;char *text=media_print_buffer,digits[16];const char *s;
    unsigned int n=media_print_used,width,precision,value,base,k,take;int left,zero,v,negative,wide,complete=0;
    va_start(args,format);
    while(*format&&n<256U){
        if(*format!='%'){if(*format=='\n'){complete=1;break;}text[n++]=*format++;continue;}
        ++format;left=zero=wide=0;width=0U;precision=0xffffffffU;
        if(*format=='-'){left=1;++format;}if(*format=='0'){zero=1;++format;}
        while(media_digit(*format))width=width*10U+(unsigned int)(*format++-'0');
        if(*format=='.'){++format;precision=0U;while(media_digit(*format))precision=precision*10U+(unsigned int)(*format++-'0');}
        if(*format=='l'){wide=1;++format;}
        if(*format=='s'){
            s=va_arg(args,const char *);take=0U;
            while(take<precision&&s[take])++take;
            while(!left&&width>take&&n<256U){text[n++]=' ';--width;}
            for(k=0U;k<take&&n<256U;++k)text[n++]=s[k];
            while(left&&width>take&&n<256U){text[n++]=' ';--width;}
        }else if(*format=='c'){text[n++]=(char)va_arg(args,int);}
        else if(*format=='d'||*format=='u'||*format=='X'){
            negative=0;
            if(*format=='d'){v=va_arg(args,int);negative=v<0;value=negative?0U-(unsigned int)v:(unsigned int)v;}
            else value=wide?(unsigned int)va_arg(args,unsigned long):va_arg(args,unsigned int);
            base=*format=='X'?16U:10U;
            k=0U;do{digits[k++]="0123456789ABCDEF"[value%base];value/=base;}while(value);
            take=k+(negative?1U:0U);
            if(negative&&zero&&n<256U)text[n++]='-';
            while(width>take&&n<256U){text[n++]=zero?'0':' ';--width;}
            if(negative&&!zero&&n<256U)text[n++]='-';
            while(k&&n<256U)text[n++]=digits[--k];
        }else if(*format=='%')text[n++]='%';else break;
        if(*format)++format;
    }
    va_end(args);media_print_used=n;
    if(complete||n==256U){media_print_used=0U;return (int)console_v1_line((const unsigned char *)text,n);}
    return 0;
}
static int media_getdevn(int ssid)
{return (int)TSCDEV((unsigned int)ssid,TSCSCHIB(&channel));}
static int media_getssid(int address)
{
    int i,found;for(i=0;i<4096;++i){found=media_getdevn(0x10000+i);if(!found)break;
        if(found==address)return TSCENABL((unsigned int)(0x10000+i),TSCSCHIB(&channel))?0:0x10000+i;}
    return 0;
}
static int media_rdblock(int device,int c,int h,int r,void *buffer,int capacity,int command)
{
    unsigned int got;
    if(device<=0||capacity<1||TSCBUILDREAD(&channel,(unsigned int)c,(unsigned int)h,
        (unsigned int)r,(unsigned int)command,(unsigned int)capacity)!=TSC_OK||
        TSCIO((unsigned int)device,TSCORB(&channel),TSCIRB(&channel)))return -1;
    if(TSCCHECKEND(&channel,(unsigned int)capacity)==TSC_OK)return 0;
    if(TSCCHECKREAD(&channel,(unsigned int)capacity,&got)!=TSC_OK)return -1;
    media_copy(buffer,TSCDATA(&channel),got);return (int)got;
}
static int media_wrblock(int device,int c,int h,int r,const void *buffer,int bytes,int command)
{
    unsigned int got;
    if(device<=0||bytes<1||TSCBUILDBLOCK(&channel,(unsigned int)c,(unsigned int)h,
        (unsigned int)r,(unsigned int)command,(unsigned int)bytes)!=TSC_OK)return -1;
    media_copy(TSCDATA(&channel),buffer,(unsigned int)bytes);
    if(TSCIO((unsigned int)device,TSCORB(&channel),TSCIRB(&channel))||
       TSCCHECKREAD(&channel,(unsigned int)bytes,&got)!=TSC_OK||got!=(unsigned int)bytes)return -1;
    return bytes;
}
static int media_tape(int device,unsigned int command,void *buffer,unsigned int bytes)
{
    unsigned int got;
    if(device<=0||TSCBUILDTAPE(&channel,command,bytes)!=TSC_OK)return -1;
    if(command==1U)media_copy(TSCDATA(&channel),buffer,bytes);
    if(TSCIO((unsigned int)device,TSCORB(&channel),TSCIRB(&channel))||
       TSCCHECKTAPE(&channel,command,bytes,&got)!=TSC_OK)return -1;
    if(command==2U&&got)media_copy(buffer,TSCDATA(&channel),got);return (int)got;
}
static int media_rdtape(int device,void *buffer,int bytes)
{return bytes>0?media_tape(device,2U,buffer,(unsigned int)bytes):-1;}
static int media_wrtape(int device,const void *buffer,int bytes)
{return bytes>0?media_tape(device,1U,(void *)buffer,(unsigned int)bytes):-1;}
static int media_tapectl(int device,int command)
{return media_tape(device,(unsigned int)command,0,1U)<0?-1:0;}
static void *media_malloc(unsigned int bytes)
{
    if(media_scratch_real||!bytes||TSRALLOC(&storage.real,0x4d454449U,
        (bytes+4095U)&~4095U,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&media_scratch_real)!=TSR_OK)return 0;
    return (unsigned char *)TSF_KAPERTURE_VA+media_scratch_real;
}
static void media_free(void *p)
{
    if(p==(unsigned char *)TSF_KAPERTURE_VA+media_scratch_real&&
       TSRRELEASE(&storage.real,0x4d454449U,media_scratch_real)==TSR_OK)media_scratch_real=0U;
}
static void media_creation_date(unsigned char date[3])
{
    unsigned int clock[2],hi,lo,seconds=0U,remainder=0U,i,days,year=1900U,span;
    TSCNOW(clock);hi=clock[0]>>12;lo=(clock[0]<<20)|(clock[1]>>12);
    for(i=0U;i<64U;++i){
        remainder=(remainder<<1)|((i<32U?hi:lo)>>(31U-(i%32U))&1U);
        seconds<<=1;if(remainder>=1000000U){remainder-=1000000U;seconds|=1U;}
    }
    days=seconds/86400U;
    for(;;){span=365U+(!(year%4U)&&(year%100U||!(year%400U))?1U:0U);
        if(days<span)break;days-=span;++year;}
    date[0]=(unsigned char)(year-1900U);date[1]=(unsigned char)((days+1U)>>8);date[2]=(unsigned char)(days+1U);
}
static int media_dscb(int device,const char *dsn,DSCB1 *result,int *outc,int *outh,int *outr)
{
    const unsigned char *raw;unsigned int c,h,r,n=media_len(dsn);int got;
    if(!n||n>44U)return 0;
    for(c=1U;c<=2U;++c)for(h=0U;h<15U;++h)for(r=c==1U&&h==0U?3U:1U;r<=100U;++r){
        got=media_rdblock(device,(int)c,(int)h,(int)r,result,140,0x0e);
        if(got<0)break;if(got!=140||!result->ds1dsnam[0])return 0;
        raw=(const unsigned char *)result;
        if(raw[44U]!=0xf1U||media_cmp(raw,dsn,n))continue;
        {unsigned int i;for(i=n;i<44U&&raw[i]==0x40U;++i){}if(i!=44U)continue;}
        if(outc)*outc=(int)c;if(outh)*outh=(int)h;if(outr)*outr=(int)r;return 1;
    }
    return 0;
}
static int media_cms_check(KMEDIA *unused,const char *input)
{
    char verb[9],name[9],extra;int mode,i,n;unsigned char member[8];TSLIMAGE *image;
    (void)unused;
    if(media_scan(input,"%8s %d %8s %c",verb,&mode,name,&extra)!=3||
       media_strcmp(verb,"CHECK")||(mode!=24&&mode!=31))return 8;
    n=(int)media_len(name);for(i=0;i<8;++i)member[i]=(unsigned char)(i<n?media_upper(name[i]):' ');
    if(load_native(TSV_CMS,(unsigned int)mode,member,&image)||load_refresh(image))return 12;
    media_printf("CMS CHECK: %s valid\n",name);return 0;
}
#define PDOS KMEDIA
#define PDOS_MEDIA_PORT 1
#define MAXBLKSZ 32767
#define NULL 0
#define memcpy media_copy
#define memset media_set
#define memcmp media_cmp
#define strlen media_len
#define strcmp media_strcmp
#define strchr media_chr
#define toupper media_upper
#define isalpha media_alpha
#define isalnum(c) (media_alpha(c)||media_digit(c))
#define sscanf media_scan
#define printf media_printf
#define rdblock media_rdblock
#define wrblock media_wrblock
#define getdevn media_getdevn
#define getssid media_getssid
#define rdtape media_rdtape
#define wrtape media_wrtape
#define tapectl media_tapectl
#define malloc media_malloc
#define free media_free
#define pdos64ReadDscbLoc media_dscb
#define pdosCmsCommand media_cms_check
#include "media_commands.inc"
#undef PDOS
#undef PDOS_MEDIA_PORT
#undef MAXBLKSZ
#undef NULL
#undef memcpy
#undef memset
#undef memcmp
#undef strlen
#undef strcmp
#undef strchr
#undef toupper
#undef isalpha
#undef isalnum
#undef sscanf
#undef printf
#undef rdblock
#undef wrblock
#undef getdevn
#undef getssid
#undef rdtape
#undef wrtape
#undef tapectl
#undef malloc
#undef free
#undef pdos64ReadDscbLoc
#undef pdosCmsCommand
static int media_init(void)
{
    unsigned int boot=*(volatile const unsigned int *)0x40bcU;
    if(media_state.volume_count)return 0;
    if(!boot||!pdosReadVolumeLabel((int)boot,media_state.volumes[0].volser))return 12;
    media_state.curdev=media_state.volumes[0].device=(int)boot;
    media_state.volumes[0].address=media_getdevn((int)boot);media_state.volume_count=1;return 0;
}
static int media_directory(void)
{
    DSCB1 d;unsigned int c,h,r,year,day,month,days;int n;char format;
    static const unsigned int months[12]={31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U};
    for(c=1U;c<=2U;++c)for(h=0U;h<15U;++h)for(r=c==1U&&h==0U?3U:1U;r<=100U;++r){
        n=media_rdblock(media_state.curdev,(int)c,(int)h,(int)r,&d,140,0x0e);
        if(n<0)break;if(n!=140)return 12;if(!d.ds1dsnam[0])return 0;
        if(d.ds1fmtid!='1')return 8;
        year=1900U+(unsigned char)d.ds1credt[0];
        day=((unsigned int)(unsigned char)d.ds1credt[1]<<8)|(unsigned char)d.ds1credt[2];
        if(!day)day=1U;
        for(month=0U;month<11U;++month){days=months[month]+(month==1U&&!(year%4U)&&(year%100U||!(year%400U))?1U:0U);
            if(day<=days)break;day-=days;}
        format=(d.ds1recfm&0xc0)==0x80?'F':(d.ds1recfm&0xc0)==0x40?'V':'U';
        media_printf("%-44.44s %04u-%02u-%02u %c%s %u %u (%u %u 1)\n",
            d.ds1dsnam,year,month+1U,day,format,(d.ds1recfm&0x10)?"B":"",
            (unsigned short)d.ds1lrecl,(unsigned short)d.ds1blkl,
            ((unsigned int)(unsigned char)d.startcchh[0]<<8)|(unsigned char)d.startcchh[1],
            ((unsigned int)(unsigned char)d.startcchh[2]<<8)|(unsigned char)d.startcchh[3]);
    }
    return 12;
}
static int media_known(const unsigned char name[8],const unsigned char *parameters,unsigned int count,char verb[9])
{
    unsigned int i=0U;while(i<8U&&name[i]!=0x40U){verb[i]=(char)name[i];++i;}verb[i]=0;
    if(!media_strcmp(verb,"CMS"))return count>=6U&&!media_cmp(parameters+2U,"CHECK ",6U);
    return !media_strcmp(verb,"DEVICES")||!media_strcmp(verb,"VOLUMES")||
           !media_strcmp(verb,"MOUNT")||!media_strcmp(verb,"SELECT")||
           !media_strcmp(verb,"UNMOUNT")||!media_strcmp(verb,"ALLOC")||
           !media_strcmp(verb,"RCOPY")||!media_strcmp(verb,"TAPE")||!media_strcmp(verb,"DIR");
}
static int media_operator(const char *verb,const unsigned char *parameters)
{
    int rc=media_init();
    if(!rc)rc=!media_strcmp(verb,"DIR")?media_directory():
               pdosMediaCommand(&media_state,verb,(char *)parameters);
    media_selected_device=(unsigned int)media_state.curdev;return rc;
}
