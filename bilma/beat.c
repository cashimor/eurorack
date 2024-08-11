/*
 * File:   beat.c
 * Author: Maarten Hofman
 *
 * Created on August 9, 2024
 */

// CONFIG
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select bit (MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Detect (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdlib.h>

#define _XTAL_FREQ 8000000

// Funk
__EEPROM_DATA(5, 0, 5, 0, 6, 0, 4, 0);
__EEPROM_DATA(4, 16,5,16, 6, 0, 8, 0);
// Rock
__EEPROM_DATA(5, 0, 4, 0, 6, 0, 4, 0);
__EEPROM_DATA(5, 1, 4, 0, 6, 0, 4, 0);
// Jazz
__EEPROM_DATA(8, 0,10,12, 0, 8, 8, 0);
__EEPROM_DATA(8,12, 0, 9, 8, 0,10,12);
__EEPROM_DATA(0, 9,10, 0, 8,12, 0, 8);
// Hip hop
__EEPROM_DATA(5, 0, 4, 1, 6, 0, 4, 0);
__EEPROM_DATA(4, 0, 5, 0, 6, 0, 4, 0);
__EEPROM_DATA(5, 0, 4, 1, 6, 0, 4, 0);
__EEPROM_DATA(4, 0, 5, 0, 6, 0, 4, 2);
// House
__EEPROM_DATA(3, 0,12, 0, 7, 0,12, 0);
__EEPROM_DATA(5, 0,12, 0, 7, 0,12, 2);
// Drum'n'base
__EEPROM_DATA(5, 0, 4, 0, 6, 0, 4, 4);
__EEPROM_DATA(4, 4, 4, 1, 4, 2, 4, 0);
// Trap
__EEPROM_DATA(5, 0, 4, 0, 4, 0, 4, 0);
__EEPROM_DATA(20,0, 4, 0, 5, 0, 4, 0);
__EEPROM_DATA(4, 0, 4, 0, 5, 0, 4, 4);
__EEPROM_DATA(20,0, 4, 0, 6, 0, 6, 0);
// My beat
__EEPROM_DATA(1,4,4,4,3,4,4,4);
__EEPROM_DATA(5,4,8,4,7,4,0,20);
__EEPROM_DATA(5,0,4,4,3,4,4,4);
__EEPROM_DATA(1,4,4,4,15,0,12,4);
__EEPROM_DATA(1,4,4,4,3,4,4,4);
__EEPROM_DATA(5,4,0,4,7,20,4,4);
__EEPROM_DATA(5,0,4,4,7,4,4,0);
__EEPROM_DATA(5,4,4,8,7,12,6,4);

unsigned char read(unsigned char location) {
  EEADR = location;
  EECON1bits.RD = 1;
  return EEDAT;
}

typedef union {
    struct {
        unsigned kick:1;
        unsigned snare:1;
        unsigned hihat:1;
        unsigned open:1;
        unsigned clap:1;
        unsigned add1:1;
        unsigned add2:1;
    };
    uint8_t byte;
} drum_t;

drum_t drums;

unsigned char pattern = 0;
unsigned char length = 15;

void input0() {
  unsigned char result;
  ADCON0 = 0b00000001;
  ADCON0bits.GO_nDONE = 1;
  while(ADCON0bits.GO_nDONE) {
    // loop
  }
  result = ADRESH;
  result = result >> 5;
  switch(result) {
      case 0:
          pattern = 0;
          length = 15;
          break;
      case 1:
          pattern = 16;
          length = 15;
          break;
      case 2:
          pattern = 32;
          length = 23;
          break;
      case 3:
          pattern = 56;
          length = 31;
          break;
      case 4:
          pattern = 88;
          length = 15;
          break;
      case 5:
          pattern = 104;
          length = 15;
          break;
      case 6:
          pattern = 120;
          length = 31;
      default:
          pattern = 152;
          length = 63;
  }
}

void input1() {
  unsigned char result;
  
  ADCON0 = 0b00000101;
  ADCON0bits.GO_nDONE = 1;
  while(ADCON0bits.GO_nDONE) {
    // loop
  }
  result = ADRESH;
  if (length == 63) {
      result = result >> 2;
  } else if (length == 15) {
      result = result >> 4;
  } else {
    result = result >> 3;
  }
  if (result > length) result = length;
  if (result == 62) result = 63;
  length = result;
}

void main(void) {
  OSCCON = 0b01110110;
  ANSEL = 0b00000011;   // All digital, except RA0 and RA1
  TRISC = 0b00000000; // PORTC All Output
  TRISA = 0b00001011; // PORTA All Input except RA0 and RA1 and RA3.
  CMCON0 = 7;  // No comparators
  ADCON0 = 0b00000001;
  ADCON1 = 0b01110000;

  unsigned char clock;
  unsigned char changed = 0;
  unsigned char location = 0;

  input0(); // Pattern
  input1(); // Length
  while(1) {
      clock = PORTAbits.RA3; 
      if (clock == changed) {
          continue;
      }
      changed = clock;
      if (!clock) {
          drums.byte = 0;
      } else {
          location = location + 1;
          if (location > length) location = 0;
          drums.byte = read(location + pattern);
      }
      
      PORTAbits.RA2 = drums.hihat;
      PORTCbits.RC2 = drums.add1;
      PORTAbits.RA5 = drums.snare;
      PORTCbits.RC3 = drums.clap;
      PORTAbits.RA4 = drums.kick;
      PORTCbits.RC1 = drums.add2;
      PORTCbits.RC0 = drums.open;
      input0();
      input1();
  }
  return;
}

