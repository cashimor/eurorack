/*
 * File:   fullsynth.c
 * Author: Maarten Hofman
 *
 * Created on July 19, 2025, 10:21 AM
 */

// PIC16F18156 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_1MHz// Reset Oscillator Selection bits (HFINTOSC (1MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = OFF      // Clock Switch Enable bit (The NOSC and NDIV bits cannot be changed by user software)
#pragma config VDDAR = HI       // VDD Range Analog Calibration Selection bit (Internal analog systems are calibrated for operation between VDD = 2.3 - 5.5V)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)

// CONFIG2
#pragma config MCLRE = INTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR is port-defined function; If LVP = 1, RA3 pin function is MCLR)
#pragma config PWRTS = PWRT_OFF // Power-up Timer Selection bits (PWRT is disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out reset disabled)
#pragma config DACAUTOEN = OFF  // DAC Buffer Automatic Range Select Enable bit (DAC Buffer reference range is determined by the REFRNG bit)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection bit (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD module is disabled; ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = OFF    // PPSLOCKED One-Way Set Enable bit (The PPSLOCKED bit can be set and cleared as needed (unlocking sequence is required))
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will not cause a reset)
#pragma config DEBUG = OFF      // Background Debugger (Background Debugger disabled)

// CONFIG3
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT Operating Mode bits (WDT Disabled, SEN is ignored)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT Input Clock Select bits (Software Control)

// CONFIG4
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash (SAF) Enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block is NOT write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block is NOT write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration Register is NOT write protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM is NOT write protected)
#pragma config WRTSAF = OFF     // Storage Area Flash (SAF) Write Protection bit (SAF is NOT write protected)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low Voltage programming enabled. MCLR/Vpp pin function is MCLR. MCLRE Configuration bit is ignored)

// CONFIG5
#pragma config CP = OFF         // Program Flash Memory Code Protection bit (Program Flash Memory code protection is disabled)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>


#define OSCS 4


const unsigned short lookup[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64808,  // 0-11
    61161, 57736, 54495, 51413, 48543, 45808, 43252, 40815, 38520, 36363, 34316, 32393,  // 12-23 (B0)
    30580, 28859, 27240, 25713, 24271, 22909, 21621, 20407, 19263, 18181, 17160, 16196,  // 24-35 (C1-B1)
    15287, 14429, 13619, 12856, 12133, 11452, 10810, 10203, 9630, 9090, 8580, 8098,  // 36-47 (C2-B2)
    7644, 7215, 6810, 6427, 6067, 5726, 5404, 5101, 4815, 4544, 4289, 4049, // 48-59 (C3-B3)
    3821, 3607, 3404, 3213, 3033, 2862, 2702, 2550, 2407, 2272, 2144, 2024, // 60-71 (C4-B4)
    1910, 1803, 1702, 1606, 1516, 1431, 1350, 1275, 1203, 1135, 1072, 1011, // 72-83 (C5-B5)
    955, 901, 850, 803, 757, 715, 675, 637, 601, 567, 535, 505, // 84-95 (C6-B6)
    477, 450, 425, 401, 378, 357, 337, 318, 300, 283, 267, 252, // 96-107 (C7-B7)
    238, 224, 212, 200, 189, 178, 168, 158, 150, 141, 133, 126, // 108-119 (C8-B8)
    118, 111, 105, 99, 94, 88, 83, 79  // 120-127 (C9-G9)    
};

const unsigned char adsr[64] = {
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 20, 19,
  36, 18, 35, 52, 34, 84, 67, 50, 100, 83, 116, 99, 132, 82, 164, 131,
  98, 180, 147, 196, 114, 163, 212, 130, 195, 146, 211, 162, 227, 178, 194, 210,
  226, 242, 161, 177, 193, 209, 225, 241, 128, 144, 160, 176, 192, 208, 224, 240
};

