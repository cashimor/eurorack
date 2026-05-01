#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

/* ===================== CONFIG ===================== */

#define AUDIO_PIN      15
#define SAMPLE_RATE    22050
#define MAX_VOICES     10

#define NUM_KEYS 5
static const uint key_pins[NUM_KEYS] = {16,17,18,19,20};

/* ===================== RNG ===================== */

static uint32_t rng = 1;
static inline float frand(void) {
    rng = rng * 1664525 + 1013904223;
    return (rng >> 8) * (1.0f / 16777216.0f);
}

/* ===================== SAMPLE TYPE ===================== */

typedef struct {
    int16_t *data;
    uint32_t length;
} Sample;

/* ===================== VOICES ===================== */

typedef struct {
    const Sample *sample;
    uint32_t pos;
    bool active;
} Voice;

static Voice voices[MAX_VOICES];

/* ===================== WAVE GENERATORS ===================== */

void sinewave(int16_t *out, uint32_t len, float decay, float pitch) {
    float vol = 1.0f;
    for (uint32_t i=0;i<len;i++) {
        float ph = (2.0f * M_PI * i) / (pitch + (i / 75.0f));
        out[i] = (int16_t)(sinf(ph) * vol * 32767.0f);
        vol *= decay;
    }
}

void noisewave(int16_t *out, uint32_t len, float decay, int pitch) {
    float vol = 1.0f;
    int cnt = 0;
    float v = frand();
    for (uint32_t i=0;i<len;i++) {
        if (++cnt > pitch) { cnt = 0; v = frand(); }
        out[i] = (int16_t)(v * vol * 32767.0f);
        vol *= decay;
    }
}

void metalwave(
    int16_t *out, uint32_t len, float decay,
    int p1,int p2,int p3,int p4,int p5, float n
) {
    float vol = 1.0f;
    int c1=0,c2=0,c3=0,c4=0,c5=0;
    int o1=-32767,o2=-32767,o3=-32767,o4=-32767,o5=-32767;

    for (uint32_t i=0;i<len;i++) {
        if (++c1>p1){c1=0;o1=-o1;}
        if (++c2>p2){c2=0;o2=-o2;}
        if (++c3>p3){c3=0;o3=-o3;}
        if (++c4>p4){c4=0;o4=-o4;}
        if (++c5>p5){c5=0;o5=-o5;}

        float v = (o1+o2+o3+o4+o5)*(1.0f/6.0f);
        v = (v + frand()*n*32767.0f)*0.5f;
        out[i] = (int16_t)(v*vol);
        vol *= decay;
    }
}

/* ===================== SAMPLES ===================== */

#define L_LONG  10000
#define L_MED   5000
#define L_SHORT 2500

static int16_t bass1_buf[L_LONG];
static int16_t bass2_buf[L_LONG];
static int16_t sn1_buf[L_MED];
static int16_t sn2_buf[L_MED];
static int16_t clap_buf[L_SHORT];
static int16_t ohh_buf[L_LONG];
static int16_t hh1_buf[L_SHORT];
static int16_t hh2_buf[L_SHORT];

static Sample bass1,bass2,sn1,sn2,clap,ohh,hh1,hh2;

static Sample* combo[8][5];

/* ===================== AUDIO ===================== */

static uint pwm_slice;
static repeating_timer_t audio_timer;

static inline void trigger(int v, Sample *s) {
    voices[v].sample = s;
    voices[v].pos = 0;
    voices[v].active = true;
}

static inline int16_t mix_audio(void) {
    int32_t m = 0;
    for (int i=0;i<MAX_VOICES;i++) {
        if (!voices[i].active) continue;
        if (voices[i].pos >= voices[i].sample->length) {
            voices[i].active = false;
            continue;
        }
        m += voices[i].sample->data[voices[i].pos++];
    }
    if (m>32767) m=32767;
    if (m<-32768) m=-32768;
    return (int16_t)m;
}

bool audio_cb(repeating_timer_t *rt) {
    int16_t s = mix_audio();
    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)(s + 32768));
    return true;
}

/* ===================== INIT ===================== */

void init_samples(void) {
    sinewave(bass1_buf,L_LONG,0.9995f,150);
    sinewave(bass2_buf,L_LONG,0.9998f,160);
    noisewave(sn1_buf,L_MED,0.9993f,0);
    noisewave(sn2_buf,L_MED,0.9992f,3);
    noisewave(clap_buf,L_SHORT,0.998f,4);
    metalwave(ohh_buf,L_LONG,0.9997f,45,87,353,452,493,0.8f);
    metalwave(hh1_buf,L_SHORT,0.9985f,2,3,50,7,109,1.0f);
    metalwave(hh2_buf,L_SHORT,0.997f,9,2,5,37,3,0.8f);

    bass1=(Sample){bass1_buf,L_LONG};
    bass2=(Sample){bass2_buf,L_LONG};
    sn1=(Sample){sn1_buf,L_MED};
    sn2=(Sample){sn2_buf,L_MED};
    clap=(Sample){clap_buf,L_SHORT};
    ohh=(Sample){ohh_buf,L_LONG};
    hh1=(Sample){hh1_buf,L_SHORT};
    hh2=(Sample){hh2_buf,L_SHORT};

    Sample* c[8][5]={
        {&bass1,&sn1,&clap,&ohh,&hh1},
        {&bass2,&sn1,&clap,&ohh,&hh1},
        {&bass1,&sn2,&clap,&ohh,&hh1},
        {&bass2,&sn2,&clap,&ohh,&hh1},
        {&bass1,&sn1,&clap,&ohh,&hh2},
        {&bass2,&sn1,&clap,&ohh,&hh2},
        {&bass1,&sn2,&clap,&ohh,&hh2},
        {&bass2,&sn2,&clap,&ohh,&hh2}
    };
    memcpy(combo,c,sizeof(combo));
}

/* ===================== MAIN ===================== */

int main(void) {
    stdio_init_all();

    for(int i=0;i<NUM_KEYS;i++){
        gpio_init(key_pins[i]);
        gpio_set_dir(key_pins[i],GPIO_IN);
        gpio_pull_down(key_pins[i]);
    }

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_set_wrap(pwm_slice, 65535);
    pwm_set_enabled(pwm_slice, true);

    init_samples();

    add_repeating_timer_us(
        -1000000 / SAMPLE_RATE,
        audio_cb,
        NULL,
        &audio_timer
    );

    while (1) {
        for (int i=0;i<NUM_KEYS;i++) {
            if (gpio_get(key_pins[i])) {
                trigger(i, combo[0][i]); // fixed combo for now
                sleep_ms(10);            // debounce
            }
        }
        tight_loop_contents();
    }
}