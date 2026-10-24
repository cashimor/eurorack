/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk. */
/* Cellz Harmony prototype 2. Freestanding Cortex-M3 application extension.
 * Integration addresses are specific to the pinned official V0.0.3.0 image.
 * No heap allocation, floating point, persistent writes, or bootloader changes.
 */
#include <stdint.h>
#include "tap_tempo.h"

#define SCALE_COUNT 14u
#define LAYOUT_COUNT 16u
#define ADC_RANGE 4096u
#define HYSTERESIS 24u
#define REG8(a) (*(volatile uint8_t *)(a))
#define REG16(a) (*(volatile uint16_t *)(a))
#define REG32(a) (*(volatile uint32_t *)(a))

typedef struct {
    uint8_t size;
    uint8_t scale[7];
    uint8_t third, fifth, seventh; /* seventh=0 means triad */
} Harmony;

/* Order across the decoded pot range. Physical rotation direction is untested.
 * Melodic minor is the ascending form.
 */
static const Harmony harmonies[SCALE_COUNT] = {
    {7, {0,2,4,5,7,9,11}, 4,7,0},  /* Major */
    {7, {0,2,4,5,7,9,11}, 4,7,11}, /* Major7 */
    {7, {0,2,3,5,7,8,10}, 3,7,0},  /* Minor / Aeolian */
    {7, {0,2,3,5,7,8,10}, 3,7,10}, /* Minor7 */
    {7, {0,2,3,5,7,8,11}, 3,7,0},  /* Harmonic minor */
    {7, {0,2,3,5,7,9,11}, 3,7,0},  /* Melodic minor */
    {5, {0,2,4,7,9,0,0}, 4,7,0},   /* Major pentatonic */
    {5, {0,3,5,7,10,0,0},3,7,0},   /* Minor pentatonic */
    {6, {0,3,5,6,7,10,0},3,7,0},   /* Minor blues: added flat fifth */
    {7, {0,2,3,5,7,9,10},3,7,0},   /* Dorian */
    {7, {0,1,3,5,7,8,10},3,7,0},   /* Phrygian */
    {7, {0,2,4,6,7,9,11},4,7,0},   /* Lydian */
    {7, {0,2,4,5,7,9,10},4,7,0},   /* Mixolydian */
    {7, {0,1,3,5,6,8,10},3,6,0},   /* Locrian: diminished triad */
};

/* Read from the original pitch conversion in emulation, semitones 0..24.
 * These retain the vendor's output scaling and two-octave channel offset.
 */
static const uint16_t dac_codes[2][25] = {
    {819,853,887,921,955,989,1023,1057,1091,1126,1160,1194,1228,
     1262,1296,1330,1364,1399,1433,1467,1501,1535,1569,1603,1637},
    {1638,1672,1706,1740,1774,1808,1842,1876,1910,1945,1979,2013,2047,
     2081,2115,2149,2183,2218,2252,2286,2320,2354,2388,2422,2456}
};

/* Reuse only the old ADC-change tracking bytes: their original reader is
 * replaced, and the Tune handler is disabled. No new RAM allocation is needed.
 */
typedef struct { uint8_t harmony, layout, ready, reserved; } State;
#define STATE ((volatile State *)0x2000026cu)
#define LOWER ((volatile uint16_t *)0x200006fcu)
#define UPPER ((volatile uint16_t *)0x2000071cu)

uint32_t harmony_bin(uint32_t adc, uint32_t count, uint32_t previous) {
    if (adc > 4095u) adc = 4095u;
    uint32_t candidate = (adc * count) >> 12;
    if (previous >= count) return candidate;
    if (candidate > previous) {
        uint32_t edge = ((previous + 1u) * ADC_RANGE + count - 1u) / count;
        if (adc < edge + HYSTERESIS) return previous;
    } else if (candidate < previous) {
        uint32_t edge = (previous * ADC_RANGE + count - 1u) / count;
        if (adc + HYSTERESIS >= edge) return previous;
    }
    return candidate;
}

/* Generate semitone offsets, independent of hardware. 16 distinct pairs.
 * Four strides and four rotations permute the same useful harmonic material.
 */