void PWM1_Initialize(void) {
 PWM1ERS = 0x00; // PWMERS External Reset Disabled;
 PWM1CLK = 0x02; // PWMCLK FOSC;
 PWM1LDS = 0x00; // PWMLDS Autoload disabled;
 PWM1PR = 0x0000;
 PWM1CPRE = 0x00; // PWMCPRE No prescale;
 PWM1PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM1GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 // PWMS1P1IF PWM1 output match did not occur;
 PWM1GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM1S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM1S1P1 = 0x0000;
 PWM1CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM1CONbits.LD = 1;

}

void PWM2_Initialize(void) {
 PWM2ERS = 0x00; // PWMERS External Reset Disabled;
 PWM2CLK = 0x02; // PWMCLK FOSC;
 PWM2LDS = 0x00; // PWMLDS Autoload disabled;
 PWM2PR = 0x0000;
 PWM2CPRE = 0x00; // PWMCPRE No prescale;
 PWM2PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM2GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 // PWMS1P1IF PWM1 output match did not occur;
 PWM2GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM2S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM2S1P1 = 0x0000;
 PWM2S1P2 = 0x0000;
 PWM2CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM2CONbits.LD = 1;
}

void PWM3_Initialize(void) {
 PWM3ERS = 0x00; // PWMERS External Reset Disabled;
 PWM3CLK = 0x02; // PWMCLK FOSC;
 PWM3LDS = 0x00; // PWMLDS Autoload disabled;
 PWM3PR = 0x0000;
 PWM3CPRE = 0x00; // PWMCPRE No prescale;
 PWM3PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM3GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 // PWMS1P1IF PWM1 output match did not occur;
 PWM3GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM3S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM3S1P1 = 0x0000;
 PWM3CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM3CONbits.LD = 1;
}

void PWM4_Initialize(void) {
 PWM4ERS = 0x00; // PWMERS External Reset Disabled;
 PWM4CLK = 0x02; // PWMCLK FOSC;
 PWM4LDS = 0x00; // PWMLDS Autoload disabled;
 PWM4PR = 0x0000;
 PWM4CPRE = 0x00; // PWMCPRE No prescale;
 PWM4PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM4GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 // PWMS1P1IF PWM1 output match did not occur;
 PWM4GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM4S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM4S1P1 = 0x0000;
 PWM4CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM4CONbits.LD = 1;
}

void DAC_Initialize(void) {
    DAC1CON = 0b10100000; // EN, auto range, RB7 only (no RA2), Vdd, Vss
    CPCONbits.CPON = 3;
}

void init_timer2(void) {
    T2PR = 249;
    T2CON = 0b10000000; // ON with 0 prescalers
    PIE2bits.TMR2IE = 1; // TMR2 Peripheral Interrupt Enable
    T2HLT = 0;
    T2CLKCON = 1;
}

inline void load0(unsigned char value) {
    DAC1DATL = value;
}

void init_uart(void) {
  SP1BRG = 1;                        // 3 at 8Mhz, 9 at 20Mhz (from 4 to 10)
  SP1BRGH = 0;
  TX1STAbits.TXEN = 1;               // enable transmitter
  TX1STAbits.SYNC = 0;
  TX1STAbits.BRGH = 0;
  RC1STAbits.RX9 = 0;
  RC1STAbits.ADDEN = 0;
  RC1STAbits.SPEN = 1;               // enable serial port
  RC1STAbits.CREN = 1;               // enable receiver
  BAUD1CONbits.BRG16 = 1;            // Set at 1 Mhz with SP1BRG at 1
  BAUD1CONbits.ABDEN = 0;
  
  // Setup interrupt
  PIE4bits.RC1IE = 1;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;
}

void init_uart2(void) {
  SP2BRG = 1;                        // 3 at 8Mhz, 9 at 20Mhz (from 4 to 10)
  SP2BRGH = 0;
  TX2STAbits.TXEN = 1;               // enable transmitter
  TX2STAbits.SYNC = 0;
  TX2STAbits.BRGH = 0;
  RC2STAbits.RX9 = 0;
  RC2STAbits.ADDEN = 0;
  RC2STAbits.SPEN = 1;               // enable serial port
  RC2STAbits.CREN = 1;               // enable receiver
  BAUD2CONbits.BRG16 = 1;            // Set at 1 Mhz with SP1BRG at 1
  BAUD2CONbits.ABDEN = 0;
  
  // Setup interrupt
  PIE5bits.RC2IE = 1;
}


