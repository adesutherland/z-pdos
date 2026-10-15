/* SPDX-License-Identifier: MIT; SHA-256 as specified by FIPS 180-4, section 6.2.
 * Supplied input/output only, uint32 arithmetic, no allocator or host services. */
#include <limits.h>
#include "twospace_digest.h"
#if UINT_MAX != 0xffffffffU
#error SHA256_requires_32_bit_unsigned_int
#endif
static unsigned int rotate(unsigned int n,unsigned int b)
{return (n>>b)|(n<<(32U-b));}
int TJJHASH(const unsigned char *p,unsigned int n,unsigned char out[32])
{
 static const unsigned int k[64]={
 0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
 0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
 0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
 0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
 0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
 0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
 0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
 0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
 unsigned int h[8],v[8],w[64],block,i,j,at,total,a,b,t1,t2,x;
 if((!p&&n)||!out||n>1048576U)return 8;
 h[0]=0x6a09e667U;h[1]=0xbb67ae85U;h[2]=0x3c6ef372U;h[3]=0xa54ff53aU;
 h[4]=0x510e527fU;h[5]=0x9b05688cU;h[6]=0x1f83d9abU;h[7]=0x5be0cd19U;
 total=(n+72U)/64U;
 for(block=0U;block<total;++block){
  for(i=0U;i<16U;++i){x=0U;for(j=0U;j<4U;++j){at=64U*block+4U*i+j;
    a=at<n?p[at]:at==n?0x80U:at>=total*64U-4U?(n<<3)>>(8U*(total*64U-1U-at))&255U:0U;x=(x<<8)|a;}w[i]=x;}
  for(i=16U;i<64U;++i){a=w[i-15U];b=w[i-2U];w[i]=w[i-16U]+(rotate(a,7U)^rotate(a,18U)^(a>>3))+w[i-7U]+(rotate(b,17U)^rotate(b,19U)^(b>>10));}
  for(i=0U;i<8U;++i)v[i]=h[i];
  for(i=0U;i<64U;++i){a=v[4];b=v[0];
   t1=v[7]+(rotate(a,6U)^rotate(a,11U)^rotate(a,25U))+((a&v[5])^(~a&v[6]))+k[i]+w[i];
   t2=(rotate(b,2U)^rotate(b,13U)^rotate(b,22U))+((b&v[1])^(b&v[2])^(v[1]&v[2]));
   for(j=7U;j>0U;--j)v[j]=v[j-1U];v[4]+=t1;v[0]=t1+t2;}
  for(i=0U;i<8U;++i)h[i]+=v[i];
 }
 for(i=0U;i<32U;++i)out[i]=(unsigned char)(h[i/4U]>>(24U-8U*(i%4U)));return 0;
}
