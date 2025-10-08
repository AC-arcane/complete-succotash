#include "clic3.h"
const int Seg1 = 0;
const int Seg2 = 1;
uc_8 key;  // Temporary key holder
unsigned char elapsed_seconds;
unsigned int keypad_number;

// Global variables for S3 timing
volatile unsigned int s3_start_time;
volatile unsigned int s3_elapsed_time;
volatile unsigned char s3_seconds;
volatile enum bool s3_timing_active = false;
volatile enum bool s3_pressed = false;
volatile enum bool s3_released = false;
volatile unsigned int blink_counter = 0;
volatile enum bool alarm_active = false;
volatile enum bool led_d0_state = false;

volatile unsigned int tick_count = 0;    // counts 0.1s intervals
volatile unsigned int seconds = 0;       // whole seconds
volatile unsigned char update_flag = 0;

// ===============================
// Delay Function (~1ms @ 1MHz)
// ===============================
void Delay_ms(unsigned int ms) {
    while (ms--) {
        unsigned int i;
        for (i = 0; i < 1000; i++) __no_operation();
    }
}


// ===============================
// Debounced read of S3 (switch)
// ===============================
#define DEBOUNCE_DELAY_MS 5

unsigned char readS3Debounced(void) {
    unsigned char firstRead, secondRead;

    BusAddress = SwitchesAddr;
    BusRead();
    firstRead = BusData & 0x80; // S3 = BIT7

    Delay_ms(DEBOUNCE_DELAY_MS);

    BusAddress = SwitchesAddr;
    BusRead();
    secondRead = BusData & 0x80;

    if (firstRead == secondRead)
        return firstRead;
    else
        return 0x00; // treat unstable as OFF
}




// ************************************************************************
// Function to get a 2-digit number from the keypad
// ************************************************************************
unsigned int getTwoDigitKeypad(void) {
    char lcd_line[17];       // Internal LCD line buffer
    uc_8 digits[2];          // Internal storage for digits
    uc_8 digit_count = 0;    // Counter for digits
    unsigned int number = 0; // Final 2-digit number
    
    lcdPut("EEnt:", 1);

    
    while (digit_count < 2) {
        if (keypadGet(&key)) {
            // Ignore keys 10-15 (A-F)
            if (key > 9) continue;
            
            // Store the digit
            digits[digit_count] = key;
            digit_count++;
            
            // Display digits as they are entered
            lcd_line[0] = '0' + digits[0];       // first digit
            if (digit_count == 2) {
                lcd_line[1] = '0' + digits[1];   // second digit
                lcd_line[2] = '\0';
            } else {
                lcd_line[1] = '_';               // cursor for next digit
                lcd_line[2] = '\0';
            }
            lcdPut(lcd_line, 2);
        }
    }
    
    // Compute final number
    number = digits[0] * 10 + digits[1];
    
    // Show confirmation
    char confirm_msg[17];
    confirm_msg[0] = 'T'; confirm_msg[1] = 'h'; confirm_msg[2] = 'r'; confirm_msg[3] = 'e';
    confirm_msg[4] = 's'; confirm_msg[5] = 'h'; confirm_msg[6] = ':'; confirm_msg[7] = ' ';
    confirm_msg[8] = '0' + (number / 10);
    confirm_msg[9] = '0' + (number % 10);
    confirm_msg[10] = 's'; confirm_msg[11] = '\0';
    lcdPut(confirm_msg, 1);
    lcdPut("Press S3 to time", 2);
    
    return number;
}



// ************************************************************************
// Update 7-segment displays with elapsed seconds
// ************************************************************************
void updateSevenSegDisplay(unsigned char seconds) {
    // Display units (right display)
    BusAddress = Seg7AddrL;
    BusData = LookupSeg[seconds % 10];
    BusWrite();
    
    // Display tens (left display) 
    BusAddress = Seg7AddrH;
    BusData = LookupSeg[(seconds / 10) % 10];
    BusWrite();
}

// ************************************************************************
// Update LED D7 to reflect S3 state
// ************************************************************************
void updateLEDD7(enum bool s3_state) {
    BusAddress = LedsAddr;
    BusRead();  // Read current LED state
    
    if (s3_state) {
        BusData |= 0x80;   // Turn on D7 (bit 7)
    } else {
        BusData &= ~0x80;  // Turn off D7 (bit 7)  
    }
    BusWrite();
}

// ************************************************************************
// Update LED D0 for threshold alarm
// ************************************************************************
void updateLEDD0(enum bool state) {
    BusAddress = LedsAddr;
    BusRead();  // Read current LED state
    
    if (state) {
        BusData |= 0x01;   // Turn on D0 (bit 0)
    } else {
        BusData &= ~0x01;  // Turn off D0 (bit 0)
    }
    BusWrite();
}



// ************************************************************************
// Main function
// ************************************************************************
void main(void) {
    Initialise();      // System initialization
    switchesInit();    
    LEDsInit();        
    sevenSegInit();    
    lcdInit();         
    keypadInit();      
    timerInit();
    
    
    // Get threshold from user
    for (;;) {
        keypad_number = getTwoDigitKeypad();
        break; 
    }
    
    // Display threshold on 7-segment initially
    updateSevenSegDisplay(keypad_number);
    
    
    WDTCTL = WDTPW | WDTHOLD;
    
    // =================================
    // Clock Setup (1 MHz DCO, REFO=ACLK)
    // =================================
    UCSCTL3 = SELREF__REFOCLK;          
    UCSCTL4 = SELA__REFOCLK | SELS__DCOCLK | SELM__DCOCLK;  
    UCSCTL0 = 0x0000;                   
    UCSCTL1 = DCORSEL_5;                
    UCSCTL2 = FLLD_1 + 30;              
    UCSCTL8 |= ACLKREQEN;               // Enable ACLK request

    __delay_cycles(750000);             // Allow DCO to settle

    // =================================
    // Timer_A0 Setup (0.1s interrupt)
    // =================================
    TA0CTL = TASSEL_1 + MC_1 + TACLR;   // ACLK, Up mode
    TA0CCR0 = 3277 - 1;                  // 0.1s @ 32768Hz
    TA0CCTL0 = CCIE;                     // Enable CCR0 interrupt
    __enable_interrupt();   // Enable global interrupts


   
    
    // Main S3 timing loop
    while (1) {
     
      if(seconds >= keypad_number){
        int k = 1;
        
      }

    }
}


// =================================
// Timer ISR (fires every 0.1s)
// =================================
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void) {
    unsigned char s3State = readS3Debounced();
    updateLEDD7(s3State);
    // S3 is pressed (active low)
    if (s3State) {
        tick_count++;

        if (tick_count >= 10) {          // 10 x 0.1s = 1 second
            tick_count = 0;
            seconds++;
            update_flag = 1;             // signal main loop / ISR to update display
            updateSevenSegDisplay(seconds);
        }
    } else {
        // Switch released ? reset counting
        tick_count = 0;
        seconds= 0;
    }
}