/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk.
 * Two-page CRC journal. D3 reads D1/D2 and writes a commit-last 32-step D3 record.
 * Migration erases only the page NOT holding the latest complete record. */
#include "drum_store.h"
#define RECORD 120u
#define SLOTS (1024u/RECORD)
#define MAGIC1 0x314d5244u
#define MAGIC2 0x324d5244u
#define MAGIC3 0x334d5244u
#define COMMIT 0x454e4f44u
static uint32_t get32(const uint8_t *p) {return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put32(uint8_t *p,uint32_t v) {for(uint32_t i=0;i<4;++i)p[i]=(uint8_t)(v>>(8u*i));}
static uint32_t crc32(const uint8_t *p,uint32_t count) {
    uint32_t crc=0xffffffffu;
    for(uint32_t i=0;i<count;++i) {crc^=p[i];for(uint32_t b=0;b<8;++b)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
    return ~crc;
}
static int blank(const uint8_t *p,uint32_t count) {for(uint32_t i=0;i<count;++i)if(p[i]!=255u)return 0;return 1;}
static uint32_t version(const uint8_t *p,uint32_t remaining) {
    uint32_t magic=get32(p),v=magic==MAGIC1?1u:(magic==MAGIC2?2u:(magic==MAGIC3?3u:0u));
    uint32_t crc_at=v==1u?24u:(v==2u?56u:108u);
    if(!v || remaining<crc_at+12u || get32(p+crc_at+4u)!=v || get32(p+crc_at+8u)!=COMMIT || get32(p+crc_at)!=crc32(p,crc_at))return 0;
    if(v==3u && get32(p+104u)!=16u && get32(p+104u)!=32u)return 0;
    return v;
}
static int latest(const uint8_t *a,const uint8_t *b,uint32_t *page,uint32_t *offset,uint32_t *format) {
    uint32_t found=0,sequence=0;
    /* All three record formats share four-byte alignment. */
    for(uint32_t pg=0;pg<2;++pg)for(uint32_t at=0;at<=1024u-36u;at+=4u) {
        const uint8_t *p=(pg?b:a)+at;
        uint32_t v=version(p,1024u-at);
        if(v && (!found || (int32_t)(get32(p+4)-sequence)>0)) {
            found=1;sequence=get32(p+4);*page=pg;*offset=at;*format=v;
        }
    }
    return (int)found;
}
int drum_store_load(const uint8_t *a,const uint8_t *b,uint32_t *data,uint32_t *sequence) {
    uint32_t page=0,offset=0,format=0;
    if(!latest(a,b,&page,&offset,&format))return 0;
    const uint8_t *p=(page?b:a)+offset;
    for(uint32_t i=0;i<24u;++i)data[i]=format==3u?get32(p+8+4*i):
        ((format==1u && i>=8u)?0u:(uint32_t)(p[8+2*i]|(uint32_t)p[9+2*i]<<8));
    data[24]=format==3u?get32(p+104):16u;
    *sequence=get32(p+4);return 1;
}
int drum_store_save(const uint8_t *a,const uint8_t *b,const uint32_t *data,const DrumStoreIO *io) {
    if(data[24]!=16u && data[24]!=32u)return 0;
    uint32_t page=0,offset=0,format=0,sequence=1;
    int found=latest(a,b,&page,&offset,&format);
    if(found)sequence=get32((page?b:a)+offset+4)+1u;
    else {
        if(blank(a,1024))page=0;
        else if(blank(b,1024))page=1;
        else return 0;
    }
    const uint8_t *destination=page?b:a;
    uint32_t slot=0;
    if(!found || format==3u)for(;slot<SLOTS;++slot)if(blank(destination+slot*RECORD,RECORD))break;
    if((found && format!=3u) || slot==SLOTS) {
        page^=1u;
        if(!io->erase(io->context,page))return 0;
        destination=page?b:a;
        if(!blank(destination,1024))return 0;
        slot=0;
    }
    uint8_t record[RECORD];put32(record,MAGIC3);put32(record+4,sequence);
    for(uint32_t i=0;i<25;++i)put32(record+8+4*i,data[i]);
    put32(record+108,crc32(record,108));put32(record+112,3);put32(record+116,COMMIT);
    for(uint32_t at=0;at<RECORD;at+=4u) {
        if(!io->program(io->context,page,slot*RECORD+at,get32(record+at)))return 0;
        if(get32(destination+slot*RECORD+at)!=get32(record+at))return 0;
    }
    return version(destination+slot*RECORD,RECORD)==3u;
}
