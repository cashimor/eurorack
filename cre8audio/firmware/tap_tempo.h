/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk. */
#ifndef CELLZ_TAP_TEMPO_H
#define CELLZ_TAP_TEMPO_H
#include <stdint.h>

typedef struct {
    uint32_t period_us, phase_us;
    uint32_t changed_ms, pressed_ms, last_tap_ms;
    uint32_t intervals[4];
    uint8_t raw, down, held, taps, count, cursor;
} TapVoice;
typedef struct { uint32_t last_ms; TapVoice voice[2]; } TapTempo;

void harmony_tempo_init(TapTempo *state, uint32_t now);
uint32_t harmony_tempo_update(TapTempo *state, uint32_t now, uint32_t buttons);
uint32_t harmony_tempo_snap(uint32_t requested_us, uint32_t other_us);
#endif