volatile unsigned char queue[64];
volatile unsigned char queueread = 0;
volatile unsigned char queuewrite = 0;
volatile unsigned char queuesize = 0;
volatile unsigned char queue2[64];
volatile unsigned char queue2read = 0;
volatile unsigned char queue2write = 0;
volatile unsigned char queue2size = 0;
volatile unsigned char queue3[64];
volatile unsigned char queue3read = 0;
volatile unsigned char queue3write = 0;
volatile unsigned char queue3size = 0;
unsigned short toloadnote[OSCS] = {65535, 65535, 65535, 65535};
unsigned short toloadpwm[OSCS] = {65535, 65535, 65535, 65535};
unsigned short toloadvelocity[OSCS] = {65535, 65535, 65535, 65535};
unsigned short loadednote[OSCS] = {0, 0, 0, 0};
unsigned char note[OSCS] = {0, 0, 0, 0};
unsigned char gate[OSCS] = {0, 0, 0, 0};
unsigned char tick[OSCS] = {0, 0, 0, 0};
unsigned char velocity[OSCS] = {0, 0, 0, 0};
unsigned char target[OSCS] = {0, 0, 0, 0};
unsigned char pwm = 255;
volatile unsigned char tickcount = 0;
unsigned char lastmidi = 0;

__interrupt() void incoming() {
    if (PIR2bits.TMR2IF) {        
        tickcount++; 
        PIR2bits.TMR2IF = 0; 
    }
    if (PIR4bits.RC1IF) {
      queue[queuewrite & 63] = RC1REG;
      queuewrite++;
      queuesize++;
      if (RC1STAbits.OERR) {
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
        queuesize = 0;
      }    
    }
    if (PIR5bits.RC2IF) {
      queue2[queue2write & 63] = RC2REG;
      queue2write++;
      queue2size++;
      if (RC2STAbits.OERR) {
        RC2STAbits.CREN = 0;
        RC2STAbits.CREN = 1;
        queue2size = 0;
      }
    }
    if (PIR5bits.TX2IF) {
      if (queue3size > 0) {
        TX2REG = queue3[queue3read & 63];
        queue3read++;
        queue3size--;
      } else {
        PIE5bits.TX2IE = 0; 
      }
    }
}

inline unsigned char getch() {
    unsigned char data;
    
    while(!queuesize) {
    }
    INTCONbits.GIE = 0;
    data = queue[queueread & 63];
    queueread++;
    queuesize--;
    INTCONbits.GIE = 1;
    return data;
}

inline unsigned char getch2() {
    unsigned char data;
    
    while(!queue2size) {
    }
    INTCONbits.GIE = 0;
    data = queue2[queue2read & 63];
    queue2read++;
    queue2size--;
    INTCONbits.GIE = 1;
    return data;
}

void putch2(unsigned char data) {
    INTCONbits.GIE = 0;
    queue3[queue3write & 63] = data;
    queue3write++;
    queue3size++;
    PIE5bits.TX2IE = 1;
    INTCONbits.GIE = 1;
}

inline unsigned char peek() {
    if (!queuesize) return 0;
    return queue[queueread & 63];
}

inline void load1() {
    unsigned char load = 0;
    
    if (PWM1CONbits.LD) return; 
    if (toloadnote[0] != 65535) {
        PWM1PR = toloadnote[0];
        loadednote[0] = toloadnote[0];
        toloadnote[0] = 65535;
        load = 1;
    }
    if (toloadpwm[0] != 65535) {
        PWM1S1P1 = toloadpwm[0];
        toloadpwm[0] = 65535;
        load = 1;
    }
    if (toloadvelocity[0] != 65535) {
        PWM1S1P2 = toloadvelocity[0];
        toloadvelocity[0] = 65535;
        load = 1;
    }
    if (load) PWM1CONbits.LD = 1;
}

