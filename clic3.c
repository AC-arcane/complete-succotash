//Function code for 7 segment displays, switches and LEDs
// Simplest form of function for these devices.
// In the initial version of CLIC3 the Keypad operations are STUBS CAM 2014-06-02
// Conforms to MISRA C required rules except 109, 110
// MSP430 version
// C.A.Maynard 20140530 v3.00
// Updated 7 segment C.A,Maynard 20140627 v3.01
// ************************************************************************

#include "clic3.h"
// ************************************************************************
ui_16 BusAddress,BusData;
 // The bridge between Assembler code and C code.

// ************************************************************************

// General Interface Initialisation functions
void Initial(void);
void BusRead(void);
void BusWrite(void);
// ************************************************************************

void switchesInit(void) {
}

// Returns true if a new value has been found
// Simplest version assumes there is always a new value.!
enum bool switchesGet(uc_8 *value){
    BusAddress=SwitchesAddr;
    BusRead();
    *value = BusData;
    return true;
}
// ************************************************************************

void LEDsInit(void) {
}

// Returns true if a new value can be stored in the interface
// As there is no way for the system to know just return true.
enum bool LEDsPut(uc_8 value){
    BusAddress=LedsAddr;
    BusData=value;
    BusWrite();
    return true;
}
// ************************************************************************
// In the initial version of CLIC3 the Keypad operations are STUBS CAM 2014-06-02
// Lookup table for operation of the Keypad
const uc_8 LookupKeys[MaxKeys] = { 
    0x82,0x11,0x12,0x14,
    0x21,0x22,0x24,0x41,
    0x42,0x44,0x81,0x84,
    0x88,0x48,0x28,0x18
};

const uc_8 LookupSeg[MaxSegs] = {
    0x040,0x079,0x024,0x030,0x019,0x012,0x002,0x078,
    0x000,0x018,0x008,0x003,0x046,0x021,0x006,0x00E,
    0x027,0x009,0x07B,0x060,0x047,0x023,0x00C,0x02F,
    0x041,0x011,0x03F,0x07F
};

volatile uc_8 KeyPadRaw = 0;
volatile uc_8 LastKey = 0;
volatile enum bool NewKey = false;
unsigned char  BigCount;
unsigned int   DelCount; 

// Initialise the keypad input interface
void keypadInit(void){
    P2IES &= ~0x01;     // Positive edge interrupt
    P2IE  |= 0x01;      // Enable interrupt
    P2IFG &= ~0x01;     // Clear any pending flag
    __enable_interrupt(); // Enable global interrupts
}
// Get a new value from the keypad if there is one otherwise return false
// Returns true if a new value has been found
enum bool keypadGet(uc_8 *value) {
    if (NewKey) {
        *value = LastKey;   // Return key index
        NewKey = false;     // Clear flag
        return true;        // New key available
    } else {
        return false;       // No new key
    }
}
// ************************************************************************



// Initialise the Seven Segment Display output interface
void sevenSegInit(void){
}

/* Send a new value to the selected display and return true otherwise return false if this could not be achieved. 
DispID chooses the display (o or 1). 
value is the selector for what is to be displayed. 
If the result is out of range then blank the display.
*/
enum bool sevenSegPut(uc_8 DispID,uc_8 value) {
    if(DispID) {
        BusAddress = Seg7AddrL; }
    else {
        BusAddress = Seg7AddrH; }
    if(MaxSegs > value) {
        BusData = LookupSeg[value];
        BusWrite();
       return true;
    } else {
        BusData = LookupSeg[Blank7Seg];
        BusWrite();
        return false;
  }
}
// ************************************************************************


void timerInit(void) {
    TA0CTL = TASSEL_2 + MC_2 + TACLR;  // SMCLK, continuous mode
}



