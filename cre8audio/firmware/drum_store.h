/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk. */
#ifndef CELLZ_DRUM_STORE_H
#define CELLZ_DRUM_STORE_H
#include <stdint.h>
typedef struct {
    void *context;
    int (*erase)(void *,uint32_t);
    int (*program)(void *,uint32_t,uint32_t,uint32_t);
} DrumStoreIO;
int drum_store_load(const uint8_t *a,const uint8_t *b,uint16_t *patterns,uint32_t *sequence);
int drum_store_save(const uint8_t *a,const uint8_t *b,const uint16_t *patterns,const DrumStoreIO *io);
#endif
