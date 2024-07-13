/*
 * File:   newmain.c
 * Author: Maarten Hofman
 *
 * Created on June 2, 2024, 10:30 AM
 */

// PIC16F18156 Configuration Bit Settings

#include "config.h"

#include <xc.h>
#include "pwm.h"
#include "uart.h"
#include "eeprom.h"

#define _XTAL_FREQ 32000000

// These are the different types of keyboards
#define OCTAVE_SWITCH
//#define OCTAVE_1
//#define OCTAVE_2
//#define SEQUENCER


#define SCAN(PRT, PIN, BIT, NOTE) \
        TRIS ## PRT ## bits.TRIS ## PIN = 0; \
        PORT ## PRT ## bits.R ## PIN = 0; \
        __delay_us(1); \
        count = 0; \
        INTCONbits.GIE = 0; \
        WPU ## PRT ## bits.WPU ## PIN = 1; \
        TRIS ## PRT ## bits.TRIS ## PIN = 1; \
        while (!PORT ## PRT ## bits.R ## PIN) count++; \
        INTCONbits.GIE = 1; \
        WPU ## PRT ## bits.WPU ## PIN = 0; \
        if (count > 1) { \
            on(BIT, NOTE); \
        } else { \
            off(BIT, NOTE); \
        }

const unsigned char lookup[128] = {
    255, 251, 246, 242, 238, 234, 231, 227, 224, 220, 217, 214, 211, 208, 205,
    202, 199, 196, 194, 191, 189, 186, 184, 182, 179, 177, 175, 173, 171, 169,
    167, 165, 163, 161, 160, 158, 156, 155, 153, 151, 150, 148, 146, 145, 144,
    142, 141, 140, 138, 137, 136, 135, 133, 132, 131, 130, 129, 127, 126, 125,
    124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 113, 112, 111,
    110, 109, 108, 108, 107, 106, 105, 105, 104, 103, 102, 102, 101, 100, 100,
    99, 98, 98, 97, 96, 96, 95, 94, 94, 93, 93, 92, 91, 91, 90, 89, 89, 89, 88,
    88, 87, 86, 86, 85, 85, 84, 84, 84, 83, 83, 82, 82, 81, 81, 80, 80, 79, 78
};


unsigned char count = 0;

void init_timer(void) {
    T0CON0 = 0b10000000;
    T0CON1 = 0b10010100; // LFINTOSC (31kHz) / 16 (= 64 but 4 beats)
    TMR0H = 128;         // Picking mid value. Do not go below 70.
    
    // Setup interrupt for timer
    PIE0bits.TMR0IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

unsigned char bpm = 0;

unsigned char seqstate = 0; // 0 - none, 1 - playback, 2+ - recording.
unsigned char beat = 0;

__interrupt() void incoming() {
    // Timer interrupt
    if (PIR0bits.TMR0IF) {
        if (bpm) {
            bpm = 0;
            if (seqstate == 0 || seqstate == 1) {
              PWM2S1P1 = 0;
              PWM2CONbits.LD = 1;
            }
            if (seqstate == 0 || seqstate > 1) {
              PWM1S1P1 = 60;
              PWM1CONbits.LD = 1;
            }

        } else {
            bpm = 1;
            beat = 1;
            if (seqstate == 0 || seqstate == 1) {
              PWM2S1P1 = 60;
              PWM2CONbits.LD = 1;
            }
            if (seqstate == 0 || seqstate > 1) {
              PWM1S1P1 = 0;
              PWM1CONbits.LD = 1;
            }
        }
        PIR0bits.TMR0IF = 0;
    }
    // EUSART interrupt
    if (PIR4bits.RC1IF) {
      queue[queuewrite & 15] = RC1REG;
      queuewrite++;
      queuesize++;
      if (RC1STAbits.OERR) {
          PORTCbits.RC0 = 1;
          RC1STAbits.CREN = 0;
          RC1STAbits.CREN = 1;
      }    
    }
}

unsigned int sentkeys = 0;
unsigned int oldkeys = 0;
unsigned int keys = 0;
unsigned int same = 0;
unsigned char octave = 60;
unsigned char msg;
unsigned char rate;
unsigned char tmp;

// Sequencer related
unsigned short beatcount = 0;
unsigned char note = 0;
unsigned char onoff = 0;
unsigned char notes[5] = {0, 0, 0, 0, 0};
unsigned char state[5] = {0, 0, 0, 0, 0};
unsigned short currentbeat = 0;
unsigned char found = 0;
unsigned char memptr = 0;


void seq_write_current() {
            // We need to write the current data
            found = 0;
            for (unsigned char i = 0; i < 5; i++) {
                if (!notes[i]) continue;
                if (state[i] & 2) {
                    found++;
                    continue;
                }
                if (!(state[i] & 1)) {
                    found++;
                }
            }
            if (found) {
                msg = currentbeat & 255;
                write(memptr++, msg);
                currentbeat = currentbeat >> 5;
                currentbeat = currentbeat & 0b11111000;
                currentbeat = currentbeat | found;
                msg = currentbeat & 255;
                write(memptr++, msg);
                for (unsigned char i = 0; i < 5; i++) {
                    if (!notes[i]) continue;
                    if (state[i] & 2) {
                        msg = notes[i] + 128;
                        write(memptr++, msg);
                        notes[i] = 0;
                        state[i] = 0;
                    } else {
                      if (!(state[i] & 1)) {
                          write(memptr++, notes[i]);
                          state[i] = state[i] | 1;
                      }
                    }
                }
            }
}

void noteon(unsigned char note) {
    if (note < 12) {
      putch(0x92);
      putch(note + octave);
      putch(100);
      return;
    }
#ifdef OCTAVE_SWITCH
    if (note == 12) {
        octave = octave - 12;
        if (octave < 24) octave = 24;
    }
    if (note == 13) {
        octave = octave + 12;
        if (octave > 96) octave = 96;
    }
    switch(octave) {
        case 24:
            PWM2S1P1 = 60;
            PWM1S1P1 = 0;
            break;
        case 36:
            PWM2S1P1 = 40;
            PWM1S1P1 = 0;
            break;
        case 48:
            PWM2S1P1 = 20;
            PWM1S1P1 = 0;
            break;
        case 60:
            PWM2S1P1 = 0;
            PWM1S1P1 = 0;
            break;
        case 72:
            PWM2S1P1 = 0;
            PWM1S1P1 = 20;
            break;                
        case 84:
            PWM2S1P1 = 0;
            PWM1S1P1 = 40;
            break;
        case 96:
            PWM2S1P1 = 0;
            PWM1S1P1 = 60;
            break;
    }
    PWM1CONbits.LD = 1;
    PWM2CONbits.LD = 1;
#endif
#ifdef SEQUENCER
    tmp = 0;
    if (note == 12) {
        if (seqstate == 0) {
          PWM2S1P1 = 60;
          PWM2CONbits.LD = 1;
          putch(0xfa);
          seqstate = 2;
        } else {
            tmp = 1;
        }
    } else if (note == 13) {
        if (seqstate == 0) {
          PWM1S1P1 = 60;
          PWM1CONbits.LD = 1;
          putch(0xfa);
          seqstate = 1;
          currentbeat = 65535;
        } else {
            tmp = 1;
        }
    }
    if (tmp) {
        if (seqstate > 1) {
            // Ensure all notes are cleared at the end.
            for (unsigned char i = 0; i < 5; i++) {
                state[i] = state[i] | 1;
            }
            beatcount++;
            seq_write_current();
            write(memptr++, 255);
            write(memptr++, 255);
        } else {
            // And that they stop playing in case of playback.
            for (unsigned char i = 0; i < 5; i++) {
                if (notes[i]) {
                    putch(0x82);
                    putch(notes[i]);
                    putch(99);
                }
            }
        }
        putch(0xfc);
        seqstate = 0;
    }
#endif
}

void noteoff(unsigned char note) {
    if (note < 12) {
      putch(0x92);
      putch(note + octave);
      putch(0);    
    }    
}

void on(unsigned int which, unsigned char note) {
    if (keys & which) return;
    keys |= which;
}

void off(unsigned int which, unsigned char note) {
    if (!(keys & which)) return;
    keys &= (65535 - which);
}

void playdiff(void) {
    // At this stage oldkeys contains keys, and sentkeys the sent keys.
    // At the end we want all values to be oldkeys.
    for(unsigned char i = 0; i < 14; i++) {  // there are 14 keys
        if ((keys & 1) != (sentkeys & 1)) {
            if (keys & 1) {
                noteon(i);
            } else {
                noteoff(i);
            }
        }
        sentkeys >>= 1;
        keys >>= 1;
    }
    sentkeys = oldkeys;
    keys = oldkeys;
}

void main(void) {
    ANSELC = 0x00;
    ANSELB = 0x00;
    ANSELA = 0x00;
    TRISC = 0;                 // Everything output for now
    TRISA = 255;                 // Everything output for now
    TRISB = 0;                 // Everything output for now
    
    RC4PPS = 0x0B;
    RB4PPS = 0x0D;
    
    TRISCbits.TRISC0 = 1;
    TRISCbits.TRISC1 = 1;
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC5 = 1;  // Knob
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB3 = 1;
    
    // B1 is is our bus, which is by default pulled high.
    TRISBbits.TRISB1 = 1;
    LATBbits.LATB1 = 0;
    
    WPUA = 0b11111111;
    WPUC = 0b00001111;
    WPUB = 0b00001110;
    
    //RA4PPS = 0x00;
    //T0CON0bits.EN = 0;
    
    init_uart();    
    PWM1_Initialize();
    PWM2_Initialize();
    
#ifdef OCTAVE_1
    octave = 72;
#endif
#ifdef OCTAVE_2
    octave = 84;
#endif
    
#ifdef SEQUENCER
    init_timer();
    PWM1S1P1 = 30;
    PWM2S1P1 = 30;
    PWM1CONbits.LD = 1;
    PWM2CONbits.LD = 1;
    // Setup AD conversion
    ADCON0bits.FM = 0;
    ADCON0bits.CS = 1;   // ADCRC Clock
    ADCON0bits.IC = 0;   // Regular mode
    ADCON0bits.ON = 1;   // Turn ADC on.
    ANSELCbits.ANSELC5 = 1;
    ADPCH = 0b000010101; // RC5 is positive input
#endif
    
    while(1) {
#ifdef SEQUENCER
        if (beat) {
            if (seqstate) putch(0xf8);
            beatcount++;
            beat = 0;
        }
        ADCON0bits.GO = 1;   // Start conversion
#endif
        
        // First we see if the last read was the same.
        if (oldkeys == keys) {
            same++;
            if (same > 50) {
                same = 0;
                if (keys != sentkeys) {
                    playdiff();
                }
            }
        } else {
            same = 0;
        }
        
        // Then check if there are incoming messages
#ifndef SEQUENCER
        if (peek() == 0xf8) {
          getch();
          putch(0xf8);
        }
        if (queuesize > 2) {
            msg = getch();
            putch(msg); // This will make sure clock messages are passed through
            if ((msg == 0x92) || (msg == 0x82)) {
              msg = getch();
              tmp = getch();
#ifdef OCTAVE_SWITCH
              if (tmp != 99) {
                msg = msg - 60 + octave;
              }
#endif
              putch(msg);
              putch(tmp);
            }
        }
#else
        if (queuesize > 2) {
            msg = getch();
            if (msg == 0x92) {
              note = getch();
              onoff = getch();
              if (seqstate > 1) {
                  if (seqstate == 2) {
                    seqstate = 3;
                    beatcount = 0;
                    currentbeat = 0;
                    memptr = 0;
                    for (unsigned char i = 0; i < 5; i++) {
                        notes[i] = 0;
                        state[i] = 0;
                    }
                  }
                  found = 255;
                  for (unsigned char i = 0; i < 5; i++) {
                      if (notes[i] == note) {
                          // Same note, check if it needs to be turned off.
                          if (!onoff) {
                              state[i] = state[i] | 2;
                          }
                          found = 255;
                          break;
                      }
                      if (!notes[i]) {
                          found = i;
                      }
                  }
                  if (found < 6) {
                      notes[found] = note;
                      state[found] = 0;
                      if (!onoff) {
                          // Record offs anyway. Just to be sure.
                          state[found] = 2;
                      }
                  }
              }
            }
        }
        if ((seqstate > 2) && (currentbeat != beatcount)) {
            // We need to write the current data
            seq_write_current();
            currentbeat = beatcount;
        } else if (seqstate == 1) {
            if (currentbeat == 65535) {
                memptr = 0;
                msg = read(memptr++);
                currentbeat = read(memptr++);
                found = currentbeat & 7;
                currentbeat = currentbeat << 5;
                currentbeat = currentbeat & 0xFF00;
                currentbeat = currentbeat + msg;
                for (unsigned char i = 0; i < 5; i++) notes[i] = 0;
                beatcount = 0;
            }
            if (currentbeat == beatcount) {
                for (unsigned char i = 0; i < found; i++) {
                  note = read(memptr++);
                  if (note & 128) {
                      putch(0x82); // Use note off to show sequencer.
                  } else {
                    putch(0x92);
                  }
                  tmp = note & 0x7f;
                  putch(tmp);
                  putch(99);
                  if (note & 128) {
                      for (unsigned char i = 0; i < 5; i++) {
                          if (notes[i] == tmp) {
                              notes[i] = 0;
                          }
                      }
                  } else {
                      for (unsigned char i = 0; i < 5; i++) {
                          if (!notes[i]) {
                              notes[i] = tmp;
                              break;
                          }
                      }
                  }
                }
                msg = read(memptr++);
                currentbeat = read(memptr++);
                if (currentbeat == 255) {
                    currentbeat = 65535;
                } else {
                  found = currentbeat & 7;
                  currentbeat = currentbeat << 5;
                  currentbeat = currentbeat & 0xFF00;
                  currentbeat = currentbeat + msg;                
                }
            }
        }
#endif
        oldkeys = keys;
        if (PORTBbits.RB1) {
          TRISBbits.TRISB1 = 0;
          SCAN(A, A0, 1, 0)
          SCAN(A, A1, 8, 3)
          SCAN(A, A2, 32, 5)
          SCAN(A, A3, 64, 6)
          SCAN(A, A4, 128, 7)
          SCAN(A, A5, 512, 9)
          SCAN(A, A6, 2048, 11)
          SCAN(A, A7, 256, 8)
          SCAN(C, C0, 1024, 10)
          SCAN(C, C1, 16, 4)
          SCAN(C, C2, 2, 1)
          SCAN(C, C3, 4, 2)
          SCAN(B, B3, 4096, 12)
          SCAN(B, B2, 8192, 13)
          TRISBbits.TRISB1 = 1;
        }
#ifdef SEQUENCER
        if (!ADCON0bits.GO) {
          rate = ADRESH;
          rate = rate >> 1;
          TMR0H = lookup[rate];
        }
#endif
     }
    return;
}
