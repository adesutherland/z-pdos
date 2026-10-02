/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original classic-object fixtures and independent byte/record expectations.
 * No native object or macro expansion is embedded here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"origin check failed, line %d: %s\n",__LINE__,#c); exit(1); } } while (0)
struct deck { unsigned char bytes[4096]; size_t length, txt, target_txt, rld, ld; };
static const unsigned char source_name[] = {0xe2,0xd6,0xe4,0xd9,0xc3,0xc5};
static const unsigned char target_name[] = {0xe3,0xc1,0xd9,0xc7,0xc5,0xe3};
static const unsigned char entry_name[] = {0xc5,0xd5,0xe3,0xd9,0xe8};
static const unsigned char pad_name[] = {0xd7,0xc1,0xc4};
static const unsigned char pattern[] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0};
static void put(unsigned char *b, unsigned long n, size_t size)
{ while (size) { b[--size] = (unsigned char)(n & 255UL); n >>= 8; } }
static unsigned long get(const unsigned char *b, size_t size)
{ unsigned long n; n = 0; while (size--) n = (n << 8) | *b++; return n; }
static unsigned char *card(struct deck *d, unsigned a, unsigned b, unsigned c,
                          unsigned long address, unsigned ident, size_t length)
{
    unsigned char *p;
    CHECK(length <= 56 && d->length + 80 <= sizeof d->bytes);
    p = d->bytes + d->length; d->length += 80; memset(p, 0x40, 80);
    p[0] = 2; p[1] = (unsigned char)a; p[2] = (unsigned char)b; p[3] = (unsigned char)c;
    put(p+5,address,3); put(p+10,(unsigned long)length,2); put(p+14,ident,2); return p+16;
}
static void esd(unsigned char *p, const unsigned char *name, size_t n,
                unsigned kind, unsigned long origin, unsigned long length)
{
    memset(p,0x40,8); if (n) memcpy(p,name,n); p[8]=(unsigned char)kind;
    put(p+9,origin,3); p[12]=0; put(p+13,length,3);
}
static void rld(unsigned char *p, unsigned target, unsigned source,
                unsigned flags, unsigned long address)
{ put(p,target,2); put(p+2,source,2); p[4]=(unsigned char)flags; put(p+5,address,3); }
static void make_deck(struct deck *d, const char *which)
{
    unsigned char *p; unsigned source, target, er; unsigned long so, to;
    int extra, private_code;
    memset(d,0,sizeof *d); extra=!strcmp(which,"source"); private_code=!strcmp(which,"private");
    source=extra?2:1; target=source+1; er=target+1;
    so=extra?0x40UL:private_code?0x80UL:0;
    to=!strcmp(which,"zero")?0:extra||private_code?0x100UL:0x20UL;
    p=card(d,0xc5,0xe2,0xc4,0,1,48);
    if (extra) { esd(p,pad_name,sizeof pad_name,0,0,16); p+=16; }
    esd(p,source_name,private_code?0:sizeof source_name,private_code?4:0,so,32); p+=16;
    esd(p,target_name,sizeof target_name,0,to,16); p+=16;
    if (!extra) esd(p,entry_name,sizeof entry_name,2,0,0);
    p=card(d,0xc5,0xe2,0xc4,0,extra?er:er+1,extra?48:32);
    if (extra) { esd(p,entry_name,sizeof entry_name,2,0,0); p+=16; }
    esd(p,target_name,sizeof target_name,2,0,0); p+=16;
    d->ld=(size_t)(p-d->bytes); esd(p,entry_name,sizeof entry_name,1,to+8,target);
    d->txt=d->length; p=card(d,0xe3,0xe7,0xe3,so,source,16);
    put(p,to+4,4); put(p+4,(0x100UL-to)&0xffffffffUL,4); put(p+8,0,4); put(p+12,4,4);
    p=card(d,0xe3,0xe7,0xe3,so+16,source,16); memset(p,0,16);
    d->target_txt=d->length; p=card(d,0xe3,0xe7,0xe3,to,target,8); memcpy(p,pattern,8);
    p=card(d,0xe3,0xe7,0xe3,to+8,target,8); memcpy(p,pattern+8,8);
    d->rld=d->length; p=card(d,0xd9,0xd3,0xc4,0,1,32);
    rld(p,target,source,0x0c,so); rld(p+8,target,source,0x0e,so+4);
    rld(p+16,er,source,0x1c,so+8); rld(p+24,er+1,source,0x1c,so+12);
    card(d,0xc5,0xd5,0xc4,0,1,0);
}
static void write_deck(const char *folder, const char *name, const struct deck *d)
{
    char path[4096]; FILE *f;
    CHECK(strlen(folder)+strlen(name)+6 < sizeof path);
    strcpy(path,folder); strcat(path,"/"); strcat(path,name); strcat(path,".obj");
    f=fopen(path,"wb"); CHECK(f!=NULL);
    CHECK(fwrite(d->bytes,1,d->length,f)==d->length); CHECK(fclose(f)==0);
}
static void prepare(const char *folder)
{
    static const char *names[]={"nonzero","zero","source","private"};
    struct deck d; unsigned char *p; size_t i;
    for (i=0;i<sizeof names/sizeof names[0];++i) { make_deck(&d,names[i]); write_deck(folder,names[i],&d); }
    memset(&d,0,sizeof d); p=card(&d,0xc5,0xe2,0xc4,0,1,48);
    esd(p,NULL,0,4,0,8); esd(p+16,entry_name,sizeof entry_name,2,0,0); esd(p+32,target_name,sizeof target_name,2,0,0);
    p=card(&d,0xe3,0xe7,0xe3,0,1,8); put(p,0,4); put(p+4,4,4);
    p=card(&d,0xd9,0xd3,0xc4,0,1,16); rld(p,2,1,0x1c,0); rld(p+8,3,1,0x1c,4);
    card(&d,0xc5,0xd5,0xc4,0,1,0); write_deck(folder,"caller",&d);
    memset(&d,0,sizeof d); p=card(&d,0xc5,0xe2,0xc4,0,1,16); esd(p,NULL,0,4,0,8);
    p=card(&d,0xe3,0xe7,0xe3,0,1,8); memset(p,0,8); card(&d,0xc5,0xd5,0xc4,0,1,0);
    write_deck(folder,"prefix",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.target_txt+5,31,3); write_deck(folder,"bad_txt_before",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.target_txt+5,47,3); write_deck(folder,"bad_txt_after",&d);
    make_deck(&d,"source"); put(d.bytes+d.rld+21,63,3); write_deck(folder,"bad_rld_before",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.rld+21,31,3); write_deck(folder,"bad_rld_after",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.ld+9,31,3); write_deck(folder,"bad_ld_before",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.rld+18,65535UL,2); write_deck(folder,"bad_id",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.rld+16,65535UL,2); write_deck(folder,"bad_target",&d);
    make_deck(&d,"nonzero"); put(d.bytes+d.rld+16,0,2); write_deck(folder,"bad_hole",&d);
}
static unsigned char *read_file(const char *path, size_t *length)
{
    FILE *f; long n; unsigned char *b;
    f=fopen(path,"rb"); CHECK(f!=NULL); CHECK(fseek(f,0,SEEK_END)==0); n=ftell(f); CHECK(n>=0);
    CHECK(fseek(f,0,SEEK_SET)==0); b=(unsigned char *)malloc((size_t)n+1); CHECK(b!=NULL);
    CHECK(fread(b,1,(size_t)n,f)==(size_t)n); CHECK(fclose(f)==0); b[n]=0; *length=(size_t)n; return b;
}
static void map_symbol(const unsigned char *map, const char *name, unsigned long expected)
{
    const char *at, *line;
    at=strstr((const char *)map,name); CHECK(at!=NULL); line=at;
    while (line>(const char *)map && line[-1]!='\n') --line;
    CHECK(strtoul(line,NULL,16)==expected);
}
static void mvs_image(const unsigned char *b, size_t length, unsigned char *image,
                      size_t image_size, size_t prefix, size_t source, size_t count)
{
    size_t at, end, n, data, destination, pending_size, i, rlds, headers;
    unsigned char flag; int pending, eof; unsigned char written[4096], seen[4096];
    memset(written,0,sizeof written); memset(seen,0,sizeof seen); at=0; rlds=headers=0; pending=eof=0; pending_size=destination=0;
    /* COPYR1, COPYR2, and directory each carry their own RDW. */
    for (i=0;i<3;++i) { CHECK(at+4<=length); n=(size_t)get(b+at,2); CHECK(n>=4 && n<=length-at); at+=n; }
    while (at<length) {
        CHECK(at+4<=length); n=(size_t)get(b+at,2); CHECK(n>=4 && n<=length-at); end=at+n; at+=4;
        while (at<end) {
            CHECK(end-at>=12); data=at+12; n=(size_t)get(b+at+10,2); CHECK(n<=end-data); at=data+n; ++headers;
            if (!n) { eof=1; continue; }
            if (pending) {
                CHECK(n==pending_size && destination+n<=image_size);
                for (i=0;i<n;++i) { CHECK(!written[destination+i]); written[destination+i]=1; }
                memcpy(image+destination,b+data,n); pending=0; continue;
            }
            if ((b[data]&3)==1 || (b[data]&3)==3) {
                CHECK(n>=18 && b[data+8]==6 && b[data+12]==0x40);
                destination=(size_t)get(b+data+9,3); pending_size=(size_t)get(b+data+13,3); pending=1;
            } else if ((b[data]&3)==2) {
                CHECK(n>=16 && get(b+data+6,2)==n-16 && (n-16)%8==0);
                for (i=16;i<n;i+=8) {
                    destination=(size_t)get(b+data+i+5,3); flag=b[data+i+4];
                    CHECK(get(b+data+i,2)!=0 && get(b+data+i+2,2)!=0);
                    CHECK(destination+4<=image_size && !seen[destination]); seen[destination]=1;
                    CHECK(flag==((destination==prefix+source+4)?0x1e:0x1c)); ++rlds;
                }
            }
        }
        CHECK(at==end);
    }
    CHECK(!pending && eof && headers>4 && rlds==count);
    for (i=0;i<image_size;++i) CHECK(written[i]);
    CHECK(seen[prefix+source] && seen[prefix+source+4] && seen[prefix+source+8] && seen[prefix+source+12]);
    if (count==6) CHECK(seen[0] && seen[4]);
}
static void check_output(const char *which, unsigned long base, const char *format,
                          const char *output, const char *map_path)
{
    unsigned char expected[4096], image[4096], *b, *map; size_t length, map_length, prefix, source, target, size, i; int caller;
    caller=!strcmp(which,"caller"); prefix=caller||!strncmp(which,"prefix_",7)?8:0;
    if (!strncmp(which,"prefix_",7)) which+=7;
    source=!strcmp(which,"source")?0x40:!strcmp(which,"private")?0x80:0;
    target=source?0x100:0x20; size=prefix+target+16;
    memset(expected,0,sizeof expected); memset(image,0,sizeof image);
    put(expected+prefix+source,base+(unsigned long)(prefix+target)+4,4);
    put(expected+prefix+source+4,(0x100UL-base-(unsigned long)(prefix+target))&0xffffffffUL,4);
    put(expected+prefix+source+8,base+(unsigned long)(prefix+target)+8,4);
    put(expected+prefix+source+12,base+(unsigned long)(prefix+target)+4,4);
    memcpy(expected+prefix+target,pattern,sizeof pattern);
    if (caller) { put(expected,base+(unsigned long)(prefix+target)+8,4); put(expected+4,base+(unsigned long)(prefix+target)+4,4); }
    b=read_file(output,&length);
    if (!strcmp(format,"binary")) { CHECK(length==size); memcpy(image,b,size); }
    else { CHECK(!strcmp(format,"mvs") && base==0); mvs_image(b,length,image,size,prefix,source,caller?6:4); }
    for (i=0;i<size;++i) CHECK(image[i]==expected[i]);
    map=read_file(map_path,&map_length); CHECK(map_length>0);
    map_symbol(map,"TARGET",base+(unsigned long)(prefix+target)); map_symbol(map,"ENTRY",base+(unsigned long)(prefix+target)+8);
    if (strcmp(which,"private")) map_symbol(map,"SOURCE",base+(unsigned long)(prefix+source));
    free(b); free(map);
}
int main(int argc, char **argv)
{
    if (argc==3 && !strcmp(argv[1],"prepare")) prepare(argv[2]);
    else if (argc==7 && !strcmp(argv[1],"check")) check_output(argv[2],strtoul(argv[3],NULL,0),argv[4],argv[5],argv[6]);
    else { fputs("test_pdld_origin prepare DIR | check CASE BASE FORMAT OUTPUT MAP\n",stderr); return 2; }
    printf("PDLD origin: %lu checks passed\n",checks); return 0;
}
