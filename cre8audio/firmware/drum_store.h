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
/* data contains 25 uint32_t words: eight steps, eight accents, eight triplets,
 * then length (16 or 32). Load widens D1/D2 masks, zero-fills missing modifiers
 * and sets length 16. Save writes D3 only; hidden upper halves are retained. */
int drum_store_load(const uint8_t *a,const uint8_t *b,uint32_t *patterns,uint32_t *sequence);
int drum_store_save(const uint8_t *a,const uint8_t *b,const uint32_t *patterns,const DrumStoreIO *io);
#endif
