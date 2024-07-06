/* 
 * File:   eeprom.h
 * Author: Maarten Hofman
 * Comments: PIC16F18156 only so far.
 * Revision history: 20240705
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef EEPROM_H
#define	EEPROM_H
unsigned char read(unsigned char location) {
  // This code block will read 1 word (byte) of DFM
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x070;
  NVMADRL = location;
  NVMCON1bits.RD = 1; // Initiate read cycle
  return NVMDATL;
}

void write(unsigned char location, unsigned char v) {
  // Wait for write to complete.
  while (NVMCON1bits.WR);
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
#endif

