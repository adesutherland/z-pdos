/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent numeric deck vectors; IBM z/VM 7.4 OBJSTMT format facts.
 */
#include "mf_classic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "object:%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static unsigned checks;
struct fixture {
    void *allocations[8]; unsigned allocations_count, allocation_fail;
    mf_octet bytes[8192]; size_t length;
    unsigned writes, fail_write, begins, finishes, invalids;
    int fail_begin, fail_finish, valid;
    struct mf_obj *object; struct mf_object_writer writer;
};
static void *acquire(void *cookie, size_t n)
{
    struct fixture *f = (struct fixture *)cookie;
    void *p;
    if (f->allocation_fail == f->allocations_count + 1) return NULL;
    p = malloc(n); CHECK(p != NULL); CHECK(f->allocations_count < 8);
    f->allocations[f->allocations_count++] = p; return p;
}
static enum mf_status sink_begin(void *cookie)
{
    struct fixture *f = (struct fixture *)cookie;
    ++f->begins; return f->fail_begin ? MF_IO : MF_OK;
}
static enum mf_status sink_write(void *cookie, const mf_octet *p, size_t n)
{
    struct fixture *f = (struct fixture *)cookie;
    ++f->writes; CHECK(n == 80);
    if (f->writes == f->fail_write) {
        /* A failed device may already have written a short prefix. */
        memcpy(f->bytes + f->length, p, 7); f->length += 7;
        return MF_IO;
    }
    CHECK(f->length + n <= sizeof(f->bytes));
    memcpy(f->bytes + f->length, p, n); f->length += n; return MF_OK;
}
static enum mf_status sink_finish(void *cookie, int valid)
{
    struct fixture *f = (struct fixture *)cookie;
    ++f->finishes;
    if (!valid) { ++f->invalids; f->valid = 0; }
    else if (f->fail_finish) return MF_IO;
    else f->valid = 1;
    return MF_OK;
}
static void init(struct fixture *f)
{
    struct mf_storage storage; struct mf_sink sink;
    memset(f, 0, sizeof(*f)); storage.cookie = f; storage.acquire = acquire;
    sink.cookie = f; sink.begin = sink_begin; sink.write = sink_write;
    sink.finish = sink_finish;
    CHECK(mf_obj_create(&storage, &sink, 4, 8, &f->object, &f->writer) == MF_OK);
}
static void clean(struct fixture *f)
{
    unsigned i; mf_obj_destroy(f->object);
    for (i = 0; i < f->allocations_count; ++i) free(f->allocations[i]);
}
static struct mf_span span(const mf_octet *s, size_t n)
{
    struct mf_span x; x.data = s; x.length = n; return x;
}
static struct mf_section sec(unsigned id, mf_u32 n)
{
    static const mf_octet nm[] = {0x43,0x4f,0x44,0x45};
    struct mf_section s; memset(&s, 0, sizeof(s));
    s.id = id; s.name = span(nm, sizeof(nm)); s.length = n;
    s.amode = 24; s.rmode = 24; return s;
}
static mf_u32 num(const mf_octet *p, unsigned n)
{
    mf_u32 x = 0; while (n--) x = x * 256UL + *p++; return x;
}
static void tag(const mf_octet *p, unsigned a, unsigned b, unsigned c)
{
    CHECK(p[0] == 2 && p[1] == a && p[2] == b && p[3] == c);
}
static void good_deck(void)
{
    static const mf_octet second[] = {0x44,0x41,0x54,0x41};
    static const mf_octet ext[] = {0x45,0x58,0x54};
    static const mf_octet exp[] = {0x53,0x54,0x41,0x52,0x54};
    static const mf_octet long_local[] = {0x4c,0x4f,0x4e,0x47,0x4c,0x4f,0x43,0x41,0x4c};
    static const mf_octet code_cp[] = {0xc3,0xd6,0xc4,0xc5,0x40,0x40,0x40,0x40};
    struct fixture f; struct mf_section s[3]; struct mf_symbol y[3];
    struct mf_fixup r; struct mf_entry e; mf_octet data[65];
    const mf_octet *p; unsigned i;
    init(&f); s[0] = sec(71, 80); s[0].amode = 31; s[0].rmode = 31;
    s[1] = sec(9, 8); s[1].name = span(second, sizeof(second));
    s[2] = sec(30, 16); s[2].dummy = 1;
    memset(y, 0, sizeof(y));
    y[0].id = 71; y[0].name = span(exp, sizeof(exp));
    y[0].kind = MF_EXPORT; y[0].section = 71; y[0].offset = 2;
    y[1].id = 99; y[1].name = span(ext, sizeof(ext)); y[1].kind = MF_EXTERNAL;
    y[2].id = 111; y[2].name = span(long_local, sizeof(long_local)); y[2].kind = MF_LOCAL;
    CHECK(f.writer.begin(f.writer.cookie, s, 3, y, 3) == MF_OK);
    CHECK(f.length == 320); p = f.bytes;
    tag(p,0xc5,0xe2,0xc4); CHECK(num(p+10,2)==16 && num(p+14,2)==1);
    CHECK(!memcmp(p+16,code_cp,8) && p[24]==0 && num(p+25,3)==0);
    CHECK(p[28]==6 && num(p+29,3)==80);
    p += 80; CHECK(num(p+14,2)==2 && p[28]==1 && num(p+29,3)==8);
    p += 80; CHECK(num(p+14,2)==3 && p[24]==2 && p[25]==0x40 && p[31]==0x40);
    p += 80; CHECK(p[14]==0x40 && p[15]==0x40 && p[24]==1);
    CHECK(num(p+25,3)==2 && num(p+29,3)==1);
    for (i=0;i<65;++i) data[i]=(mf_octet)i;
    CHECK(f.writer.text(f.writer.cookie,71,0,data,30)==MF_OK);
    CHECK(f.writer.text(f.writer.cookie,71,30,data+30,35)==MF_OK);
    memset(&r,0,sizeof(r)); r.section=71; r.offset=4; r.width=4;
    r.target_kind=MF_REF_SECTION; r.target=9; r.address_kind=MF_ADDRESS_A;
    CHECK(f.writer.fixup(f.writer.cookie,&r)==MF_OK);
    r.target_kind=MF_REF_EXTERNAL; r.target=99; r.offset=8;
    r.address_kind=MF_ADDRESS_V;
    CHECK(f.writer.fixup(f.writer.cookie,&r)==MF_OK);
    r.address_kind=MF_ADDRESS_A; r.subtract=1; r.offset=12;
    CHECK(f.writer.fixup(f.writer.cookie,&r)==MF_OK);
    CHECK(f.writer.gap(f.writer.cookie,71,65,7)==MF_OK);
    CHECK(f.writer.text(f.writer.cookie,71,72,data,8)==MF_OK);
    CHECK(f.writer.text(f.writer.cookie,9,0,data,8)==MF_OK);
    e.present=1; e.section=71; e.offset=2;
    CHECK(f.writer.entry(f.writer.cookie,&e)==MF_OK);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_OK && f.valid && f.finishes==1);
    CHECK(f.length==960);
    p=f.bytes+320; tag(p,0xe3,0xe7,0xe3);
    CHECK(num(p+5,3)==0 && num(p+10,2)==56 && num(p+14,2)==1);
    CHECK(!memcmp(p+16,data,56));
    p+=80; CHECK(num(p+5,3)==56 && num(p+10,2)==9 && !memcmp(p+16,data+56,9));
    p+=80; tag(p,0xd9,0xd3,0xc4); CHECK(num(p+10,2)==8);
    CHECK(num(p+16,2)==2 && num(p+18,2)==1 && p[20]==0x0c && num(p+21,3)==4);
    p+=80; CHECK(num(p+16,2)==3 && p[20]==0x1c && num(p+21,3)==8);
    p+=80; CHECK(num(p+16,2)==3 && p[20]==0x0e && num(p+21,3)==12);
    p+=80; CHECK(num(p+5,3)==72 && num(p+10,2)==8 && num(p+14,2)==1);
    p+=80; CHECK(num(p+5,3)==0 && num(p+14,2)==2);
    p+=80; tag(p,0xc5,0xd5,0xc4); CHECK(num(p+5,3)==2 && num(p+14,2)==1);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_OBJECT && f.finishes==1);
    clean(&f);
}
static void failure(unsigned which, enum mf_status expected)
{
    static const mf_octet badname[]={0xc3,0xa9};
    static const mf_octet longname[]={65,66,67,68,69,70,71,72,73};
    struct fixture f; struct mf_section s[2]; struct mf_symbol y;
    struct mf_fixup r; struct mf_entry e; mf_octet bytes[80]; enum mf_status st;
    init(&f); s[0]=sec(4,80); s[1]=sec(5,80); s[1].name=span(NULL,0);
    memset(&y,0,sizeof(y)); y.id=11; y.kind=MF_EXTERNAL;
    y.name=s[0].name;
    memset(bytes,0,sizeof(bytes)); st=MF_OK;
    if(which==0) s[0].amode=64;
    if(which==1) s[0].length=0x1000000UL;
    if(which==2) s[0].name=span(longname,sizeof(longname));
    if(which==3) s[0].name=span(badname,sizeof(badname));
    if(which==4) s[1].id=4;
    if(which==5) f.fail_begin=1;
    if(which==6) f.fail_write=1;
    st=f.writer.begin(f.writer.cookie,s,which==4?2:1,NULL,0);
    if(which>=7) CHECK(st==MF_OK);
    if(which==7) st=f.writer.text(f.writer.cookie,4,79,bytes,2);
    if(which==8) { CHECK(f.writer.gap(f.writer.cookie,4,0,8)==MF_OK); st=f.writer.text(f.writer.cookie,4,4,bytes,1); }
    if(which==9) st=f.writer.text(f.writer.cookie,77,0,bytes,1);
    if(which==10) st=f.writer.gap(f.writer.cookie,4,79,2);
    memset(&r,0,sizeof(r)); r.width=4; r.section=4; r.target_kind=MF_REF_SECTION;
    r.target=4; r.address_kind=MF_ADDRESS_A;
    if(which>=11 && which<=13) CHECK(f.writer.text(f.writer.cookie,4,0,bytes,4)==MF_OK);
    if(which==11) {r.width=8; st=f.writer.fixup(f.writer.cookie,&r);}
    if(which==12) {r.target=999; st=f.writer.fixup(f.writer.cookie,&r);}
    if(which==13) {r.offset=78; st=f.writer.fixup(f.writer.cookie,&r);}
    if(which==14) {e.section=4;e.offset=80;e.present=1;st=f.writer.entry(f.writer.cookie,&e);}
    if(which==15) {f.fail_write=2;CHECK(f.writer.text(f.writer.cookie,4,0,bytes,1)==MF_OK); st=f.writer.finish(f.writer.cookie,1);}
    if(which==16) {f.fail_finish=1;st=f.writer.finish(f.writer.cookie,1);}
    if(which==17) st=f.writer.finish(f.writer.cookie,0);
    if(which==18) { mf_obj_destroy(f.object); st=f.writer.text(f.writer.cookie,4,0,bytes,1); }
    CHECK(st==expected);
    if(which<15) { CHECK(f.writer.text(f.writer.cookie,4,0,bytes,1)==expected); CHECK(f.writer.finish(f.writer.cookie,1)==expected); }
    CHECK(!f.valid && f.invalids==1);
    clean(&f);
}
static void edges(void)
{
    struct fixture f; struct mf_section s; struct mf_entry e;
    struct mf_storage storage; struct mf_sink sink; struct mf_object_writer w;
    struct mf_obj *obj; unsigned i;
    static const mf_octet eight[]={65,66,67,68,69,70,71,72};
    init(&f);s=sec(1,0xffffffUL);s.name=span(eight,8);s.amode=24;s.rmode=31;
    CHECK(f.writer.begin(f.writer.cookie,&s,1,NULL,0)==MF_OK);
    CHECK(f.bytes[28]==5 && num(f.bytes+29,3)==0xffffffUL);
    e.present=0;e.section=999;e.offset=0xffffffffUL;
    CHECK(f.writer.entry(f.writer.cookie,&e)==MF_OK);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_OK);
    CHECK(f.bytes[85]==0x40 && f.bytes[94]==0x40);clean(&f);
    for(i=1;i<=3;++i) {
        memset(&f,0,sizeof(f));f.allocation_fail=i;
        storage.cookie=&f;storage.acquire=acquire;
        sink.cookie=&f;sink.begin=sink_begin;sink.write=sink_write;sink.finish=sink_finish;
        obj=(struct mf_obj *)1;
        CHECK(mf_obj_create(&storage,&sink,4,8,&obj,&w)==MF_LIMIT && obj==NULL && w.begin==NULL);
        f.object=NULL;clean(&f);
    }
    init(&f);s=sec(1,8);
    CHECK(f.writer.begin(f.writer.cookie,&s,5,NULL,0)==MF_LIMIT);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_LIMIT && f.invalids==1);clean(&f);
    init(&f);s=sec(1,8);s.dummy=1;
    CHECK(f.writer.begin(f.writer.cookie,&s,1,NULL,0)==MF_OK && f.length==0);
    CHECK(f.writer.gap(f.writer.cookie,1,0,4)==MF_OBJECT);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_OBJECT && f.invalids==1);clean(&f);
}
static void definitions(void)
{
    static const mf_octet external[]={69,88,84};
    static const mf_octet longname[]={65,66,67,68,69,70,71,72,73};
    static const mf_octet nonascii[]={0xc3,0xa9};
    static const enum mf_status errors[]={MF_LIMIT,MF_UNSUPPORTED,MF_DUPLICATE,
        MF_DUPLICATE,MF_RANGE,MF_RANGE,MF_RANGE,MF_OBJECT,MF_OBJECT,MF_OBJECT,
        MF_UNSUPPORTED,MF_LIMIT};
    struct fixture f;struct mf_section s[2];struct mf_symbol y[2];
    unsigned i;enum mf_status st;
    for(i=0;i<sizeof(errors)/sizeof(errors[0]);++i){
        init(&f);s[0]=sec(1,8);s[1]=sec(2,8);s[1].dummy=1;
        memset(y,0,sizeof(y));y[0].id=8;y[0].kind=MF_EXTERNAL;
        y[0].name=span(external,3);y[1]=y[0];y[1].id=9;
        if(i==0)y[0].name=span(longname,9);
        if(i==1)y[0].name=span(nonascii,2);
        if(i==2)y[1].id=8;
        if(i==4||i==5||i==6){y[0].kind=MF_EXPORT;y[0].section=i==4?99:(i==5?2:1);y[0].offset=i==6?8:0;}
        if(i==7)y[0].section=1;
        if(i==8)y[0].kind=(enum mf_symbol_kind)99;
        if(i==9)y[0].id=0;
        if(i==10)s[0].rmode=64;
        st=f.writer.begin(f.writer.cookie,s,2,y,i==2||i==3?2:(i==11?9:1));
        CHECK(st==errors[i]);CHECK(f.begins==0);
        CHECK(f.writer.finish(f.writer.cookie,1)==errors[i]&&f.invalids==1);clean(&f);
    }
    init(&f);s[0]=sec(1,4);s[1]=sec(2,16);s[1].dummy=1;
    s[1].name=span(longname,9);
    CHECK(f.writer.begin(f.writer.cookie,s,2,NULL,0)==MF_OK&&f.length==80);
    CHECK(f.writer.finish(f.writer.cookie,1)==MF_OK);clean(&f);
}
static void relocation_failures(void)
{
    struct fixture f;struct mf_section s[2];struct mf_symbol y;
    struct mf_fixup r;mf_octet bytes[8];unsigned i;enum mf_status st;
    static const mf_octet ext[]={69,88,84};
    static const enum mf_status errors[]={MF_OBJECT,MF_OBJECT,MF_OBJECT,
        MF_UNDEFINED,MF_UNDEFINED,MF_OBJECT,MF_UNSUPPORTED,MF_OBJECT};
    memset(bytes,0,sizeof(bytes));
    for(i=0;i<sizeof(errors)/sizeof(errors[0]);++i){
        init(&f);s[0]=sec(1,16);s[1]=sec(2,16);s[1].dummy=1;
        memset(&y,0,sizeof(y));y.id=10;y.name=span(ext,3);y.kind=MF_LOCAL;
        CHECK(f.writer.begin(f.writer.cookie,s,2,&y,1)==MF_OK);
        memset(&r,0,sizeof(r));r.section=1;r.target=1;r.width=4;
        r.target_kind=MF_REF_SECTION;r.address_kind=MF_ADDRESS_A;
        if(i!=0)CHECK(f.writer.text(f.writer.cookie,1,0,bytes,8)==MF_OK);
        if(i==1)CHECK(f.writer.gap(f.writer.cookie,1,8,4)==MF_OK);
        if(i==2){CHECK(f.writer.text(f.writer.cookie,1,12,bytes,4)==MF_OK);r.offset=8;}
        if(i==3)r.target=2;
        if(i==4){r.target_kind=MF_REF_EXTERNAL;r.target=10;}
        if(i==5)r.target_kind=(enum mf_reference_kind)99;
        if(i==6)r.address_kind=(enum mf_address_kind)99;
        st=f.writer.fixup(f.writer.cookie,i==7?NULL:&r);
        CHECK(st==errors[i]);CHECK(f.writer.finish(f.writer.cookie,1)==errors[i]);
        CHECK(f.invalids==1&&!f.valid);clean(&f);
    }
}
static void short_writes(void)
{
    struct fixture f;struct mf_section s;struct mf_fixup r;
    mf_octet bytes[8];unsigned i;enum mf_status st;
    memset(bytes,0,sizeof(bytes));
    for(i=1;i<=4;++i){
        init(&f);s=sec(1,8);f.fail_write=i;
        st=f.writer.begin(f.writer.cookie,&s,1,NULL,0);
        if(st==MF_OK)st=f.writer.text(f.writer.cookie,1,0,bytes,8);
        memset(&r,0,sizeof(r));r.section=1;r.width=4;r.target=1;
        r.target_kind=MF_REF_SECTION;r.address_kind=MF_ADDRESS_A;
        if(st==MF_OK)st=f.writer.fixup(f.writer.cookie,&r);
        if(st==MF_OK)st=f.writer.finish(f.writer.cookie,1);
        else CHECK(f.writer.finish(f.writer.cookie,1)==MF_IO);
        CHECK(st==MF_IO&&f.writes==i&&f.invalids==1&&!f.valid);
        CHECK(f.length==(i-1)*80+7);clean(&f);
    }
}
int main(void)
{
    static const enum mf_status expected[]={MF_UNSUPPORTED,MF_RANGE,MF_LIMIT,MF_UNSUPPORTED,
        MF_DUPLICATE,MF_IO,MF_IO,MF_RANGE,MF_UNSUPPORTED,MF_OBJECT,MF_RANGE,
        MF_UNSUPPORTED,MF_UNDEFINED,MF_RANGE,MF_RANGE,MF_IO,MF_IO,MF_SOURCE,MF_SOURCE};
    unsigned i;good_deck();for(i=0;i<sizeof(expected)/sizeof(expected[0]);++i)failure(i,expected[i]);
    edges();definitions();relocation_failures();short_writes();
    printf("object: %u checks passed\n",checks);return 0;
}
