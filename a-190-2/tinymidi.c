/*
 * File:   tinymidi.c
 * Author: Maarten Hofman
 *
 * Created on July 11, 2025, 2:35 PM
 *
 * This file contains the firmware for a simple MIDI to CV (Control Voltage)
 * converter using a PIC16F18115 microcontroller.
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
 * @brief Pinout configuration
 * RA0: PWM output for modulation
 * RA1: PWM output for pitch bend
 * RA2: Gate output signal
 * RA3: MIDI input (UART RX)
 * RA4: PWM output for MIDI note
 * RA5: PWM output for MIDI velocity
 */

#include <xc.h>

#define _XTAL_FREQ 32000000

// MIDI receive queue
unsigned char queue[16];
unsigned char queueread = 0;
unsigned char queuewrite = 0;
unsigned char queuesize = 0;

/**
 * @brief Reads a byte from the Data Flash Memory (DFM).
 * @param location The address to read from (0x00-0xFF).
 * @return The data byte read from the specified location.
 */
unsigned char read(unsigned char location) {
  // This code block will read 1 word (byte) of DFM
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
  NVMADRL = location;
  NVMCON1bits.RD = 1; // Initiate read cycle
  return NVMDATL;
}

/**
 * @brief Writes a byte to the Data Flash Memory (DFM).
 * @param location The address to write to (0x00-0xFF).
 * @param v The data byte to write.
 */
void write(unsigned char location, unsigned char v) {
  // Wait for any previous write to complete.
  while (NVMCON1bits.WR);
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
  NVMADRL = location;
  NVMDATL = v;
  NVMCON1bits.WREN = 1; // Allows program/erase cycles
  INTCONbits.GIE = 0; // Disable interrupts for unlock sequence
  NVMCON2 = 0x55; // Perform required unlock sequence
  NVMCON2 = 0xAA;
  NVMCON1bits.WR = 1; // Begin program/erase cycle
  INTCONbits.GIE = 1; // Restore interrupt enable bit value
  NVMCON1bits.WREN = 0; // Disable program/erase
  // Note: Write errors (WRERR) are intentionally ignored.
  NVMCON1bits.WRERR = 0;
}

/**
 * @brief Initializes PWM module 1 for Note and Velocity CV generation.
 * PWM1 is configured to output two PWM signals on RA4 (Note) and RA5 (Velocity).
 */
void PWM1_Initialize(void) {
 PWM1ERS = 0x00; // PWMERS External Reset Disabled;
 PWM1CLK = 0x02; // PWMCLK FOSC;
 PWM1LDS = 0x00; // PWMLDS Autoload disabled;
 PWM1PRL = 0x7F; // 7-bit resolution for MIDI note/velocity
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
 RA5PPS = 0x0B; // RA5 -> PWM1OUT2 (Velocity)
 RA4PPS = 0x0C; // RA4 -> PWM1OUT1 (Note)
}

/**
 * @brief Initializes PWM module 2 for Pitch Bend and Modulation CV generation.
 * PWM2 is configured to output two PWM signals on RA1 (Pitch Bend) and RA0 (Modulation).
 */
void PWM2_Initialize(void) {
 PWM2ERS = 0x00; // PWMERS External Reset Disabled;
 PWM2CLK = 0x02; // PWMCLK FOSC;
 PWM2LDS = 0x00; // PWMLDS Autoload disabled;
 PWM2PRL = 0xFF; // 10-bit resolution for pitchbend/modulation
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
 RA1PPS = 0x0D; // RA1 -> PWM2OUT1 (Pitchbend)
 RA0PPS = 0x0E; // RA0 -> PWM2OUT2 (Modulation)
}

/**
 * @brief Loads new duty cycle values into PWM1.
 * @param note The MIDI note number (0-127).
 * @param velocity The MIDI velocity (0-127).
 */
inline void load1(unsigned char note, unsigned char velocity) {
    // Wait for previous load to complete
    while (PWM1CONbits.LD) {
    }
    PWM1S1P1L = note;
    PWM1S1P2L = velocity;
    PWM1CONbits.LD = 1; // Trigger load of new duty cycle values
}

/**
 * @brief Loads new duty cycle values into PWM2.
 * @param pitchbend The 14-bit MIDI pitch bend value (0-16383).
 * @param modulation The 7-bit MIDI modulation value (0-127).
 */
inline void load2(unsigned short pitchbend, unsigned short modulation) {
    // Wait for previous load to complete
    while (PWM2CONbits.LD) {
    }
    // Note: Pitchbend is inverted here
    PWM2S1P1 = (16383 - pitchbend) >> 4;
    PWM2S1P2 = modulation << 3;
    PWM2CONbits.LD = 1; // Trigger load of new duty cycle values
}

/**
 * @brief Gets a byte from the MIDI receive queue.
 * This function will block until a byte is available.
 * @return The byte from the queue.
 */
unsigned char getch() {
    unsigned char data;
    
    // Wait for data to be available in the queue
    while(!queuesize) {
    }

    // Disable interrupts to safely access the queue
    INTCONbits.GIE = 0;
    data = queue[queueread & 15];
    queueread++;
    queuesize--;
    // Re-enable interrupts
    INTCONbits.GIE = 1;

    return data;
}

/**
 * @brief Peeks at the next byte in the MIDI receive queue without removing it.
 * @return The next byte in the queue, or 0 if the queue is empty.
 */
