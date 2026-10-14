/* SPDX-License-Identifier: MIT; native dataset/member/concat/temp consumers. */
#include <stdio.h>
#include <string.h>
extern int D4LINK(void);
#define REQUIRE(x,n) do{if(!(x))return n;}while(0)
static int write_bytes(const char *dd,unsigned int n,unsigned int tag)
{FILE *f=fopen(dd,"wb");unsigned char data[256];unsigned int i;
 /* Maintained PDPCLIB variable-binary streams include the record RDW. */
 if(!f)return 41;data[0]=0U;data[1]=(unsigned char)(n+4U);data[2]=data[3]=0U;
 for(i=0U;i<n;++i)data[i+4U]=(unsigned char)(i*37U+tag);
 if(fwrite(data,1U,n+4U,f)!=n+4U||fclose(f))return 42;return 0;}
static int read_bytes(const char *dd,unsigned int n,unsigned int tag)
{FILE *f=fopen(dd,"rb");unsigned char data[256];unsigned int i;
 if(!f)return 43;if(fread(data,1U,n+4U,f)!=n+4U)return 44;
 if(data[0]||data[1]!=n+4U||data[2]||data[3])return 45;
 for(i=0U;i<n;++i)if(data[i+4U]!=(unsigned char)(i*37U+tag))return 45;
 if(fgetc(f)!=EOF||ferror(f)||fclose(f))return 46;return 0;}
static int sequential(const char *dd)
{FILE *f=fopen(dd,"rb");unsigned char data[256];unsigned int i;
 if(!f||fread(data,1U,sizeof data,f)!=sizeof data)return 47;
 for(i=0U;i<256U;++i)if(data[i]!=(unsigned char)i)return 48;
 return fgetc(f)!=EOF||ferror(f)||fclose(f)?49:0;}
int main(int argc,char **argv)
{
 const char *mode=argc>1?argv[1]:"";FILE *f;unsigned char data[256];unsigned int i;
 if(!strcmp(mode,"write")){
  f=fopen("dd:PSOUT","wb");REQUIRE(f,51);for(i=0U;i<256U;++i)data[i]=(unsigned char)i;
  REQUIRE(fwrite(data,1U,256U,f)==256U&&!fclose(f),52);
  REQUIRE(!write_bytes("dd:MEMOUT",13U,11U)&&!write_bytes("dd:SECOND",13U,93U),53);
  f=fopen("dd:ZERO","wb");REQUIRE(f&&!fclose(f),54);puts("DATASETS WRITE PASS");return 0;
 }
 if(!strcmp(mode,"read")){
  REQUIRE(!sequential("dd:PSIN")&&!read_bytes("dd:MEMIN",13U,11U)&&!read_bytes("dd:SECOND",13U,93U),55);
  f=fopen("dd:ZERO","rb");REQUIRE(f&&fgetc(f)==EOF&&!ferror(f)&&!fclose(f),56);
  REQUIRE(fopen("dd:MEMIN(OTHER)","rb")==NULL,57);puts("DATASETS READ PASS");return 0;
 }
 if(!strcmp(mode,"replace")){
  REQUIRE(!write_bytes("dd:MEMOUT",13U,177U)&&!read_bytes("dd:MEMIN",13U,177U)&&!read_bytes("dd:SECOND",13U,93U),58);
  puts("MEMBER REPLACE PASS");return 0;
 }
 if(!strcmp(mode,"concat")){
  f=fopen("dd:JOIN","rb");REQUIRE(f&&fread(data,1U,128U,f)==128U,59);
  for(i=0U;i<32U;++i)REQUIRE(data[i]==(unsigned char)(64U+i),60);
  for(i=0U;i<64U;++i)REQUIRE(data[32U+i]==(unsigned char)i,61);
  for(i=0U;i<32U;++i)REQUIRE(data[96U+i]==(unsigned char)(96U+i),62);
  REQUIRE(fgetc(f)==EOF&&!ferror(f)&&!fclose(f),63);
  f=fopen("dd:JOIN","rb");REQUIRE(f&&fgetc(f)==64&&!fclose(f),64);puts("CONCAT PASS");return 0;
 }
 if(!strcmp(mode,"tempwrite")){
  f=fopen("dd:WORK","wb");REQUIRE(f,65);for(i=0U;i<256U;++i)data[i]=(unsigned char)i;
  REQUIRE(fwrite(data,1U,256U,f)==256U&&!fclose(f),66);puts("TEMP WRITE PASS");return 0;
 }
 if(!strcmp(mode,"tempkeep")){
  f=fopen("dd:WORK","rb");REQUIRE(f&&fread(data,1U,128U,f)==128U&&!D4LINK(),67);
  REQUIRE(fread(data+128U,1U,128U,f)==128U,68);for(i=0U;i<256U;++i)REQUIRE(data[i]==(unsigned char)i,69);
  REQUIRE(fgetc(f)==EOF&&!ferror(f)&&!fclose(f),70);puts("TEMP KEEP PASS");return 0;
 }
 if(!strcmp(mode,"tempdelete")){REQUIRE(!sequential("dd:WORK"),71);puts("TEMP DELETE PASS");return 0;}
 if(!strcmp(mode,"tempreplace")){
  f=fopen("dd:WORK","wb");REQUIRE(f,84);for(i=0U;i<256U;++i)data[i]=(unsigned char)(i^0xa5U);
  REQUIRE(fwrite(data,1U,sizeof data,f)==sizeof data&&!fclose(f),85);
  f=fopen("dd:TREAD","rb");REQUIRE(f&&fread(data,1U,sizeof data,f)==sizeof data,86);
  for(i=0U;i<256U;++i)REQUIRE(data[i]==(unsigned char)(i^0xa5U),87);
  REQUIRE(fgetc(f)==EOF&&!ferror(f)&&!fclose(f),88);puts("TEMP REPLACE PASS");return 0;
 }
 if(!strcmp(mode,"tempempty")){f=fopen("dd:WORK","wb");REQUIRE(f&&!fclose(f),89);puts("TEMP EMPTY WRITE PASS");return 0;}
 if(!strcmp(mode,"tempzero")){f=fopen("dd:WORK","rb");REQUIRE(f&&fgetc(f)==EOF&&!ferror(f)&&!fclose(f),90);puts("TEMP EMPTY READ PASS");return 0;}
 if(!strcmp(mode,"quota")){
  f=fopen("dd:WORK","wb");REQUIRE(f,72);memset(data,0xa5,sizeof data);
  /* Genuine native WRITE exceeds the declared temporary capacity. */
  fwrite(data,1U,sizeof data,f);fclose(f);return 73;
 }
 if(!strcmp(mode,"fault")){
  f=fopen("dd:PSOUT","wb");REQUIRE(f,74);memset(data,0xd3,sizeof data);REQUIRE(fwrite(data,1U,sizeof data,f)==sizeof data,75);
  f=fopen("dd:WORK","wb");REQUIRE(f,76);REQUIRE(fwrite(data,1U,sizeof data,f)==sizeof data,77);
  {volatile unsigned int *bad=(volatile unsigned int *)0xfffffff0U;return (int)*bad;}
 }
 return 78;
}