inline void load2() {
    unsigned char load = 0;
    
    if (PWM2CONbits.LD) return;
    if (toloadnote[1] != 65535) {
        PWM2PR = toloadnote[1];
        loadednote[1] = toloadnote[1];
        toloadnote[1] = 65535;
        load = 1;
    }
    if (toloadpwm[1] != 65535) {
        PWM2S1P1 = toloadpwm[1];
        toloadpwm[1] = 65535;
        load = 1;
    }
    if (toloadvelocity[1] != 65535) {
        PWM2S1P2 = toloadvelocity[1];
        toloadvelocity[1] = 65535;
        load = 1;
    }
    if (load) PWM2CONbits.LD = 1;
}

inline void load3() {
    unsigned char load = 0;
    
    if (PWM3CONbits.LD) return;
    if (toloadnote[2] != 65535) {
        PWM3PR = toloadnote[2];
        loadednote[2] = toloadnote[2];
        toloadnote[2] = 65535;
        load = 1;
    }
    if (toloadpwm[2] != 65535) {
        PWM3S1P1 = toloadpwm[2];
        toloadpwm[2] = 65535;
        load = 1;
    }
    if (toloadvelocity[2] != 65535) {
        PWM3S1P2 = toloadvelocity[2];
        toloadvelocity[2] = 65535;
        load = 1;
    }
    if (load) PWM3CONbits.LD = 1;
}

inline void load4() {
    unsigned char load = 0;
    
    if (PWM4CONbits.LD) return;
    if (toloadnote[3] != 65535) {
        PWM4PR = toloadnote[3];
        loadednote[3] = toloadnote[3];
        toloadnote[3] = 65535;
        load = 1;
    }
    if (toloadpwm[3] != 65535) {
        PWM4S1P1 = toloadpwm[3];
        PWM4CONbits.LD = 1;
        toloadpwm[3] = 65535;
        load = 1;
    }
    if (toloadvelocity[3] != 65535) {
        PWM4S1P2 = toloadvelocity[3];
        PWM4CONbits.LD = 1;
        toloadvelocity[3] = 65535;
        load = 1;
    }
    if (load) PWM4CONbits.LD = 1;
}

unsigned short compute_pwm(unsigned char pwm, unsigned short max) {
  if (pwm == 0) return 0;
  if (pwm == 255) return max / 2;
  unsigned short total = 0;
  unsigned short current = max / 4;
  if (pwm & 128) total += current;
  current /= 2;
  if (pwm & 64) total += current;
  current /= 2;
  if (pwm & 32) total += current;
  current /= 2;
  if (pwm & 16) total += current;
  current /= 2;
  if (pwm & 8) total += current;
  current /= 2;
  if (!current) current = 1;
  if (pwm & 4) total += current;
  current /= 2;
  if (!current) current = 1;
  if (pwm & 2) total += current;
  current /= 2;
  if (!current) current = 1;
  if (pwm & 1) total += current;
  return total;
}

unsigned short compute_velocity(unsigned char velocity, unsigned short max) {
  if (velocity == 0) return 0;
  if (velocity == 255) return max;
  unsigned short total = 0;
  unsigned short current = max >> 1;
  if (velocity & 128) total += current;
  current >>= 1;
  if (velocity & 64) total += current;
  current >>= 1;
  if (velocity & 32) total += current;
  current >>= 1 ;
  if (velocity & 16) total += current;
  current >>= 1;
  if (velocity & 8) total += current;
  current >>= 1;
  if (!current) current = 1;
  if (velocity & 4) total += current;
  current >>= 1;
  if (!current) current = 1;
  if (velocity & 2) total += current;
  current >>= 1;
  if (!current) current = 1;
  if (velocity & 1) total += current;
  return total;
}