inline unsigned char peek() {
    unsigned char data = 0;

    // Disable interrupts for safe access
    INTCONbits.GIE = 0;
    if (queuesize > 0) {
        data = queue[queueread & 15];
    }
    // Re-enable interrupts
    INTCONbits.GIE = 1;

    return data;
}

/**
 * @brief Interrupt Service Routine for UART receive.
 * This function is called when a byte is received via UART.
 * It places the received byte into a circular buffer (queue).
 */
__interrupt() void incoming() {
    // If the queue is not full, add the new byte.
    if (queuesize < 16) {
        queue[queuewrite & 15] = RC1REG;
        queuewrite++;
        queuesize++;
    }
    // If the queue is full, the new byte is discarded.

    // Handle UART Overrun Error
    if (RC1STAbits.OERR) {
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
    }    
}

/**
 * @brief Initializes the UART for MIDI communication (31250 baud).
 */
void init_uart(void) {
  // For 31250 baud rate with 32MHz FOSC, SP1BRG should be 63
  // Formula: SPBRG = (FOSC / (16 * BaudRate)) - 1
  // SPBRG = (32,000,000 / (16 * 31250)) - 1 = 64 - 1 = 63
  // The original value of 15 is for a different FOSC or baud rate.
  // Assuming 31250 baud is the target for MIDI.
  SP1BRG = 63;
  SP1BRGH = 0;
  TX1STAbits.TXEN = 1;               // enable transmitter
  TX1STAbits.SYNC = 0;               // async mode
  TX1STAbits.BRGH = 0;               // low speed
  RC1STAbits.RX9 = 0;                // 8-bit reception
  RC1STAbits.ADDEN = 0;
  RC1STAbits.SPEN = 1;               // enable serial port
  RC1STAbits.CREN = 1;               // enable receiver
  BAUD1CONbits.BRG16 = 0;            // 8-bit baud rate generator
  BAUD1CONbits.ABDEN = 0;
  
  // Setup UART Receive Interrupt
  PIE4bits.RC1IE = 1;
  INTCONbits.PEIE = 1;               // Enable peripheral interrupts
  INTCONbits.GIE = 1;                // Enable global interrupts

  // PPS setup for UART RX
  RX1PPS = 3; // RA3 -> RX
}

/**
 * @brief Main application entry point.
 */
void main(void) {
    // --- Variable Declarations ---
    unsigned char state = 0;        // Current MIDI status byte
    unsigned char raw_note = 0;     // Last received note number (for learn mode)
    unsigned char value = 0;
    unsigned short pitchbend = 8192; // Pitchbend value, centered at 8192
    unsigned short modulation = 0;   // Modulation value
    unsigned char lsb, msb;
    unsigned char gate_on = 0;      // Flag to indicate if the gate is currently high

    // Variables for learnable settings
    unsigned char note_offset = 0;      // Note offset for calibration
    unsigned char selected_channel = 0xFF; // MIDI channel to respond to (0xFF for all)
    unsigned char current_channel = 0xFF;  // Track last seen channel
    unsigned char selected_cc = 0xFF;      // CC number to respond to (0xFF for all)
    unsigned char current_cc = 0xFF;       // Last seen CC number

    // --- Initialization ---
    INLVLA = 0; // Use TTL input levels
    
    init_uart();
    PWM1_Initialize();
    PWM2_Initialize();

    // Configure Gate Pin (RA2)
    PORTA = 0x00;
    LATA = 0x00;
    ANSELA = 0x00;
    TRISA = 0x00;
    WPUAbits.WPUA2 = 0;
    PORTAbits.RA2 = 0;
    LATAbits.LATA2 = 0;
    TRISAbits.TRISA2 = 1; // Set RA2 as input (high-impedance, gate off)


    // Initialize CV outputs
    load2(pitchbend, modulation);
    
    // Load saved settings from DFM
    note_offset = read(0);
    selected_channel = read(1);
    selected_cc = read(2);
    
    // --- Main Loop ---
    while (1) {
        value = getch();  // Wait for and get next MIDI byte

        if (value >= 0x80) { // Is it a status byte?
            state = value;
            // Update current channel if it's a channel voice message
            if (value < 0xF0) {
                current_channel = state & 0x0F;
            }
        } else { // It's a data byte
            // Handle running status
            switch(state & 0xF0) {
                case 0x90: // Note On
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    raw_note = value;         // Raw note for offset learn
                    value = getch();          // Get velocity
                    if (value > 0) { // Note On with velocity > 0
                        load1(raw_note - note_offset, value); // Apply offset here
                        LATAbits.LATA2 = 1;    // Gate high
                        TRISAbits.TRISA2 = 0;  // Output mode
                        gate_on = 1;
                    } else { // Note On with velocity 0 is a Note Off
                        PORTAbits.RA2 = 0;
                        LATAbits.LATA2 = 0;
                        TRISAbits.TRISA2 = 1;  // Input mode (gate off)
                        gate_on = 0;
                    }
                    break;
                case 0x80: // Note Off
                    if (selected_channel != 0xFF && current_channel != selected_channel) break;
                    value = getch();          // Discard velocity
                    TRISAbits.TRISA2 = 1;     // Set pin to input (gate off)
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
                    // This prevents re-learning note offset when learning CC
                    raw_note = note_offset;
                    modulation = getch();
                    if (current_cc == selected_cc || selected_cc == 0xFF) {
                        load2(pitchbend, modulation);
                    }
                    break;
            }
        }

        // --- Learn Mode Logic ---
        // If gate is off and the learn button (on RA2) is pressed...
        if (!gate_on) {
            if (PORTAbits.RA2) {
              // ...save the last seen parameters.
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