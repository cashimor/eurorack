/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk. */
#ifndef CELLZ_DRUM_ENGINE_H
#define CELLZ_DRUM_ENGINE_H
#include <stdint.h>
#define DRUM_PATTERN_WORDS 25u

typedef struct {
    uint32_t now;
    uint16_t touches, tempo, swing;
    uint8_t buttons, alternate, clock_edge;
} DrumInput;
typedef struct {
    uint32_t started_ms, period_us;
    uint8_t active, next_hit, step, bank, accent;
} DrumTriplet;
typedef struct {
    /* Persistent prefix: 24 masks and the shared playback length, all uint32_t. */
    uint32_t patterns[2][4], accents[2][4], triplets[2][4], length;
    uint32_t last_ms, phase_us, last_edge_ms, external_period_us;
    uint32_t pending_at_ms, pulse_until_ms[4], touch_changed_ms, button_changed_ms;
    uint32_t pressed_ms[2], combo_ms, last_step_ms, feedback_until_ms;
    DrumTriplet group[4];
    uint16_t raw_touches, touches, tempo_adc, swing_adc;
    uint8_t instrument, selected_bank, active_bank, next_step, step, pending_step;
    uint8_t external, have_edge, pending, raw_buttons, buttons, combo, combo_saved;
    uint8_t dirty, save_requested, saved_status, has_step, outputs, controls_ready;
    uint8_t accent_outputs, shift_used, combo_edited;
    uint8_t edit_page, swing_pickup, length_gesture;
    uint16_t swing_anchor, last_swing_raw;
} Drum;
void drum_init(Drum *d, uint32_t now);
uint32_t drum_update(Drum *d, const DrumInput *in);
uint32_t drum_interval_us(const Drum *d);
uint32_t drum_led_mask(const Drum *d, uint32_t now);
void drum_expire(Drum *d, uint32_t now);
void drum_after_save(Drum *d, uint32_t now, uint32_t success);
#endif