// ************************************************************************
void lcdInit(void) {
    unsigned int Wait;

    // --- I2C setup ---
    UCB1CTL1 |= UCSWRST;                                  // Reset I2C
    UCB1CTL0 |= (UCMST + UCMODE_3 + UCSYNC);              // Master mode, I2C, synchronous
    UCB1CTL1 |= (UCTR + UCSSEL_1);                        // Transmit, ACLK
    UCB1BR0 = 63;                                         // 400kHz baud rate

    UCB1I2CSA = 0x3E;                                     // Slave address (LCD)

    P4SEL |= 0x02;                                        // SDA (P4.1)
    P4SEL |= 0x04;                                        // SCL (P4.2)

    UCB1CTL1 &= ~UCSWRST;                                 // Release I2C reset 

    // --- LCD setup sequence ---
    UCB1CTL1 |= UCTXSTT;                                  // Start

    UCB1TXBUF = 0x00;                                     // Control LCD
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x39;                                     // Function set
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x14;                                     // Internal oscillator frequency
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x74;                                     // Contrast set
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x54;                                     // Power/ICON control/Contrast set
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x6F;                                     // Follower control
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x0E;                                     // Display ON/OFF, Cursor ON, No blink 
    while (!(UCTXIFG & UCB1IFG));

    UCB1TXBUF = 0x01;                                     // Clear display
    while (!(UCTXIFG & UCB1IFG));

    UCB1CTL1 |= UCTXSTP;                                  // Stop
    UCB1IFG &= ~UCTXIFG;                                  // Clear transmit interrupt flag

    // Small delay for LCD to settle
    for (Wait = 0; Wait < 10000; Wait++) { }
}


void lcdPut(const char *str, unsigned char line) {
    unsigned char i;

    // --- Move cursor to start of desired line ---
    while (UCB1CTL1 & UCTXSTP);        // Wait for any previous STOP
    UCB1CTL1 |= UCTR | UCTXSTT;        // TX mode, Start condition

    UCB1TXBUF = 0x00;                  // Control byte = instruction
    while (!(UCB1IFG & UCTXIFG));

    if (line == 1) {
        UCB1TXBUF = 0x80;              // Line 1 DDRAM address
    } else {
        UCB1TXBUF = 0xC0;              // Line 2 DDRAM address
    }
    while (!(UCB1IFG & UCTXIFG));

    UCB1CTL1 |= UCTXSTP;               // Stop condition
    UCB1IFG &= ~UCTXIFG;
    while (UCB1CTL1 & UCTXSTP);        // ?? Wait for STOP to complete

    // --- Write data bytes (characters) ---
    UCB1CTL1 |= UCTR | UCTXSTT;        // TX mode, Start condition

    UCB1TXBUF = 0x40;                  // Control byte = data
    while (!(UCB1IFG & UCTXIFG));

    for (i = 0; str[i] != '\0' && i < 16; i++) {
        UCB1TXBUF = str[i];            // Send character
        while (!(UCB1IFG & UCTXIFG));
    }

    UCB1CTL1 |= UCTXSTP;               // Stop condition
    UCB1IFG &= ~UCTXIFG;
    while (UCB1CTL1 & UCTXSTP);        // ?? Wait for STOP complete
}

// ************************************************************************
// ISR for keypad (P2.0 interrupt)
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    unsigned char Count;

    // Read keypad
    BusAddress = KeyPadAddr;
    BusRead();
    KeyPadRaw = BusData;

    // Find the logical key index
    for (Count = 0; Count < 16; Count++) {
        if (KeyPadRaw == LookupKeys[Count]) {
            LastKey = Count;    // Store key index
            NewKey = true;      // Set flag
            
            
            PUCTL|=PUOUT0;                         // Switch ON buzzer            
      

      
            for(BigCount=0;BigCount<5;BigCount++){      
            
            
            for(DelCount=0;DelCount<50000;DelCount++){}  //50000

            }      
            
            PUCTL&=~PUOUT0;                      // Switch OFF buzzer  
            
            
            break;
        }
    }

    // Clear interrupt flag
    P2IFG &= ~0x01;
}