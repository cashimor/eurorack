/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk.
 * Two-page append journal, CRC32 and commit word last. No direct hardware access.
 * Never erases the page holding the most recent complete record. */
#include "drum_store.h"
#define RECORD 36u
#define SLOTS (1024u/RECORD)
#define MAGIC 0x314d5244u
#define COMMIT 0x454e4f44u
static uint32_t get32(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put32(uint8_t *p,uint32_t v) { for(uint32_t i=0;i<4;++i) p[i]=(uint8_t)(v>>(8u*i)); }
static uint32_t crc32(const uint8_t *p,uint32_t count) {
    uint32_t crc=0xffffffffu;
    for(uint32_t i=0;i<count;++i) { crc^=p[i]; for(uint32_t bit=0;bit<8;++bit) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u); }
    return ~crc;
}
static int blank(const uint8_t *p,uint32_t count) {
    for(uint32_t i=0;i<count;++i) if(p[i]!=255u) return 0;
    return 1;
}
static int valid(const uint8_t *p) {
    return get32(p)==MAGIC && get32(p+28)==1u && get32(p+32)==COMMIT && get32(p+24)==crc32(p,24);
}
static int latest(const uint8_t *a,const uint8_t *b,uint32_t *page,uint32_t *slot) {
    uint32_t found=0,sequence=0;
    for(uint32_t pg=0;pg<2;++pg) for(uint32_t s=0;s<SLOTS;++s) {
        const uint8_t *r=(pg?b:a)+s*RECORD;
        if(valid(r) && (!found || (int32_t)(get32(r+4)-sequence)>0)) {
            found=1; sequence=get32(r+4); *page=pg; *slot=s;
        }
    }
    return (int)found;
}
int drum_store_load(const uint8_t *a,const uint8_t *b,uint16_t *patterns,uint32_t *sequence) {
    uint32_t page=0,slot=0;
    if(!latest(a,b,&page,&slot)) return 0;
    const uint8_t *r=(page?b:a)+slot*RECORD;
    for(uint32_t i=0;i<8;++i) patterns[i]=(uint16_t)(r[8+2*i]|(uint16_t)r[9+2*i]<<8);
    *sequence=get32(r+4); return 1;
}
int drum_store_save(const uint8_t *a,const uint8_t *b,const uint16_t *patterns,const DrumStoreIO *io) {
    uint32_t page=0,slot=0,sequence=1;
    int found=latest(a,b,&page,&slot);
    if(found) sequence=get32((page?b:a)+slot*RECORD+4)+1u;
    else {
        /* Do not erase or append into unidentified data on first use. A wholly
         * erased page is required; this also recovers an interrupted first save. */
        if(blank(a,1024)) page=0;
        else if(blank(b,1024)) page=1;
        else return 0;
    }
    const uint8_t *destination=page?b:a;
    for(slot=0;slot<SLOTS;++slot) if(blank(destination+slot*RECORD,RECORD)) break;
    if(slot==SLOTS) {
        page^=1u;
        if(!io->erase(io->context,page)) return 0;
        destination=page?b:a;
        if(!blank(destination,1024)) return 0;
        slot=0;
    }
    uint8_t record[RECORD];
    put32(record,MAGIC); put32(record+4,sequence);
    for(uint32_t i=0;i<8;++i) { record[8+2*i]=(uint8_t)patterns[i]; record[9+2*i]=(uint8_t)(patterns[i]>>8); }
    put32(record+24,crc32(record,24)); put32(record+28,1); put32(record+32,COMMIT);
    for(uint32_t offset=0;offset<RECORD;offset+=4) {
        if(!io->program(io->context,page,slot*RECORD+offset,get32(record+offset))) return 0;
        if(get32(destination+slot*RECORD+offset)!=get32(record+offset)) return 0;
    }
    return valid(destination+slot*RECORD);
}
