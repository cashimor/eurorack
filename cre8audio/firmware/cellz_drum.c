/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk.
 * Cellz Drum D2 hardware adapter, pinned official V0.0.3.0 only. */
#include "drum_engine.h"
#include "drum_store.h"
#define R32(a) (*(volatile uint32_t *)(a))
#define R16(a) (*(volatile uint16_t *)(a))
#define R8(a) (*(volatile uint8_t *)(a))
#define READY 0x314d5244u
#define PAGE0 0x0800e800u
#define PAGE1 0x0800ec00u
#define CV_NORMAL 1352u /* Nominal ~3.3 V; bench verification still required. */
#define CV_HIGH 2048u /* Midscale. Nominal ~5 V inferred from pitch scaling;
                       * measure the actual jack voltage before drum patching. */
typedef struct {
    volatile uint32_t milliseconds, pending_clock, ready;
    Drum engine;
    uint16_t led_mask;
    uint8_t scan_pad;
    uint32_t sequence;
} Runtime;
#define D ((Runtime *)0x20001200u)
_Static_assert(sizeof(Runtime)<=1024,"Drum runtime exceeds reserved RAM");
_Static_assert(__builtin_offsetof(Drum,last_ms)==DRUM_PATTERN_WORDS*2u,"Persistent masks must remain contiguous");

static uint32_t lock(void) {
    uint32_t mask;
    __asm__ volatile("mrs %0, primask\n cpsid i":"=r"(mask)::"memory");
    return mask;
}
static void unlock(uint32_t mask) { __asm__ volatile("msr primask, %0"::"r"(mask):"memory"); }

static void outputs(uint32_t mask) {
    uint32_t accents=D->engine.accent_outputs;
    uint32_t high=((mask&1u)?8u:0u)|((mask&2u)?4u:0u);
    R32(0x40010810u)=high|((12u&~high)<<16);
    R32(0x40007408u)=(mask&4u)?((accents&4u)?CV_HIGH:CV_NORMAL):0u;
    R32(0x40007414u)=(mask&8u)?((accents&8u)?CV_HIGH:CV_NORMAL):0u;
}
static void blank_leds(void) { R32(0x40010c10u)=0xf000u; }
static void scan_leds(void) {
    blank_leds();
    uint32_t pad=D->scan_pad, column=pad&3u;
    for(uint32_t row=0;row<4;++row)
        R16(0x40000834u+row*4u)=(row==(pad>>2) && (D->led_mask&(1u<<pad)))?960u:0u;
    R32(0x40010c14u)=1u<<(12u+column);
    D->scan_pad=(pad+1u)&15u;
}
void drum_systick(void) {
    ((void (*)(void))0x08006d9du)(); /* Preserve USB/main-loop countdowns. */
    if(D->ready!=READY) return;
    uint32_t now=++D->milliseconds;
    uint32_t previous=D->engine.outputs;
    drum_expire(&D->engine,now);
    if(previous!=D->engine.outputs) outputs(D->engine.outputs);
    scan_leds();
}
void drum_left_irq(void) {
    if(R32(0x40010414u)&0x8000u) {
        R32(0x40010414u)=0x8000u;
        if(D->ready==READY) D->pending_clock=1;
    }
}
void drum_right_irq(void) { if(R32(0x40010414u)&8u) R32(0x40010414u)=8u; }

static int erase_page(void *context,uint32_t page) {
    (void)context;
    if(page>1) return 0;
    return ((int (*)(uint32_t))0x08004b59u)(page?PAGE1:PAGE0)==4;
}
static int program_word(void *context,uint32_t page,uint32_t offset,uint32_t value) {
    (void)context;
    if(page>1 || offset>1020u || (offset&3u)) return 0;
    return ((int (*)(uint32_t,uint32_t))0x08004c35u)((page?PAGE1:PAGE0)+offset,value)==4;
}
void drum_save(void) {
    uint32_t mask=lock();
    outputs(0); blank_leds();
    DrumStoreIO io;
    io.context=0; io.erase=erase_page; io.program=program_word;
    ((void (*)(void))0x08004c9du)();
    int result=drum_store_save((const uint8_t *)PAGE0,(const uint8_t *)PAGE1,
                              &D->engine.patterns[0][0],&io);
    ((void (*)(void))0x08004bddu)();
    if(result) {
        uint16_t verified[DRUM_PATTERN_WORDS];
        result=drum_store_load((const uint8_t *)PAGE0,(const uint8_t *)PAGE1,verified,&D->sequence);
        if(result) for(uint32_t i=0;i<DRUM_PATTERN_WORDS;++i) if(verified[i]!=((uint16_t *)D->engine.patterns)[i]) result=0;
    }
    D->pending_clock=0;
    drum_after_save(&D->engine,D->milliseconds,(uint32_t)result);
    D->led_mask=0;
    unlock(mask);
}
void drum_boot(void) {
    D->ready=0; D->milliseconds=0; D->pending_clock=0; D->scan_pad=0; D->sequence=0;
    outputs(0); blank_leds();
    drum_init(&D->engine,0);
    if(drum_store_load((const uint8_t *)PAGE0,(const uint8_t *)PAGE1,
                       &D->engine.patterns[0][0],&D->sequence)) D->engine.saved_status=1;
    D->led_mask=(uint16_t)drum_led_mask(&D->engine,0);
    /* Existing GPIO configuration routes TIM4 to the four LED rows. Use
     * 20 kHz PWM (48 MHz timer), 40% duty like the original single-pad driver.
     * One pad per millisecond (62.5 Hz full refresh) preserves the original
     * one-LED-at-a-time peak load. Blank columns before changing row PWM. */
    R16(0x40000800u)=0;
    R16(0x40000828u)=0; R16(0x4000082cu)=2399;
    R16(0x40000818u)=0x6060; R16(0x4000081cu)=0x6060;
    R16(0x40000820u)=0x1111;
    for(uint32_t row=0;row<4;++row) R16(0x40000834u+row*4u)=0;
    R16(0x40000814u)=1; R16(0x40000800u)=1;
    static const uint8_t signature[8]={'D','R','U','M','0','0','0','2'};
    for(uint32_t i=0;i<8;++i) R8(0x20000134u+i)=signature[i];
    D->ready=READY;
}
void drum_tick(void) {
    if(D->ready!=READY) return;
    uint32_t mask=lock();
    DrumInput in;
    in.now=D->milliseconds;
    in.buttons=R8(0x20000274u)&3u;
    in.touches=R16(0x200006d4u);
    in.tempo=R16(0x200002b4u); in.swing=R16(0x200002b6u);
    in.alternate=(R32(0x40010c08u)&8u)?0:1; /* Jack gate drives PB3 low. */
    in.clock_edge=D->pending_clock?1:0; D->pending_clock=0;
    outputs(drum_update(&D->engine,&in));
    D->led_mask=(uint16_t)drum_led_mask(&D->engine,in.now);
    /* Additional read-only application status, after the preserved base fields. */
    R8(0x2000013cu)=D->engine.instrument;
    R8(0x2000013du)=D->engine.selected_bank;
    R8(0x2000013eu)=D->engine.active_bank;
    R8(0x2000013fu)=D->engine.has_step?D->engine.step:255u;
    R8(0x20000140u)=D->engine.external;
    R8(0x20000141u)=D->engine.dirty;
    R8(0x20000142u)=D->engine.saved_status;
    unlock(mask);
    if(D->engine.save_requested) drum_save();
}