void harmony_generate(uint32_t setting, uint32_t layout,
                      uint16_t lower[16], uint16_t upper[16]) {
    static const uint8_t strides[4] = {1,5,11,15};
    if (setting >= SCALE_COUNT) setting = 0;
    layout &= 15u;
    const Harmony *h = &harmonies[setting];
    uint32_t pool = 2u * h->size + 1u;
    for (uint32_t pad = 0; pad < 16; ++pad) {
        uint32_t index = (pad * strides[layout >> 2] + 4u * (layout & 3u)) & 15u;
        uint32_t degree = index % pool;
        upper[pad] = (uint16_t)(12u * (degree / h->size) + h->scale[degree % h->size]);
        if (index < 4) lower[pad] = 0;
        else if (index < 8) lower[pad] = 12;
        else if (index < 10) lower[pad] = h->fifth;
        else if (index < 12) lower[pad] = 12u + h->fifth;
        else if (index == 12) lower[pad] = h->third;
        else if (index == 13) lower[pad] = 12u + h->third;
        else if (index == 14) lower[pad] = 24;
        else lower[pad] = h->seventh ? h->seventh : h->third;
    }
}

#ifndef CELLZ_HOST_TEST
/* The pinned runtime reserves RAM through 0x200011d8, including its downward
 * growing stack. This extension owns the following 256-byte region instead;
 * no vendor data, USB buffers or stack bytes are reused for tap state. */
typedef struct { volatile uint32_t milliseconds; TapTempo tempo; } ClockState;
#define CLOCK ((ClockState *)0x20001200u)
_Static_assert(sizeof(ClockState) <= 256, "Clock state exceeds reserved RAM");

void harmony_systick(void) {
    ((void (*)(void))0x08006d9du)(); /* Preserve the original 1 ms countdowns. */
    ++CLOCK->milliseconds;
}

static void apply(uint32_t setting, uint32_t layout) {
    uint16_t lower[16], upper[16];
    harmony_generate(setting, layout, lower, upper);
    uint32_t mask;
    __asm__ volatile ("mrs %0, primask\n cpsid i" : "=r"(mask) :: "memory");
    for (uint32_t i = 0; i < 16; ++i) {
        /* Original quantized playback divides each stored value by 178. */
        LOWER[i] = lower[i] * 178u;
        UPPER[i] = upper[i] * 178u;
    }
    REG8(0x200006c8u) = 1; /* Quantized playback is always active. */
    uint32_t selected = REG8(0x200006e6u);
    if (selected >= 16) selected = 0;
    REG32(0x40007408u) = dac_codes[0][lower[selected]];
    REG32(0x40007414u) = dac_codes[1][upper[selected]];
    STATE->harmony = (uint8_t)setting;
    STATE->layout = (uint8_t)layout;
    STATE->ready = 0xa5;
    __asm__ volatile ("msr primask, %0" :: "r"(mask) : "memory");
}

void harmony_boot(void) {
    CLOCK->milliseconds = 0;
    harmony_tempo_init(&CLOCK->tempo, 0);
    STATE->ready = 0;
    REG8(0x200006e6u) = 0; /* Selected pad; ignore persisted position/notes. */
    REG8(0x200006e7u) = 0;
    REG16(0x200006e8u) = 250; /* Existing touch arpeggiator interval, milliseconds. */
    REG8(0x200006eau) = 0; /* No additional arpeggiator octave sweep. */
    REG8(0x20000277u) = 0; /* Original per-voice editing flags remain inactive. */
    REG8(0x20000278u) = 0;
    apply(0, 0); /* Always a usable deterministic map, even before ADC settles. */
    /* Signature in otherwise unused application status bytes; preserve the
     * vendor's report fields and base version for its USB mode-switch handler.
     */
    static const uint8_t signature[8] = {'H','A','R','M','O','N','Y','2'};
    for (uint32_t i = 0; i < 8; ++i) REG8(0x20000134u + i) = signature[i];
    ((void (*)(int,int))0x080061d9u)(0, 1); /* Existing selected-pad indicator. */
}

void harmony_tick(void) {
    uint32_t setting = harmony_bin(REG16(0x200002b4u), SCALE_COUNT,
                                   STATE->ready == 0xa5 ? STATE->harmony : 255u);
    uint32_t layout = harmony_bin(REG16(0x200002b6u), LAYOUT_COUNT,
                                  STATE->ready == 0xa5 ? STATE->layout : 255u);
    if (STATE->ready != 0xa5 || setting != STATE->harmony || layout != STATE->layout)
        apply(setting, layout);
    uint32_t due = harmony_tempo_update(&CLOCK->tempo, CLOCK->milliseconds,
                                         REG8(0x20000274u) & 3u);
    /* Software-pend the existing clock interrupts. Their original handlers
     * retain pad movement, pitch, LED and gate behavior. External clocks remain
     * available; simultaneous internal directions pend both interrupts. */
    uint32_t lines = ((due & 1u) ? 0x8000u : 0u) | ((due & 2u) ? 8u : 0u);
    if (lines) REG32(0x40010410u) |= lines;
}
#endif /* CELLZ_HOST_TEST: native tests exercise only the pure scale functions. */
