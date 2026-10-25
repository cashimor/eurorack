/* Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
 * of Cre8audio gear or other equipment resulting from use. Use at your own risk.
 * Pure four-track/two-bank sequencer. No hardware access or flash writes. */
#include "drum_engine.h"
static uint32_t difference(uint32_t a,uint32_t b) { return a>b?a-b:b-a; }
static uint32_t base_us(const Drum *d) { return 15000000u/(40u+d->tempo_adc*200u/4095u); }
static uint32_t swing_part(const Drum *d) { return 500u+d->swing_adc*250u/4095u; }
static uint32_t bit(uint32_t step) { return 1u<<(step&31u); }
static uint32_t next(const Drum *d,uint32_t step) { return (step+1u)%d->length; }
static uint32_t previous(const Drum *d,uint32_t step) { return (step+d->length-1u)%d->length; }
static uint32_t pair(const Drum *d,uint32_t step) { return bit(step)|bit(next(d,step)); }
static void cancel_groups(Drum *d) { for(uint32_t t=0;t<4;++t) d->group[t].active=0; }

void drum_init(Drum *d,uint32_t now) {
    volatile uint8_t *bytes=(volatile uint8_t *)d;
    for(uint32_t i=0;i<sizeof(*d);++i) bytes[i]=0;
    /* Gate jacks: snare/clap. CV jacks: kick/hi-hat (suggested patch only). */
    static const uint16_t defaults[2][4]={
        {0x1010,0x4040,0x1111,0x5555},{0xf010,0xc444,0x1151,0xdddd}
    };
    for(uint32_t b=0;b<2;++b) for(uint32_t t=0;t<4;++t) d->patterns[b][t]=defaults[b][t];
    d->length=16;d->last_ms=now;d->step=15;
}
uint32_t drum_interval_us(const Drum *d) {
    uint32_t base=base_us(d),long_step=base*2u*swing_part(d)/1000u;
    if(!d->has_step) return base;
    return (d->next_step&1u)?long_step:2u*base-long_step;
}
void drum_expire(Drum *d,uint32_t now) {
    for(uint32_t t=0;t<4;++t) if((d->outputs&(1u<<t)) && (int32_t)(now-d->pulse_until_ms[t])>=0) {
        d->outputs&=(uint8_t)~(1u<<t);d->accent_outputs&=(uint8_t)~(1u<<t);
    }
}
static void trigger(Drum *d,uint32_t t,uint32_t now,uint32_t accent) {
    d->outputs|=1u<<t;
    if(t>=2u && accent) d->accent_outputs|=1u<<t;
    else d->accent_outputs&=(uint8_t)~(1u<<t);
    d->pulse_until_ms[t]=now+5u;
}
static void play(Drum *d,uint32_t step,uint32_t now) {
    d->step=(uint8_t)step;d->has_step=1;d->last_step_ms=now;
    for(uint32_t t=0;t<4;++t) {
        DrumTriplet *g=&d->group[t];
        uint32_t starts=d->triplets[d->active_bank][t];
        uint32_t prior=previous(d,step);
        if(g->active && g->bank==d->active_bank && g->step==prior && (starts&bit(prior)))
            continue; /* The paired pad is covered by the three-hit schedule. */
        g->active=0;
        if(!(d->patterns[d->active_bank][t]&bit(step))) continue;
        uint32_t accent=d->accents[d->active_bank][t]&bit(step);
        if((starts&bit(step)) && (d->patterns[d->active_bank][t]&pair(d,step))==pair(d,step)) {
            accent=d->accents[d->active_bank][t]&pair(d,step);
            g->started_ms=now;g->period_us=d->external?d->external_period_us:base_us(d);
            g->active=1;g->next_hit=1;g->step=(uint8_t)step;g->bank=d->active_bank;g->accent=accent?1:0;
        }
        trigger(d,t,now,accent);
    }
}
static void service_groups(Drum *d,uint32_t now) {
    for(uint32_t t=0;t<4;++t) {
        DrumTriplet *g=&d->group[t];
        if(!g->active) continue;
        if(g->bank!=d->active_bank || !(d->triplets[g->bank][t]&bit(g->step))) { g->active=0;continue; }
        uint32_t elapsed_us=(now-g->started_ms)*1000u,span=2u*g->period_us;
        if(elapsed_us>=span) { g->active=0;continue; }
        uint32_t hit=elapsed_us*3u/span;
        if(hit>=g->next_hit && hit<=2u) {
            /* Skip overdue subdivisions after a stall, emitting at most one. */
            trigger(d,t,now,g->accent);g->next_hit=(uint8_t)(hit+1u);
        }
    }
}
static void request(Drum *d,uint32_t now,uint32_t external) {
    uint32_t step=d->next_step;
    d->next_step=next(d,step);d->pending=0;
    uint32_t delay=external&&(step&1u)?d->external_period_us*(2u*swing_part(d)-1000u)/1000u:0u;
    if(delay) {d->pending=1;d->pending_step=(uint8_t)step;d->pending_at_ms=now+(delay+999u)/1000u;}
    else play(d,step,now);
}
static uint32_t group_mask(const Drum *d,uint32_t starts,uint32_t step) {
    if(starts&bit(step)) return pair(d,step);
    if(starts&bit(previous(d,step))) return pair(d,previous(d,step));
    return bit(step);
}
static void edit(Drum *d,uint32_t step,uint32_t modifiers) {
    uint32_t bank=d->active_bank,t=d->instrument;
    uint32_t *p=&d->patterns[bank][t],*a=&d->accents[bank][t],*g=&d->triplets[bank][t];
    uint32_t mask=bit(step);
    d->shift_used|=(uint8_t)modifiers;
    if(modifiers==3u) {d->combo_edited=1;return;} /* Both buttons are save, not edit. */
    if(modifiers==2u) {
        mask=group_mask(d,*g,step);*p|=mask;
        if(t>=2u) { if(*a&mask) *a&=~mask;else *a|=mask; }
    } else if(modifiers==1u) {
        mask=pair(d,step);*p|=mask;
        if(*g&bit(step)) *g&=~bit(step);
        else {
            /* Remove overlapping groups; their other steps remain enabled. */
            *g&=~(bit(previous(d,step))|bit(next(d,step)));*g|=bit(step);
            if(t>=2u && (*a&mask)) *a|=mask;
        }
    } else {
        if(*p&mask) {
            *p&=~mask;*a&=~mask;
            *g&=~(bit(step)|bit(previous(d,step)));
        } else *p|=mask;
    }
    d->group[t].active=0;d->dirty=1;
}
uint32_t drum_update(Drum *d,const DrumInput *in) {
    uint32_t now=in->now,elapsed=now-d->last_ms;
    d->last_ms=now;drum_expire(d,now);
    uint32_t tempo=in->tempo>4095u?4095u:in->tempo,swing=in->swing>4095u?4095u:in->swing;
    if(!d->controls_ready || difference(tempo,d->tempo_adc)>=16u) {
        if(d->controls_ready) cancel_groups(d);
        d->tempo_adc=(uint16_t)tempo;
    }
    uint32_t buttons=in->buttons&3u;
    if(!d->controls_ready) {
        d->swing_adc=(uint16_t)swing;d->last_swing_raw=(uint16_t)swing;
    }
    if((buttons&2u) && !(d->raw_buttons&2u)) {
        d->swing_anchor=(uint16_t)swing;d->swing_pickup=1;
    }
    /* Freeze swing during accent/length edits. After release, pick up the
     * old value by reaching or crossing it, so changing length cannot jump swing. */
    if(!((buttons|d->buttons)&2u)) {
        if(d->swing_pickup && (difference(swing,d->swing_adc)<=32u ||
           (d->last_swing_raw<d->swing_adc && swing>=d->swing_adc) ||
           (d->last_swing_raw>d->swing_adc && swing<=d->swing_adc))) d->swing_pickup=0;
        if(!d->swing_pickup && difference(swing,d->swing_adc)>=16u) d->swing_adc=(uint16_t)swing;
    }
    d->last_swing_raw=(uint16_t)swing;d->controls_ready=1;
    if(buttons!=d->raw_buttons) {d->raw_buttons=(uint8_t)buttons;d->button_changed_ms=now;}
    if(buttons!=d->buttons && now-d->button_changed_ms>=20u) {
        uint32_t rising=buttons&~d->buttons,falling=d->buttons&~buttons;
        for(uint32_t i=0;i<2;++i) if(rising&(1u<<i)) d->pressed_ms[i]=now;
        if(buttons==3u && !d->combo) {d->combo=1;d->combo_ms=now;}
        if(!d->combo) for(uint32_t i=0;i<2;++i)
            if((falling&(1u<<i)) && !(d->shift_used&(1u<<i)) && now-d->pressed_ms[i]<800u) {
                if(i==0) d->instrument=(d->instrument+1u)&3u;
                else if(d->length==32u && d->edit_page==0u) d->edit_page=1;
                else {d->edit_page=0;d->selected_bank^=1u;}
            }
        d->buttons=(uint8_t)buttons;
        if(!buttons && d->combo) d->combo=d->combo_saved=d->combo_edited=0;
    }
    if(!(buttons|d->buttons)) d->length_gesture=0;
    d->shift_used&=(uint8_t)(buttons|d->buttons);
    uint32_t changed_length=0;
    if(buttons==2u && d->buttons==2u && !d->combo && !d->length_gesture &&
       !(d->shift_used&2u) && !in->touches && !d->touches &&
       difference(swing,d->swing_anchor)>=512u) {
        uint32_t length=swing>d->swing_anchor?32u:16u;
        d->length_gesture=1;d->shift_used|=2u;
        if(length!=d->length) {
            d->length=length;d->edit_page=0;d->next_step=0;d->step=(uint8_t)(length-1u);
            d->has_step=d->pending=d->outputs=d->accent_outputs=0;
            d->phase_us=0;d->dirty=1;cancel_groups(d);changed_length=1;
        }
    }
    if(d->length_gesture && (buttons==3u || d->buttons==3u)) d->combo_edited=1;
    if((buttons==3u || d->buttons==3u) && in->touches) d->combo_edited=1;
    uint32_t bank=d->selected_bank^(in->alternate?1u:0u);
    if(bank!=d->active_bank) cancel_groups(d);
    d->active_bank=(uint8_t)bank;
    if(in->touches!=d->raw_touches) {d->raw_touches=in->touches;d->touch_changed_ms=now;}
    if(now-d->touch_changed_ms>=12u) {
        uint32_t pressed=d->raw_touches&~d->touches;
        for(uint32_t pad=0;pad<16;++pad) if(pressed&bit(pad))
            if(!d->length_gesture) edit(d,d->edit_page*16u+15u-pad,buttons?buttons:d->buttons);
        d->touches=d->raw_touches;
    }
    if(d->buttons==3u && buttons==3u && d->combo && !d->combo_saved && !d->combo_edited && !in->touches && now-d->combo_ms>=1500u) {
        d->combo_saved=1;d->save_requested=1;
    }
    if(changed_length) return d->outputs;
    uint32_t accepted=in->clock_edge&&(!d->have_edge || now-d->last_edge_ms>=20u);
    if(accepted) {
        uint32_t interval=now-d->last_edge_ms;
        uint32_t period=d->have_edge&&interval<=4000u?interval*1000u:base_us(d);
        if(!d->external || difference(period,d->external_period_us)>d->external_period_us/4u) cancel_groups(d);
        d->external_period_us=period;d->last_edge_ms=now;d->have_edge=d->external=1;d->phase_us=0;
        request(d,now,1);
    } else if(d->external) {
        uint32_t timeout=d->external_period_us*3u/1000u;
        if(timeout<500u) timeout=500u;
        if(now-d->last_edge_ms>timeout) {d->external=d->have_edge=d->pending=0;d->phase_us=0;cancel_groups(d);}
        else if(d->pending && (int32_t)(now-d->pending_at_ms)>=0) {d->pending=0;play(d,d->pending_step,now);}
    } else {
        uint32_t interval=drum_interval_us(d);
        if(elapsed>4000u) {d->phase_us=0;cancel_groups(d);request(d,now,0);}
        else {
            d->phase_us+=elapsed*1000u;
            if(d->phase_us>=interval) {
                d->phase_us-=interval;request(d,now,0);
                if(d->phase_us>=drum_interval_us(d)) d->phase_us=0;
            }
        }
    }
    service_groups(d,now);
    return d->outputs;
}
uint32_t drum_led_mask(const Drum *d,uint32_t now) {
    if((int32_t)(now-d->feedback_until_ms)<0) return 0;
    if(d->length_gesture) return d->length==32u?0xc000u:0x8000u;
    uint32_t mask=d->patterns[d->active_bank][d->instrument],modifiers=d->raw_buttons|d->buttons;
    if(modifiers==2u) mask=d->accents[d->active_bank][d->instrument]&mask;
    else if(modifiers==1u) {
        uint32_t starts=d->triplets[d->active_bank][d->instrument];
        uint32_t members=0;
        for(uint32_t step=0;step<d->length;++step)
            if((starts&bit(step)) && (mask&pair(d,step))==pair(d,step)) members|=pair(d,step);
        mask&=members;
    } else if(!modifiers && d->has_step && now-d->last_step_ms<60u) mask&=~bit(d->step);
    mask>>=d->edit_page*16u;
    uint32_t physical=0;
    for(uint32_t step=0;step<16;++step) if(mask&bit(step)) physical|=bit(15u-step);
    return physical;
}
void drum_after_save(Drum *d,uint32_t now,uint32_t success) {
    d->save_requested=0;d->saved_status=success?1:2;
    if(success) d->dirty=0;
    d->outputs=d->accent_outputs=d->pending=0;d->phase_us=0;d->last_ms=now;
    cancel_groups(d);d->feedback_until_ms=now+120u;
}
