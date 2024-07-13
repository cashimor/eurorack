/* 
 * File:   uart.h
 * Author: Maarten Hofman
 * Comments: PIC16F18156 only so far.
 * Revision history: 20240705
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef UART_H
#define	UART_H
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

unsigned char queue[16];
unsigned char queueread = 0;
unsigned char queuewrite = 0;
unsigned char queuesize = 0;

unsigned char getch() {
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

inline unsigned char peek() {
    if (!queuesize) return 0;
    return queue[queueread & 15];
}
#endif

