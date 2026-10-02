/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent host checker for fixtures/bootstrap.asm. No assembler helpers.
 * Format facts: IBM z/VM 7.4 OBJSTMT; expected bytes calculated from fixture.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do { ++checks; if (!(x)) { fprintf(stderr,"deck card %lu: %s\n",cards,#x); exit(1); } } while(0)
static unsigned checks;
static unsigned long cards;
struct identity { unsigned id, kind; unsigned long length; };
static struct identity ids[16];
static unsigned id_count;
static unsigned long number(const unsigned char *p, unsigned n)
{
    unsigned long x=0;while(n--)x=x*256UL+*p++;return x;
}
static unsigned identity(unsigned id)
{
    unsigned i;for(i=0;i<id_count;++i)if(ids[i].id==id)return ids[i].kind;
    return 0;
}
static unsigned name_kind(const unsigned char *p)
{
    static const unsigned char names[4][8]={
        {0xc3,0xd6,0xc4,0xc5,0x40,0x40,0x40,0x40},
        {0xe2,0xc5,0xc3,0xd6,0xd5,0xc4,0x40,0x40},
        {0xc5,0xe7,0xe3,0x40,0x40,0x40,0x40,0x40},
        {0xe2,0xe3,0xc1,0xd9,0xe3,0x40,0x40,0x40}};
    unsigned i;for(i=0;i<4;++i)if(!memcmp(p,names[i],8))return i+1;
    return 0;
}
int main(int argc,char **argv)
{
    static const unsigned char expected_code[16]={
        0x18,0x34,0x18,0x12,0,0,0,2,0,0,0,0,0,0,0,0};
    static const unsigned char expected_second[4]={0,0,0,2};
    unsigned char card[80],text[2][24],present[2][24];
    FILE *input;size_t got;
    unsigned ns=0,er=0,ld=0,ended=0,body=0,relocations=0,reloc_seen=0;
    unsigned export_id=0; unsigned long export_offset=0;
    unsigned size,i,k,id,type,section,target,flag,continued=0;
    unsigned long off,len;
    memset(text,0,sizeof(text));memset(present,0,sizeof(present));
    REQUIRE(argc==2);input=fopen(argv[1],"rb");REQUIRE(input!=NULL);
    while((got=fread(card,1,80,input))!=0){
        ++cards; REQUIRE(got==80 && !ended && card[0]==2);
        if(card[1]==0xc5&&card[2]==0xe2&&card[3]==0xc4){
            REQUIRE(!body);size=(unsigned)number(card+10,2);
            REQUIRE(size>0 && size<=48 && size%16==0);
            id=(unsigned)number(card+14,2);
            for(i=16;i<16+size;i+=16){
                k=name_kind(card+i);REQUIRE(k!=0);type=card[i+8];
                if(type==1){
                    REQUIRE(k==4 && ld==0);++ld;
                    export_offset=number(card+i+9,3);
                    export_id=(unsigned)number(card+i+13,3);
                }else{
                    REQUIRE(id!=0 && identity(id)==0 && id_count<16);
                    ids[id_count].id=id;ids[id_count].kind=k;
                    ids[id_count].length=number(card+i+13,3);++id_count;++id;
                    if(type==0){
                        REQUIRE(k==1||k==2);++ns;
                        REQUIRE(number(card+i+9,3)==0);
                        if(k==1) REQUIRE(card[i+12]==6 && number(card+i+13,3)==24);
                        else REQUIRE(card[i+12]==1 && number(card+i+13,3)==4);
                    }else{REQUIRE(type==2&&k==3&&er==0);++er;}
                }
            }
        }else if(card[1]==0xe3&&card[2]==0xe7&&card[3]==0xe3){
            body=1;size=(unsigned)number(card+10,2);REQUIRE(size>0&&size<=56);
            k=identity((unsigned)number(card+14,2));REQUIRE(k==1||k==2);
            off=number(card+5,3);len=k==1?24:4;
            REQUIRE(off<=len && size<=len-off);
            for(i=0;i<size;++i){REQUIRE(!present[k-1][off+i]);
                present[k-1][off+i]=1;text[k-1][off+i]=card[16+i];}
        }else if(card[1]==0xd9&&card[2]==0xd3&&card[3]==0xc4){
            body=1;size=(unsigned)number(card+10,2);REQUIRE(size>0&&size<=56);
            i=0;continued=0;target=section=0;
            while(i<size){
                if(!continued){REQUIRE(size-i>=8);
                    target=identity((unsigned)number(card+16+i,2));
                    section=identity((unsigned)number(card+18+i,2));i+=4;
                }else REQUIRE(size-i>=4);
                flag=card[16+i];off=number(card+17+i,3);i+=4;
                continued=flag&1;flag&=254;
                REQUIRE(section==1||section==2);REQUIRE(target>=1&&target<=3);
                REQUIRE(flag==0x0c||flag==0x1c);
                k=99;
                if(section==1&&target==1&&flag==0x0c&&off==4)k=0;
                if(section==1&&target==3&&flag==0x1c&&off==8)k=1;
                if(section==1&&target==3&&flag==0x0c&&off==12)k=2;
                if(section==2&&target==1&&flag==0x0c&&off==0)k=3;
                REQUIRE(k<4 && !(reloc_seen&(1U<<k)));
                reloc_seen|=1U<<k;++relocations;
            }
            REQUIRE(!continued);
        }else{
            REQUIRE(card[1]==0xc5&&card[2]==0xd5&&card[3]==0xc4);
            REQUIRE(identity((unsigned)number(card+14,2))==1 && number(card+5,3)==2);
            ended=1;
        }
    }
    REQUIRE(!ferror(input));REQUIRE(fclose(input)==0);
    REQUIRE(cards>0&&ended&&ns==2&&er==1&&ld==1&&id_count==3);
    REQUIRE(identity(export_id)==1&&export_offset==2);
    REQUIRE(relocations==4&&reloc_seen==15);
    for(i=0;i<24;++i){
        REQUIRE(present[0][i]==(i<16?1:0));
        if(i<16)REQUIRE(text[0][i]==expected_code[i]);
    }
    for(i=0;i<4;++i)REQUIRE(present[1][i]&&text[1][i]==expected_second[i]);
    printf("deck: %u independent checks passed (%lu cards)\n",checks,cards);return 0;
}
