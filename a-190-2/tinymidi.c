/*
 * File:   tinymidi.c
 * Author: Maarten Hofman
 *
 * Created on July 11, 2025, 2:35 PM
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

/**
 * RA0: modulate PWM
 * RA4: MIDI note PWM
 * RA5: MIDI velocity PWM
 * RA2: Gate out
 * RA3: MIDI in
 * RA1: pitchbend PWM
 */

#include <xc.h>

#define _XTAL_FREQ 32000000


unsigned char queue[16];
unsigned char queueread = 0;
unsigned char queuewrite = 0;
unsigned char queuesize = 0;

unsigned char read(unsigned char location) {
  // This code block will read 1 word (byte) of DFM
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
  NVMADRL = location;
  NVMCON1bits.RD = 1; // Initiate read cycle
  return NVMDATL;
}

void write(unsigned char location, unsigned char v) {
  // Wait for write to complete.
  while (NVMCON1bits.WR);
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
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

void PWM1_Initialize(void) {
 PWM1ERS = 0x00; // PWMERS External Reset Disabled;
 PWM1CLK = 0x02; // PWMCLK FOSC;
 PWM1LDS = 0x00; // PWMLDS Autoload disabled;
 PWM1PRL = 0x7F;
 PWM1PRH = 0x00;
 PWM1CPRE = 0x00; // PWMCPRE No prescale;
 PWM1PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM1GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 // PWMS1P1IF PWM1 output match did not occur;
 PWM1GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM1S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM1S1P1 = 0x0000;
 PWM1S1P2 = 0x0000;
 PWM1CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM1CONbits.LD = 1;
 RA5PPS = 0x0B;
 RA4PPS = 0x0C;
}

void PWM2_Initialize(void) {
 PWM2ERS = 0x00; // PWMERS External Reset Disabled;
 PWM2CLK = 0x02; // PWMCLK FOSC;
 PWM2LDS = 0x00; // PWMLDS Autoload disabled;
 PWM2PRL = 0xFF;
 PWM2PRH = 0x03;
 PWM2CPRE = 0x00; // PWMCPRE No prescale;
 PWM2PIPOS = 0x00; // PWMPIPOS No postscale;
 PWM2GIR = 0x00; // PWMS1P2IF PWM2 output match did not occur;
 PWM2GIE = 0x00; // PWMS1P2IE disabled; PWMS1P1IE disabled;
 PWM2S1CFG = 0x00; // PWMPOL2 disabled; PWMPOL1 disabled; PWMPPEN
 // disabled; PWMMODE PWMOUT1,PWMOUT2 in left
 // aligned mode
 PWM2S1P1 = 0x0000;
 PWM2S1P2 = 0x0000;
 PWM2CON = 0x80; // PWMEN enabled; PWMLD disabled; PWMERSPOL
 // disabled; PWMERSNOW disabled;
 PWM2CONbits.LD = 1;
 RA1PPS = 0x0D;
 RA0PPS = 0x0E;
}

inline void load1(unsigned char note, unsigned char velocity) {
    while (PWM1CONbits.LD) {
    }
    PWM1S1P1L = note;
    PWM1S1P2L = velocity;
    PWM1CONbits.LD = 1;
}

inline void load2(unsigned short pitchbend, unsigned short modulation) {
    while (PWM2CONbits.LD) {
    }
    PWM2S1P1 = (16383 - pitchbend) >> 4;
    PWM2S1P2 = modulation << 3;
    PWM2CONbits.LD = 1;
}

unsigned char getch() {
    unsigned char data;
    
    while(!queuesize) {
    }
    data = queue[queueread & 15];
    queueread++;
    queuesize--;
    return data;
}

inline unsigned char peek() {
    if (!queuesize) return 0;
    return queue[queueread & 15];

}

__interrupt() void incoming() {
    queue[queuewrite & 15] = RC1REG;
    queuewrite++;
    queuesize++;
    if (RC1STAbits.OERR) {
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
    }    
}

void init_uart(void) {
  SP1BRG = 15;                        // 3 at 8Mhz, 9 at 20Mhz (from 4 to 10), 15 at 32 Mhz
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
  
  // Setup interrupt
  PIE4bits.RC1IE = 1;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;
  RX1PPS = 3;
}

void main(void) {
    unsigned char state = 0;
    unsigned char raw_note = 0;
    unsigned char value = 0;
    unsigned short pitchbend = 8192;
    unsigned short modulation = 0;
    unsigned char lsb, msb;
    unsigned char gate_on = 0;
    unsigned char note_offset = 0;
    unsigned char selected_channel = 0xFF; // Accept all channels initially
    unsigned char current_channel = 0xFF;  // Track last seen channel
    unsigned char selected_cc = 0xFF;
    unsigned char current_cc = 0xFF;

    INLVLA = 0;
    
    init_uart();
    PWM1_Initialize();
    PWM2_Initialize();
    PORTA = 0x00;
    LATA = 0x00;
    ANSELA = 0x00;
    TRISA = 0x00;
    WPUAbits.WPUA2 = 0;
    PORTAbits.RA2 = 0;
    LATAbits.LATA2 = 0;
    TRISAbits.TRISA2 = 1;


    load2(pitchbend, modulation);
    
    // Read ROM values
    note_offset = read(0);
    selected_channel = read(1);
    selected_cc = read(2);
    
    while (1) {
        value = getch();  // Wait for next MIDI byte
        if (value >= 0x80) {
            // Status byte
            state = value;
            if (value < 0xF0) current_channel = state & 0x0F;
        } else {
            switch(state & 0xF0) {
                case 0x90: // Note On
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    raw_note = value;         // Raw note for offset learn
                    current_cc = selected_cc;
                    value = getch();          // Get velocity
                    if (value > 0) {
                        load1(raw_note - note_offset, value); // Apply offset here
                        LATAbits.LATA2 = 1;    // Gate high
                        TRISAbits.TRISA2 = 0;  // Output mode
                        gate_on = 1;
                    } else {
                        PORTAbits.RA2 = 0;
                        LATAbits.LATA2 = 0;
                        TRISAbits.TRISA2 = 1;  // Input mode (gate off)
                        gate_on = 0;
                    }
                    break;
                case 0x80: // Note Off
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    value = getch();          // Discard velocity
                    TRISAbits.TRISA2 = 1;
                    PORTAbits.RA2 = 0;
                    LATAbits.LATA2 = 0;
                    gate_on = 0;
                    break;
                case 0xE0: // Pitch Bend
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    lsb = value;
                    msb = getch();
                    pitchbend = ((unsigned short)msb << 7) | lsb;
                    load2(pitchbend, modulation);
                    break;
                case 0xB0: // Control Change
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    current_cc = value;
                    raw_note = note_offset;
                    modulation = getch();
                    if (current_cc == selected_cc || selected_cc == 0xFF) {
                        load2(pitchbend, modulation);
                    }
                    break;
            }
        }

        if (!gate_on) {
            if (PORTAbits.RA2) {
              if (note_offset != raw_note) {
                  note_offset = raw_note; // Store raw note that was last seen
                  write(0, note_offset);
              }
              if (selected_channel != current_channel) {
                  selected_channel = current_channel;
                  write(1, selected_channel);
              }
              if (selected_cc != current_cc) {
                  selected_cc = current_cc;
                  write(2, selected_cc);
              }
            }
        }
    }
}