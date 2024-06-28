/*
 * File:   vco3.c
 * Author: Maarten Hofman
 *
 * Created on April 28, 2024, 3:28 PM
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

#define _XTAL_FREQ 1000000
#define OSCS 4


const unsigned short lookup[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64808,  // 0-11
    61161, 57736, 54495, 51413, 48543, 45808, 43252, 40815, 38520, 36363, 34316, 32393,  // 12-23 (B0)
    30580, 28859, 27240, 25713, 24271, 22909, 21621, 20407, 19263, 18181, 17160, 16196,  // 24-35 (C1-B1)
    15287, 14429, 13619, 12856, 12133, 11452, 10810, 10203, 9630, 9090, 8580, 9098,  // 36-47 (C2-B2)
    7644, 7215, 6810, 6427, 6067, 5726, 5404, 5101, 4815, 4544, 4289, 4049, // 48-59 (C3-B3)
    3821, 3607, 3404, 3213, 3033, 2862, 2702, 2550, 2407, 2272, 2144, 2024, // 60-71 (C4-B4)
    1910, 1803, 1702, 1606, 1516, 1431, 1350, 1275, 1203, 1135, 1072, 1011, // 72-83 (C5-B5)
    955, 901, 850, 803, 757, 715, 675, 637, 601, 567, 535, 505, // 84-95 (C6-B6)
    477, 450, 425, 401, 378, 357, 337, 318, 300, 283, 267, 252, // 96-107 (C7-B7)
    238, 224, 212, 200, 189, 178, 168, 158, 150, 141, 133, 126, // 108-119 (C8-B8)
    118, 111, 105, 99, 94, 88, 83, 79  // 120-127 (C9-G9)    
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
    DAC1CON = 0b10100000; // EN, auto range, RB7 and RA2, Vdd, Vss
    CPCONbits.CPON = 1;
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

unsigned char queue[16];
unsigned char queueread = 0;
unsigned char queuewrite = 0;
unsigned char queuesize = 0;
unsigned short toloadnote[4] = {0, 0, 0, 0};
unsigned short toloadpwm[4] = {0, 0, 0, 0};
unsigned short loadednote[4] = {0, 0, 0, 0};
unsigned char note[5] = {0, 0, 0, 0};
unsigned char gate[5] = {0, 0, 0, 0};

unsigned char midi;
unsigned char inote;
unsigned char ogate = 0;
unsigned char velocity;
unsigned char pwm = 128;
unsigned char pwmold = 128;
unsigned char clean = 0;

__interrupt() void incoming() {
    queue[queuewrite & 15] = RC1REG;
    queuewrite++;
    queuesize++;
    if (RC1STAbits.OERR) {
        PORTCbits.RC0 = 1;
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
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

inline void load1() {
    if (PWM1CONbits.LD) return;
    if ((!toloadnote[0]) && (!toloadpwm[0])) return;
    if (toloadnote[0]) {
        PWM1PR = toloadnote[0];
        loadednote[0] = toloadnote[0];
    }
    if (toloadpwm[0]) PWM1S1P1 = toloadpwm[0];
    toloadnote[0] = 0;
    toloadpwm[0] = 0;
    PWM1CONbits.LD = 1;
}

inline void load2() {
    if (PWM2CONbits.LD) return;
    if ((!toloadnote[1]) && (!toloadpwm[1])) return;
    if (toloadnote[1]) {
        PWM2PR = toloadnote[1];
        loadednote[1] = toloadnote[1];
    }
    if (toloadpwm[1]) PWM2S1P1 = toloadpwm[1];
    toloadnote[1] = 0;
    toloadpwm[1] = 0;
    PWM2CONbits.LD = 1;
}

inline void load3() {
    if (PWM3CONbits.LD) return;
    if ((!toloadnote[2]) && (!toloadpwm[2])) return;
    if (toloadnote[2]) {
        PWM3PR = toloadnote[2];
        loadednote[2] = toloadnote[2];
    }
    if (toloadpwm[2]) PWM3S1P1 = toloadpwm[2];
    toloadnote[2] = 0;
    toloadpwm[2] = 0;
    PWM3CONbits.LD = 1;
}

inline void load4() {
    if (PWM4CONbits.LD) return;
    if ((!toloadnote[3]) && (!toloadpwm[3])) return;
    if (toloadnote[3]) {
        PWM4PR = toloadnote[3];
        loadednote[3] = toloadnote[3];
    }
    if (toloadpwm[3]) PWM4S1P1 = toloadpwm[3];
    toloadnote[3] = 0;
    toloadpwm[3] = 0;
    PWM4CONbits.LD = 1;
}

inline void clean_oscs() {
    PWM2PR = 1;
    PWM2S1P1 = 1;
    PWM2CONbits.LD = 1;
    PWM3PR = 1;
    PWM3S1P1 = 1;
    PWM3CONbits.LD = 1;
    PWM4PR = 1;
    PWM4S1P1 = 1;
    PWM4CONbits.LD = 1;
}

unsigned short compute_pwm(unsigned short max) {
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

void main(void) {
    init_uart();
    PWM1_Initialize();
    PWM2_Initialize();
    PWM3_Initialize();
    PWM4_Initialize();
    PORTA = 0x00;
    PORTC = 0x00;
    LATA = 0x00;
    LATC = 0x00;
    ANSELA = 0x00;
    ANSELC = 0x00;
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x00;
    RC4PPS = 0x0B;
    RB4PPS = 0x0D;
    RC5PPS = 0x0F;
    RC6PPS = 0x11;
    RC0PPS = 0x00;
    // RB7PPS = 0x00;
        
    TRISCbits.TRISC7 = 1;  // RX
    TRISAbits.TRISA0 = 1;  // Potentiometer

    ADCON0bits.FM = 0;
    ADCON0bits.CS = 1; // ADCRC Clock
    ADCON0bits.IC = 0; // Regular mode
    ADCON0bits.ON = 1; // Turn ADC on.
    ANSELAbits.ANSELA0 = 1;
    ADPCH = 0x00; //RA0 is positive input
    ADCON0bits.GO = 1; //Start conversion
    
    DAC_Initialize();

/*
    while(1) {
        PORTBbits.RB7 = 1;
        DAC1DATL = 64;
    }
    */
    
    while(1) {
      if (queuesize > 2) {
        midi = getch();
        // TODO(add midi off check)
        if (midi == 0x99 || midi == 0x92) {
          inote = getch();
          velocity = getch();
          if (velocity > 0) {
            if (!clean) {
                clean_oscs();
                clean = 1;
            }
            if ((inote != note[0]) && (inote != note[1]) && (inote != note[2])
                    && (inote != note[3])) {
                for (unsigned char i = 0; i < OSCS; i++) {
                    if (!gate[i]) {
                        note[i] = inote;
                        gate[i] = 1;
                        toloadnote[i] = lookup[inote];
                        toloadpwm[i] = compute_pwm(toloadnote[i]);
                        break;
                    }
                }
            }
          } else {
            for (unsigned char i = 0; i < OSCS; i++) {
                if (inote == note[i]) {
                    note[i] = 0;
                    gate[i] = 0;
                }
            }
          }
        }
      }
      load1();
      load2();
      load3();
      load4();
      if (!ogate) {
        if (gate[0] || gate[1] || gate[2] || gate[3]) {
            if (!toloadnote[0] && !toloadnote[1] && !toloadnote[2] && !toloadnote[3]) {
              ogate = 1;
              PORTCbits.RC2 = 1;
            }
        }
      } else {
          if (!gate[0] && !gate[1] && !gate[2] && !gate[3]) {
              PORTCbits.RC2 = 0;
              ogate = 0;
              clean = 0;
          }
      }
      if (!ADCON0bits.GO) {
        pwm = ADRESH;
        if (pwm != pwmold) {
            for (unsigned char i = 0; i < OSCS; i++) {
              toloadpwm[i] = compute_pwm(loadednote[i]);
            }
            pwmold = pwm;
        }
        ADCON0bits.GO = 1; //Start conversion
      }
    }
    return;
}

