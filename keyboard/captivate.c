/*
 * File:   newmain.c
 * Author: Maarten Hofman
 *
 * Created on June 2, 2024, 10:30 AM
 */

// PIC16F18156 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_32MHz// Reset Oscillator Selection bits (HFINTOSC (32 MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
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
#pragma config PPS1WAY = ON     // PPSLOCKED One-Way Set Enable bit (The PPSLOCKED bit can be cleared and set only once after an unlocking sequence is executed; once PPSLOCKED is set, all future changes to PPS registers are prevented)
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
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)

// CONFIG5
#pragma config CP = OFF         // Program Flash Memory Code Protection bit (Program Flash Memory code protection is disabled)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#define _XTAL_FREQ 32000000
#define CHARGE_DELAY 10

unsigned char count = 0;

void PWM1_Initialize(void) {
 PWM1ERS = 0x00; // PWMERS External Reset Disabled;
 PWM1CLK = 0x02; // PWMCLK FOSC;
 PWM1LDS = 0x00; // PWMLDS Autoload disabled;
 PWM1PR = 60;
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
 PWM2PR = 60;
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
}

void putch(unsigned char data) {
  while(!PIR4bits.TX1IF) {          // wait until the transmitter is ready
  }
  TX1REG = data;                     // send one character
}

unsigned int sentkeys = 0;
unsigned int oldkeys = 0;
unsigned int keys = 0;
unsigned char octave = 60;

void on(unsigned int which, unsigned char note) {
    if (keys & which) return;
    keys |= which;
    if (note < 12) {
      putch(0x92);
      putch(note + octave);
      putch(100);
    } else if (note == 12) {
        octave = octave - 12;
        if (octave < 24) octave = 24;
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
        }
        PWM1CONbits.LD = 1;
        PWM2CONbits.LD = 1;
    } else if (note == 13) {
        octave = octave + 12;
        if (octave > 96) octave = 96;
        PWM1S1P1 = octave - 12;
        PWM2S1P1 = octave - 60;
        switch(octave) {
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
    }
}

void off(unsigned int which, unsigned char note) {
    if (!(keys & which)) return;
    keys &= (65535 - which);
    if (note < 12) {
      putch(0x92);
      putch(note + octave);
      putch(0);    
    }
}

void main(void) {

    ANSELC = 0x00;
    ANSELB = 0x00;
    ANSELA = 0x00;
    TRISC = 0;                 // Everything output for now
    TRISA = 255;                 // Everything output for now
    TRISB = 0;                 // Everything output for now
    WPUA = 255;
    RC4PPS = 0x0B;
    RB4PPS = 0x0D;

    TRISCbits.TRISC0 = 1;
    WPUCbits.WPUC0 = 1;
    TRISCbits.TRISC1 = 1;
    WPUCbits.WPUC1 = 1;
    TRISCbits.TRISC2 = 1;
    WPUCbits.WPUC2 = 1;
    TRISCbits.TRISC3 = 1;
    WPUCbits.WPUC3 = 1;
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB3 = 1;
    WPUBbits.WPUB2 = 1;
    WPUBbits.WPUB3 = 1;
    
    //RA4PPS = 0x00;
    //T0CON0bits.EN = 0;
    
    init_uart();    
    PWM1_Initialize();
    PWM2_Initialize();
    
    while(1) {
        // Check C4
        TRISAbits.TRISA0 = 0;  // C4 output
        PORTAbits.RA0 = 0;     // short the capacitor
        __delay_us(CHARGE_DELAY);         // Wait until shorted
        count = 0;
        TRISAbits.TRISA0 = 1;  // Start reading PORTA
        while (!PORTAbits.RA0) count++;
        TRISAbits.TRISA0 = 1;  // Done checking
        if (count > 1) {
            on(1, 0);
        } else {
            off(1, 0);
        }
        TRISAbits.TRISA1 = 0;
        PORTAbits.RA1 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA1 = 1;
        while (!PORTAbits.RA1) count++;
        if (count > 1) {
            on(8, 3);
        } else {
            off(8, 3);
        }
        // Start 2
        TRISAbits.TRISA2 = 0;
        PORTAbits.RA2 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA2 = 1;
        while (!PORTAbits.RA2) count++;
        if (count > 1) {
            on(32, 5);
        } else {
            off(32, 5);
        }
        // End 2
        // Start 3
        TRISAbits.TRISA3 = 0;
        PORTAbits.RA3 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA3 = 1;
        while (!PORTAbits.RA3) count++;
        if (count > 1) {
            on(64, 6);
        } else {
            off(64, 6);
        }
        // End 3
        // Start 4
        TRISAbits.TRISA4 = 0;
        PORTAbits.RA4 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA4 = 1;
        while (!PORTAbits.RA4) count++;
        if (count > 1) {
            on(128, 7);
        } else {
            off(128, 7);
        }
        // End 4
        // Start 5
        TRISAbits.TRISA5 = 0;
        PORTAbits.RA5 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA5 = 1;
        while (!PORTAbits.RA5) count++;
        if (count > 1) {
            on(512, 9);
        } else {
            off(512, 9);
        }
        // End 5
        // Start 6
        TRISAbits.TRISA6 = 0;
        PORTAbits.RA6 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA6 = 1;
        while (!PORTAbits.RA6) count++;
        if (count > 1) {
            on(2048, 11);
        } else {
            off(2048, 11);
        }
        // End 6
        // Start 7
        TRISAbits.TRISA7 = 0;
        PORTAbits.RA7 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISAbits.TRISA7 = 1;
        while (!PORTAbits.RA7) count++;
        if (count > 1) {
            on(256, 8);
        } else {
            off(256, 8);
        }
        // End 7
        // Start RC0
        TRISCbits.TRISC0 = 0;
        PORTCbits.RC0 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISCbits.TRISC0 = 1;
        while (!PORTCbits.RC0) count++;
        if (count > 1) {
            on(1024, 10);
        } else {
            off(1024, 10);
        }
        // End RC0
        // Start RC1
        TRISCbits.TRISC1 = 0;
        PORTCbits.RC1 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISCbits.TRISC1 = 1;
        while (!PORTCbits.RC1) count++;
        if (count > 1) {
            on(16, 4);
        } else {
            off(16, 4);
        }
        // End RC1
        // Start RC2
        TRISCbits.TRISC2 = 0;
        PORTCbits.RC2 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISCbits.TRISC2 = 1;
        while (!PORTCbits.RC2) count++;
        if (count > 1) {
            on(2, 1);
        } else {
            off(2, 1);
        }
        // End RC2
        // Start RC3
        TRISCbits.TRISC3 = 0;
        PORTCbits.RC3 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISCbits.TRISC3 = 1;
        while (!PORTCbits.RC3) count++;
        if (count > 1) {
            on(4, 2);
        } else {
            off(4, 2);
        }
        // End RC3
        // button 1
        TRISBbits.TRISB3 = 0;
        PORTBbits.RB3 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISBbits.TRISB3 = 1;
        while (!PORTBbits.RB3) count++;
        if (count > 1) {
            on(4096, 12);
        } else {
            off(4096, 12);
        }
        // end button 1
        // button 2
        TRISBbits.TRISB2 = 0;
        PORTBbits.RB2 = 0;
        __delay_us(CHARGE_DELAY);
        count = 0;
        TRISBbits.TRISB2 = 1;
        while (!PORTBbits.RB2) count++;
        if (count > 1) {
            on(8192, 13);
        } else {
            off(8192, 13);
        }
        // end button 2
     }
    return;
}