inline void add_note(unsigned char current_note, unsigned char current_velocity) {
    unsigned char i;
    unsigned char found = 255;
    unsigned char maxvel = 255;
    for (i = 0; i < OSCS; i++) {
        if (note[i] == current_note) {
            target[i] = current_velocity << 1;
            velocity[i] = 0;
            toloadvelocity[i] = 0;
            toloadnote[i] = lookup[current_note];
            toloadpwm[i] = compute_pwm(pwm, lookup[current_note]);
            gate[i] = 1;
            return;
        }
        if (!gate[i]) {
            if (velocity[i] < maxvel) {
              found = i;
              maxvel = velocity[i];
            }
        }
    }
    if (found == 255) {
        if (lastmidi != 0x92) {
            putch2(0x92);
            lastmidi = 0x92;
        }
        putch2(current_note);
        putch2(current_velocity);
        return;
    } else {
        if (lastmidi != 0xB2) {
            putch2(0xB2);    // CC Status, Channel 3
            lastmidi = 0xB2;
        }
        putch2(102);     // CC Number 102 (Kill Specific Note)
        putch2(current_note);      // Value is Note 51 (The note to kill)
    }
    target[found] = current_velocity << 1;
    velocity[found] = 0;
    toloadvelocity[found] = 0;
    toloadnote[found] = lookup[current_note];
    toloadpwm[found] = compute_pwm(pwm, lookup[current_note]);
    note[found] = current_note;
    gate[found] = 1;
}

inline void remove_note(unsigned char current_note) {
    unsigned char i;
    unsigned char found = 0;
    for (i = 0; i < OSCS; i++) {
        if (note[i] == current_note) {
          found = 1;
          gate[i] = 0;
        }
    }
    if (!found) {
        if (lastmidi != 0x92) {
          putch2(0x92);
          lastmidi = 0x92;
        }
        putch2(current_note);
        putch2(0);
    }
}

inline void kill_note(unsigned char current_note) {
    unsigned char i;
    for (i = 0; i < OSCS; i++) {
        if (note[i] == current_note) {
          toloadvelocity[i] = 0;
          velocity[i] = 0;
          target[i] = 0;
          gate[i] = 0;
        }
    }
}

