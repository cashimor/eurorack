/*
 * File:   tinyadsr.c
 * Author: Maarten Hofman
 *
 * Created on February 24, 2024, 8:45 AM
 */

// PIC16F18115 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_32MHz// Reset Oscillator Selection bits (HFINTOSC (32 MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config VDDAR = HI       // VDD Range Analog Calibration Selection bit (Internal analog systems are calibrated for operation between VDD = 2.3 - 5.5V)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2
#pragma config MCLRE = INTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR is port-defined function; If LVP = 1, RA3 pin function is MCLR)
#pragma config PWRTS = PWRT_OFF // Power-up Timer Selection bits (PWRT is disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // DISABLE BROWN OUT RESET
#pragma config DACAUTOEN = OFF  // DAC Buffer Automatic Range Select Enable bit (DAC Buffer reference range is determined by the REFRNG bit)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection bit (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD module is disabled; ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = OFF    // PPSLOCKED One-Way Set Enable bit (The PPSLOCKED bit can be set and cleared as needed (unlocking sequence is required))
#pragma config STVREN = OFF     // DISABLE STACK OVERFLOW RESET
#pragma config DEBUG = OFF      // Background Debugger (Background Debugger disabled)

// CONFIG3
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF        // WATCHDOG TIMER DISABLED
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

/**
 * RA0: Output
 * RA4: Attack
 * RA5: Decay
 * RA2: Gate
 * RA3: RESET
 * RA1: Sustain?
 */
#include <xc.h>

#define _XTAL_FREQ 32000000

const unsigned char lookup[157] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
                                   2, 2, 2, 2, 2,
                                   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                   4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                   5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                   6, 6, 6, 6, 6, 6, 6,
                                   7, 7, 7, 7, 7, 7, 7,
                                   8, 8, 8, 8, 8, 8,
                                   9, 9, 9, 9, 9, 9,
                                   10, 10, 10, 10,
                                   11, 11, 11, 11,
                                   12, 12, 12, 12, 12,
                                   13, 13, 13,
                                   14, 14, 14, 14,
                                   15, 15, 15, 15,
                                   16, 16, 16,
                                   17, 17,
                                   18, 18, 18,
                                   19, 19,
                                   20, 20, 20,
                                   21, 21, 21,
                                   22, 22,
                                   23, 23,
                                   24, 24,
                                   25, 25,
                                   26, 26,
                                   27,
                                   28, 28,
                                   29, 29,
                                   30, 30,
                                   31,
                                   32, 32,
                                   33,
                                   34, 34,
                                   35, 35,
                                   36,
                                   37,
                                   38,
                                   39, 39,
                                   40,
                                   41,
                                   42,
                                   43, 43};

void PWM1_Initialize(void) {
 PWM1ERS = 0x00; // PWMERS External Reset Disabled;
 PWM1CLK = 0x02; // PWMCLK FOSC;
 PWM1LDS = 0x00; // PWMLDS Autoload disabled;
 PWM1PRL = 0xFF;
 PWM1PRH = 0xFF;
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

unsigned short value = 0;
unsigned char attack = 0;
unsigned char attackspeed = 1;
unsigned char decay = 1;
unsigned char decayspeed = 1;
unsigned short compare = 0;
unsigned char current = 0;
unsigned short release = 0;
unsigned char ih = 0;

unsigned char speed = 0;
unsigned char count = 0;

__interrupt() void edge() {
    release = value;
    value = compare;
    PIR0 = 0;
    ih = 1;
}

inline void load(unsigned short what) {
  if (!PWM1CONbits.LD) {
    PWM1S1P1 = what;
    PWM1CONbits.LD = 1;
  }
}

inline void translate(unsigned char what) {
  what = 255 - what;
  if (what < 34) {
    speed = 1;
    count = 64 - what;
    return;
  }
  count = 64;
  if (what > 191) {
      speed = what - 148;
      return;
  }
  speed = lookup[what - 34];
  while (what > 34 && lookup[what - 35] == speed) {
      count--;
      what--;
  }
}

void main(void) {
    PWM1_Initialize();
    PORTA = 0x00;
    LATA = 0x00;
    ANSELA = 0x00;
    TRISA = 0x00;
    RA0PPS = 0x0B;
    
    //Setup ADC
    ADCON0bits.FM = 0;
    ADCON0bits.CS = 1; //ADCRC Clock
    ADCON0bits.IC = 0; //Regular mode
    ADPCH = 0x04; //RA4 is positive input
    TRISAbits.TRISA4 = 1; // Set RA4 to input (attack)
    TRISAbits.TRISA5 = 1; // Set RA5 to input (decay)
    TRISAbits.TRISA1 = 1; // Set RA1 to input (sustain)
    TRISAbits.TRISA2 = 1; // Set RA2 to input
    
    ANSELAbits.ANSELA4 = 1; // Select RA4 (attack)
    ANSELAbits.ANSELA5 = 1; // Select RA5 (decay)
    ANSELAbits.ANSELA1 = 1; // Select RA1 (sustain)
    
    ADACQ = 32; //Set acquitisition time
    ADCON0bits.ON = 1; //Turn ADC O
    ADREF = 0; // VDD
    while(1) {
      while (!PORTAbits.RA2) {
          value = 0;
          load(value);
      }
      
      // ATTACK SECTION
      ADPCH = 0x04; //RA4 is positive input
      ADCON0bits.GO = 1; //Start conversion
      while (ADCON0bits.GO); //Wait for conversion done
      translate(ADRESH); //Read result
      attack = count;
      attackspeed = speed;
      compare = 65534 - attackspeed;
      ih = 0;
      PIE0 = 0b00000001;
      INTCON = 0b10000000;
      while (value < compare) {
        load(value);
        value = value + attackspeed;
        current = attack;
        while (current > 0) current--;          
      }
      
      // DECAY SECTION
      ADPCH = 0x05; //RA5 is positive input
      ADCON0bits.GO = 1; //Start conversion
      while (ADCON0bits.GO); //Wait for conversion done
      translate(ADRESH); //Read result      
      decay = count;
      decayspeed = speed;
      
      ADPCH = 0x01; // RA1 is positive input (sustain)
      ADCON0bits.GO = 1; //Start conversion
      while (ADCON0bits.GO); //Wait for conversion done
      compare = ADRES;
      if (compare < decayspeed) {
          compare = decayspeed;
      }
      value = 65535;
      if (!PORTAbits.RA2) {
          value = compare;
      }
      while(value > compare) {
        load(value);
        value = value - decayspeed;
        current = decay;
        while (current > 0) current--;
      }
      
      // SUSTAIN SECTION
      while (PORTAbits.RA2) {
        ADPCH = 0x01; // RA1 is positive input (sustain)
        ADCON0bits.GO = 1; //Start conversion
        while (ADCON0bits.GO); //Wait for conversion done
        value = ADRES;
        load(value);
        release = value;
      }

      // RELEASE SECTION
      compare = decayspeed;
      value = release;
      ih = 0;
      PIE0 = 0b00000001;
      INTCON = 0b10000001;
      while(value > compare) {
        load(value);
        value = value - decayspeed;
        current = decay;
        while (current > 0) current--;
      }
      if (ih) {
          value = release;
      } else {
          value = 0;
      }
      while (!PORTAbits.RA2) {
          load(value);
      }
    }
    while(1) {
    }
}

