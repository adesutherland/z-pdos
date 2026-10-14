/* SPDX-License-Identifier: MIT; portable supplied-storage device semantics.
 * IBM GA21-9124-1 pp.48/62-69 and A24-3312-7 pp.35-41 describe commands
 * and status. Native submission, ownership cleanup and U spans remain in K.
 */
#include "twospace_batch.h"
static unsigned int tbc_word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void tbc_clear(TBCRESULT *r)
{unsigned int i;r->actual=r->device_status=r->channel_status=r->sense_bytes=r->eof=0U;for(i=0U;i<TBC_SENSE_BYTES;++i)r->sense[i]=0U;}
int TBCBUILD(TSCSTATE *s,unsigned int kind,unsigned int command,unsigned int bytes)
{
    if(kind!=TBC_READER&&kind!=TBC_PRINTER)return TBC_BAD;
    if(command==0xe4U){if(bytes!=7U)return TBC_BAD;}
    else if(command==4U){if(bytes!=TBC_SENSE_BYTES)return TBC_BAD;}
    else if(kind==TBC_READER){if(command!=2U||bytes!=TBC_CARD_BYTES)return TBC_BAD;}
    else if((command!=9U&&command!=0x89U)||!bytes||bytes>TBC_PRINT_BYTES)return TBC_BAD;
    /* The common builder already accepts 09; 89 has the same data transfer
     * shape and differs only in the printer carriage-control command. */
    if(TSCBUILDCONSCMD(s,command==0x89U?9U:command,bytes,
                      kind==TBC_READER||command==4U||command==0xe4U))return TBC_BAD;
    if(command==0x89U)s->aperture[s->region_real+TSC_CCW_OFFSET]=0x89U;
    return TBC_OK;
}
int TBCCHECK(const TSCSTATE *s,const unsigned char *irb,unsigned int kind,
             unsigned int command,unsigned int count,TBCRESULT *r)
{
    unsigned int residual,status;
    if(!s||!s->aperture||!irb||!r||(kind!=TBC_READER&&kind!=TBC_PRINTER)||!count)return TBC_BAD;
    tbc_clear(r);r->device_status=irb[8U];r->channel_status=irb[9U];
    residual=((unsigned int)irb[10U]<<8)|irb[11U];status=irb[8U]&0x7fU;
    if(irb[9U]||residual>count||tbc_word(irb+4U)!=s->region_real+TSC_CCW_OFFSET+8U)return TBC_IO;
    if(kind==TBC_READER&&command==2U&&count==TBC_CARD_BYTES&&status==0x0dU&&residual==count){r->eof=1U;return TBC_EOF;}
    if(status!=0x0cU)return TBC_IO;
    if(command==4U){if(count!=TBC_SENSE_BYTES)return TBC_BAD;r->actual=count-residual;return TBC_OK;}
    if(command==0xe4U){if(count!=7U)return TBC_BAD;}
    else if(kind==TBC_READER){if(command!=2U||count!=TBC_CARD_BYTES)return TBC_BAD;}
    else if((command!=9U&&command!=0x89U)||count>TBC_PRINT_BYTES)return TBC_BAD;
    if(residual)return TBC_IO;
    r->actual=count;return TBC_OK;
}
int TBCOPEN(TBCPOOL *p,unsigned int owner,unsigned int kind,unsigned int address,unsigned int *handle)
{
    TBCENDPOINT *e;unsigned int i;
    if(!p||!owner||!handle||(kind!=TBC_READER&&kind!=TBC_PRINTER)||!address||address>65535U)return TBC_BAD;
    e=&p->endpoint[kind-1U];if(e->handle)return TBC_BUSY;
    for(i=0U;i<2U;++i)if(p->endpoint[i].handle&&p->endpoint[i].address==address)return TBC_BUSY;
    if(p->next_handle==0xffffffffU)return TBC_BAD;
    e->owner=owner;e->handle=++p->next_handle;e->kind=kind;e->address=address;e->ssid=0U;tbc_clear(&e->result);
    *handle=e->handle;return TBC_OK;
}
TBCENDPOINT *TBCFIND(TBCPOOL *p,unsigned int owner,unsigned int handle)
{unsigned int i;if(!p||!owner||!handle)return 0;for(i=0U;i<2U;++i)if(p->endpoint[i].handle==handle&&p->endpoint[i].owner==owner)return &p->endpoint[i];return 0;}
int TBCCLOSE(TBCPOOL *p,unsigned int owner,unsigned int handle)
{TBCENDPOINT *e=TBCFIND(p,owner,handle);if(!e)return TBC_BAD;e->owner=e->handle=e->address=e->ssid=0U;return TBC_OK;}
