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
}

void putch(unsigned char data) {
  while(!PIR4bits.TX1IF) {          // wait until the transmitter is ready
  }
  TX1REG = data;                     // send one character
}

unsigned int keys;
unsigned char octave = 60;

void on(unsigned int which, unsigned char note) {
    if (keys & which) return;
    keys |= which;
    putch(0x92);
    putch(note + octave);
    putch(100);
}

void off(unsigned int which, unsigned char note) {
    if (!(keys & which)) return;
    keys &= (65535 - which);
    putch(0x92);
    putch(note + octave);
    putch(0);    
}

void main(void) {

    ANSELC = 0x00;
    ANSELA = 0x00;
    TRISC = 0;                 // Everything output for now
    TRISA = 0;                 // Everything output for now
    TRISB = 0;                 // Everything output for now
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    TRISAbits.TRISA2 = 1;
    WPUAbits.WPUA0 = 1;        // Weak pullup for charging capacitor
    WPUAbits.WPUA1 = 1;
    WPUAbits.WPUA2 = 1;
    
    init_uart();    
    
    while(1) {
        // Check C4
        TRISAbits.TRISA0 = 0;  // C4 output
        PORTAbits.RA0 = 0;     // short the capacitor
        __delay_us(10);         // Wait until shorted
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
        __delay_us(10);
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
        __delay_us(10);
        count = 0;
        TRISAbits.TRISA2 = 1;
        while (!PORTAbits.RA2) count++;
        if (count > 1) {
            on(32, 5);
        } else {
            off(32, 5);
        }
        // End 2
        PORTCbits.RC4 = 0;
        PORTBbits.RB4 = 0;
        if (count == 1 || count == 3) PORTCbits.RC4 = 1;
        if (count == 2 || count == 3) PORTBbits.RB4 = 1;
        __delay_ms(100);
    }
    return;
}
