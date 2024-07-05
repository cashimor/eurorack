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

#define _XTAL_FREQ 32000000

// These are the different types of keyboards
//#define OCTAVE_SWITCH
#define OCTAVE_1
#define SEQUENCER


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

void init_uart(void) {
  SP1BRG = 15;                       // 3 at 8Mhz, 9 at 20Mhz (from 4 to 10)
  SP1BRGH = 0;
  TX1STAbits.TXEN = 1;               // enable transmitter
  TX1STAbits.SYNC = 0;
  TX1STAbits.BRGH = 0;
  RC1STAbits.RX9 = 0;
  RC1STAbits.ADDEN = 0;
  RC1STAbits.SPEN = 1;               // enable serial port
  RC1STAbits.CREN = 1;               // enable receiver
  BAUD1CONbits.BRG16 = 0;            // Set at 1 Mhz with SP1BRG at 1
  BAUD1CONbits.ABDEN = 0;
  RB5PPS = 0x13;
  TRISCbits.TRISC7 = 1;                 // RX
  
  // Setup interrupt for input
  PIE4bits.RC1IE = 1;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;

}

void init_timer(void) {
    T0CON0 = 0b10000000;
    T0CON1 = 0b10010100; // LFINTOSC (31kHz) / 16 (= 64 but 4 beats)
    TMR0H = 128;         // Picking mid value. Do not go below 70.
    
    // Setup interrupt for timer
    PIE0bits.TMR0IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

unsigned char queue[16];
unsigned char queueread = 0;
unsigned char queuewrite = 0;
unsigned char queuesize = 0;
unsigned char bpm = 0;

unsigned char seqstate = 0; // 0 - none, 1 - playback, 2 - recording.

__interrupt() void incoming() {
    // Timer interrupt
    if (PIR0bits.TMR0IF) {
        if (bpm) {
            bpm = 0;
            if (seqstate == 0 || seqstate == 1) {
              PWM2S1P1 = 0;
              PWM2CONbits.LD = 1;
            }
            if (seqstate == 0 || seqstate == 2) {
              PWM1S1P1 = 60;
              PWM1CONbits.LD = 1;
            }

        } else {
            bpm = 1;
            if (seqstate == 0 || seqstate == 1) {
              PWM2S1P1 = 60;
              PWM2CONbits.LD = 1;
            }
            if (seqstate == 0 || seqstate == 2) {
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

char getch() {
    unsigned char data;

    while(!queuesize) {
    }
    data = queue[queueread & 15];
    queueread++;
    queuesize--;
    return data;
}

void putch(unsigned char data) {
  while(!PIR4bits.TX1IF) {          // wait until the transmitter is ready
  }
  TX1REG = data;                     // send one character
}

unsigned int sentkeys = 0;
unsigned int oldkeys = 0;
unsigned int keys = 0;
unsigned int same = 0;
unsigned char octave = 60;
unsigned char msg;
unsigned char rate;

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
    if (note == 12) {
        if (seqstate == 0) {
          PWM2S1P1 = 60;
          PWM2CONbits.LD = 1;
          seqstate = 2;
        } else {
            seqstate = 0;
        }
    } else if (note == 13) {
        if (seqstate == 0) {
          PWM1S1P1 = 60;
          PWM1CONbits.LD = 1;
          seqstate = 1;
        } else {
            seqstate = 0;
        }
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

unsigned char read(unsigned char location) {
  // This code block will read 1 word (byte) of DFM
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x070;
  NVMADRL = location;
  NVMCON1bits.RD = 1; // Initiate read cycle
  return NVMDATL;
}

void write(unsigned char location, unsigned char v) {
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x070;
  NVMADRL = location;
  NVMDATL = v;
  NVMCON1bits.WREN = 1; // Allows program/erase cycles
  INTCONbits.GIE = 0; // Disable interrupts
  NVMCON2 = 0x55; // Perform required unlock sequence
  NVMCON2 = 0xAA;
  NVMCON1bits.WR = 1; // Begin program/erase cycle
  INTCONbits.GIE = 1; // Restore interrupt enable bit value
  NVMCON1bits.WREN = 0; // Disable program/erase
  NVMCON1bits.WRERR = 0; // Ignore errors
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
        ADCON0bits.GO = 1;   // Start conversion
#endif
        
        // First we see if the last read was the same.
        if (oldkeys == keys) {
            same++;
            if (same > 10) {
                same = 0;
                if (keys != sentkeys) {
                    playdiff();
                }
            }
        }
        // Then check if there are incoming messages
        if (queuesize > 2) {
            msg = getch();
#ifndef SEQUENCER
            putch(msg);
#endif
            if (msg == 0x92) {
              msg = getch();
              msg = msg - 60 + octave;
#ifndef SEQUENCER
              putch(msg);
#endif
            }
            msg = getch();
#ifndef SEQUENCER
            putch(msg);
#endif
        }
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