void main(void) {
    unsigned char selected_channel = 0xFF;
    unsigned char current_channel = 0x00;
    unsigned char current_note = 0x00;
    unsigned char current_velocity = 0x00;
    unsigned char value = 0;  // Incoming MIDI value
    unsigned char state = 0;  // Current MIDI command
    unsigned char state2 = 0;
    unsigned char pwmold = 255;
    unsigned char adcresult[5] = { 255, 128, 128, 128, 128 };
    unsigned char currentadc = 0; // Start with RA0
    unsigned char oldtick = 0;
    unsigned char maxvel = 0;
    unsigned char rt = 1, rv = 1, at = 1, av = 1, dt = 1, dv = 1, sustain = 255;
    unsigned char pedal = 0;
    unsigned char midi_pwm = 255, midi_attack = 255, midi_decay = 255, midi_sustain = 255, midi_release = 255, active_output = 0;
    
    init_timer2();
    init_uart2();
    init_uart();
    PWM1_Initialize();
    PWM2_Initialize();
    PWM3_Initialize();
    PWM4_Initialize();
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    TRISA = 0xFF; // All A are input for now.
    TRISB = 0b00001110; // RB1-RB3 are set as an input (RX2 and other pins)
    TRISC = 0x82; // RC7 is RX1 and RC1 MUST be input
    RC0PPS = 0x00;
 
    // Voice 1 (second transistor in schematic):
    RC6PPS = 0x0B;
    RC0PPS = 0x0C;
    // Voice 2 (first transistor in schematic):
    RB4PPS = 0x0D;
    RB0PPS = 0x0E; // THIS IS RC1, will need shortcut, and RC1 needs to be input
    // Voice 3 (third transistor in schematic):
    RC5PPS = 0x0F;
    RC3PPS = 0x10;
    // Voice 4 (fourth transistor in schematic):
    RC4PPS = 0x11;
    RC2PPS = 0x12;    
    
    RB6PPS = 0x16; // TX2
    
    RX2PPS = 0b001001; // RB1 = RX2
    RX1PPS = 0b010111; // RC7 = RX1-
    
    RB7PPS = 0x00;
    // RB1PPS = 0x00;

    ADCON0bits.FM = 0;
    ADCON0bits.CS = 1; // ADCRC Clock
    ADCON0bits.IC = 0; // Regular mode
    ADCON0bits.ON = 1; // Turn ADC on.
    ANSELA = 0x1F;
    ADPCH = 0x00; //RA0 is first positive input
    ADCON0bits.GO = 1; //Start conversion
            
    DAC_Initialize();
    while(1) {
      if (queuesize > 0) {
        value = getch();
        if (value >= 0x80) {
            // Status byte
            state = value;
            if (value < 0xF0) current_channel = state & 0x0F;
        } else {
            current_note = value;
            current_velocity =  getch();
            switch(state & 0xF0) {
                case 0x90: // Note On
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    if (current_velocity > 0) {
                        add_note(current_note, current_velocity);
                    } else {
                        remove_note(current_note);
                    }
                    break;
                case 0x80: // Note Off
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    remove_note(current_note);
                    break;
                case 0xb0: // CC
                    if (current_note == 64) {
                        if (current_velocity > 64) {
                            pedal = 1;
                        } else {
                            pedal = 0;
                        }
                    } else if (current_note == 1) {
                        midi_pwm = current_velocity << 1;
                    } else if (current_note == 73) {
                        midi_attack = current_velocity << 1;
                    } else if (current_note == 72) {
                        midi_release = current_velocity << 1;
                    } else if (current_note == 75) {
                        midi_decay = current_velocity << 1;
                    } else if (current_note == 79) {
                        midi_sustain = current_velocity << 1;
                    } else {
                        break;
                    }
                    if (lastmidi != 0xb2) {
                       putch2(0xb2);
                       lastmidi = 0xb2;
                    }
                    putch2(current_note);
                    putch2(current_velocity);
                    break;
            }
        }
      }
      if (queue2size > 0) {
        value = getch2();
        if (value >= 0x80) {
            state2 = value;
        } else {
            current_note = value;
            current_velocity =  getch2();
            switch(state2 & 0xF0) {
                case 0x90: // Note On
                    if (current_velocity > 0) {
                        add_note(current_note, current_velocity);
                    } else {
                        remove_note(current_note);
                    }
                    break;
                case 0x80: // Note Off
                    remove_note(current_note);
                    break;
                case 0xb0: // CC
                    if (current_note == 64) {
                        if (current_velocity > 64) {
                            pedal = 1;
                        } else {
                            pedal = 0;
                        }
                    } else if (current_note == 1) {
                        midi_pwm = current_velocity << 1;
                    } else if (current_note == 73) {
                        midi_attack = current_velocity << 1;
                    } else if (current_note == 72) {
                        midi_release = current_velocity << 1;
                    } else if (current_note == 75) {
                        midi_decay = current_velocity << 1;
                    } else if (current_note == 79) {
                        midi_sustain = current_velocity << 1;
                    } else {                    
                        kill_note(current_velocity);
                        break;
                    }
                    if (lastmidi != 0xb2) {
                       putch2(0xb2);
                       lastmidi = 0xb2;
                    }
                    putch2(current_note);
                    putch2(current_velocity);
                    break;
            }
        }
      }
      if (!ADCON0bits.GO) {
          adcresult[currentadc] = ADRESH;
          currentadc++;
          if (currentadc > 4) currentadc = 0;
          ADPCH = currentadc;
          ADCON0bits.GO = 1; // Start next conversion
          switch (currentadc) {
              case 0: // This means a roll over, so it was 4.
                pwm = (midi_pwm != 255) ? midi_pwm : adcresult[4];
                if (pwm != pwmold) {
                  for (unsigned char i = 0; i < OSCS; i++) {
                    toloadpwm[i] = compute_pwm(pwm, loadednote[i]);
                  }
                }
                pwmold = pwm;
                break;
              case 1: // This is 0 (release)
                  value = (midi_release != 255) ? midi_release >> 2 : adcresult[0] >> 2;
                  rt = adsr[value] >> 4;
                  if (pedal) rt = (rt + 1) << 1;
                  rv = (adsr[value] & 0x0f) + 1;
                  break;
              case 2: // This is 1 (sustain)
                  sustain = (midi_sustain != 255) ? midi_sustain : adcresult[1];
                  break;
              case 3: // This is 2 (decay)
                  value = (midi_decay != 255) ? midi_decay >> 2 : adcresult[2] >> 2;
                  dt = adsr[value] >> 4;
                  dv = (adsr[value] & 0x0f) + 1;                  
                  break;
              case 4: // This is 3 (attack)
                  value = (midi_attack != 255) ? midi_attack >> 2 : adcresult[3] >> 2;
                  at = adsr[value] >> 4;
                  av = ((adsr[value] & 0x0f) + 1) << 2;
                  break;             
          }
      }
      if (tickcount != oldtick) {
          oldtick = tickcount;
          maxvel = 0;
          for (unsigned char i = 0; i < OSCS; i++) {
            switch (gate[i]) {
              case 1: // ATTACK
                if (velocity[i] < target[i]) {
                  if (tick[i]) {
                    tick[i]--;
                  } else {
                    tick[i] = at;
                    if (target[i] - velocity[i] < av) {
                      velocity[i] = target[i];
                      tick[i] = 0;
                      gate[i] = 2;
                    } else {
                      velocity[i] = velocity[i] + av;
                    }
                    toloadvelocity[i] = compute_velocity(velocity[i], loadednote[i]);
                  }
                } else {
                  tick[i] = 0;
                  gate[i] = 2;
                }
                break;
              case 2: // DECAY
                if (velocity[i] > sustain) {
                    if (tick[i]) {
                      tick[i]--;
                    } else {
                      tick[i] = dt;
                      if (velocity[i] - sustain < dv) {
                        velocity[i] = sustain;
                        tick[i] = 0;
                        gate[i] = 3;
                      } else {
                        velocity[i] = velocity[i] - dv;
                      }
                      toloadvelocity[i] = compute_velocity(velocity[i], loadednote[i]);
                    }
                } else {
                    tick[i] = 0;
                    gate[i] = 3;
                }
                break;
              case 3: // SUSTAIN
                  value = velocity[i];
                  velocity[i] = sustain;
                  if (velocity[i] > target[i]) velocity[i] = target[i];
                  if (value != velocity[i]) {
                      toloadvelocity[i] = compute_velocity(velocity[i], loadednote[i]);
                  }
                  break;
              case 0: // RELEASE
                if (velocity[i] > 0) {
                  if (tick[i]) {
                    tick[i]--;
                  } else {
                    tick[i] = rt;
                    if (velocity[i] < rv) {
                        velocity[i] = 0;
                        toloadvelocity[i] = 0;
                    } else {
                        velocity[i] = velocity[i] - rv;
                        toloadvelocity[i] = compute_velocity(velocity[i], loadednote[i]);
                    }
                  }
                }
                break;
            }
            if (velocity[i] > maxvel) maxvel = velocity[i];
          }
          if (maxvel > 0) {
              active_output = 1;
          } else if (active_output) {
              midi_pwm = 255;
              midi_attack = 255;
              midi_decay = 255;
              midi_sustain = 255;
              midi_release = 255;
              active_output = 0;
          }
      }
      if (gate[0] || gate[1] || gate[2] || gate[3]) {
        LATBbits.LATB5 = 1;
      } else {
        LATBbits.LATB5 = 0;
      }
      load1();
      load2();
      load3();
      load4();
      load0(maxvel);
    }
}
