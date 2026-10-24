/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk.
 * Pure tap/clock engine. Unsigned elapsed times tolerate millisecond wraparound.
 * No globals, allocation, hardware access, floating point, or persistent writes. */
#include "tap_tempo.h"

#define MIN_TAP_MS 150u
#define MAX_TAP_MS 4000u
#define HOLD_MS 800u
#define DEBOUNCE_MS 20u
#define MIN_PERIOD_US 150000u
#define MAX_PERIOD_US 32000000u

static uint32_t distance(uint32_t a, uint32_t b) {
    return a > b ? a - b : b - a;
}

/* Deliberate vocabulary: integer multiples through eight, plus the familiar
 * 3:2 and 4:3 polyrhythms. No 5:7, 7:8, 24:7, etc. Ratio is step duration,
 * so the corresponding tempo ratio is its inverse. */
uint32_t harmony_tempo_snap(uint32_t requested_us, uint32_t other_us) {
    static const uint8_t ratios[][2] = {
        {1,1}, {1,2}, {2,1}, {1,3}, {3,1}, {1,4}, {4,1},
        {1,5}, {5,1}, {1,6}, {6,1}, {1,7}, {7,1}, {1,8}, {8,1},
        {2,3}, {3,2}, {3,4}, {4,3}
    };
    if (!other_us) return requested_us;
    uint32_t best = other_us, error = 0xffffffffu;
    for (uint32_t i = 0; i < sizeof(ratios)/sizeof(ratios[0]); ++i) {
        /* Maximum multiplication: 32,000,000 * 8, safely below UINT32_MAX. */
        uint32_t candidate = (other_us * ratios[i][0] + ratios[i][1]/2u) / ratios[i][1];
        if (candidate < MIN_PERIOD_US || candidate > MAX_PERIOD_US) continue;
        uint32_t difference = distance(candidate, requested_us);
        if (difference < error) { best = candidate; error = difference; }
    }
    return best;
}

void harmony_tempo_init(TapTempo *state, uint32_t now) {
    state->last_ms = now;
    for (uint32_t i = 0; i < 2; ++i) {
        TapVoice *v = &state->voice[i];
        v->period_us = v->phase_us = 0;
        v->changed_ms = v->pressed_ms = v->last_tap_ms = now;
        for (uint32_t j = 0; j < 4; ++j) v->intervals[j] = 0;
        v->raw = v->down = v->held = v->taps = v->count = v->cursor = 0;
    }
}

static uint32_t tap(TapTempo *state, uint32_t channel, uint32_t when) {
    TapVoice *v = &state->voice[channel];
    uint32_t elapsed = when - v->last_tap_ms;
    if (!v->taps || elapsed > MAX_TAP_MS) {
        v->last_tap_ms = when;
        v->taps = 1; v->count = v->cursor = 0;
        return 0;
    }
    if (elapsed < MIN_TAP_MS) return 0;
    v->last_tap_ms = when;
    if (v->count) {
        uint32_t sum = 0;
        for (uint32_t j = 0; j < v->count; ++j) sum += v->intervals[j];
        uint32_t mean = sum / v->count;
        /* A substantially different tap restarts measurement with the latest
         * interval. A running clock keeps its old tempo until reconfirmed. */
        if (distance(elapsed, mean) > mean/4u) {
            v->count = v->cursor = 0; v->taps = 1;
        }
    }
    v->intervals[v->cursor] = elapsed;
    v->cursor = (v->cursor + 1u) & 3u;
    if (v->count < 4) ++v->count;
    if (v->taps < 3) ++v->taps;
    if (v->taps < 3) return 0;
    uint32_t sum = 0;
    for (uint32_t j = 0; j < v->count; ++j) sum += v->intervals[j];
    uint32_t requested = (sum * 1000u + v->count/2u) / v->count;
    v->period_us = harmony_tempo_snap(requested, state->voice[channel ^ 1u].period_us);
    return 1u << channel;
}

uint32_t harmony_tempo_update(TapTempo *state, uint32_t now, uint32_t buttons) {
    uint32_t elapsed = now - state->last_ms, due = 0, retapped = 0;
    state->last_ms = now;
    /* Account for genuine old-clock edges before applying any tap changes.
     * Otherwise a tap exactly on each edge could continually cancel playback. */
    for (uint32_t i = 0; i < 2; ++i) {
        TapVoice *v = &state->voice[i];
        if (!v->period_us) continue;
        /* Fractional milliseconds accumulate; stalls never produce a burst. */
        if (elapsed > MAX_PERIOD_US/1000u) { v->phase_us = 0; due |= 1u << i; }
        else {
            v->phase_us += elapsed * 1000u;
            if (v->phase_us >= v->period_us) {
                v->phase_us %= v->period_us; due |= 1u << i;
            }
        }
    }
    for (uint32_t i = 0; i < 2; ++i) {
        TapVoice *v = &state->voice[i];
        uint32_t raw = (buttons >> i) & 1u;
        if (raw != v->raw) { v->raw = raw; v->changed_ms = now; }
        if (v->down != raw && now - v->changed_ms >= DEBOUNCE_MS) {
            v->down = raw;
            if (raw) { v->pressed_ms = v->changed_ms; v->held = 0; }
            else if (!v->held && v->changed_ms - v->pressed_ms < HOLD_MS) {
                /* Commit on release: a held button never supplies a tap. */
                uint32_t accepted = tap(state, i, v->pressed_ms);
                retapped |= accepted;
            }
        }
        if (v->down && raw && !v->held && now - v->pressed_ms >= HOLD_MS) {
            v->period_us = v->phase_us = 0;
            v->taps = v->count = v->cursor = 0; v->held = 1;
        }
    }
    if (retapped) {
        /* A confirmed tempo starts one shared cycle for all running voices.
         * Do not synthesize a step here: the old clock may just have stepped.
         * Each voice waits one complete period from this common boundary.
         * Already-due old-clock edges above are preserved, never duplicated. */
        for (uint32_t i = 0; i < 2; ++i) state->voice[i].phase_us = 0;
    }
    for (uint32_t i = 0; i < 2; ++i) {
        if (!state->voice[i].period_us) due &= ~(1u << i);
    }
    return due;
}
