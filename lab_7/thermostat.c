#include <C8051F020.h>
#include "lcd.h"

// Aligned Global Variables Initialization
bit toggle = 0; // Used as a toggle for ADC reading, to match the logic
long tempSum = 0; // Total temperature for averaging
long potSum = 0; // Total potentiometer reading for averaging
long tempAvg = 0; // Average temperature after readings
long potAvg = 0; // Average potentiometer reading
int adcIndex = 0; // Counter for ADC checks, matching the role of 'index'
bit averaged = 0; // Flag to indicate completion of averaging
char tempTens = 0; // Tens digit of the temperature for display
char tempOnes = 0; // Ones digit of the temperature for display
char potTens = 0; // Tens digit of the potentiometer value for display
char potOnes = 0; // Ones digit of the potentiometer value for display

// Function to display a character on the LCD
void disp_char(unsigned char row, unsigned char column, char character) {
    int i, j;
    unsigned char k;
    i = 128 * row + column;
    j = (character - 0x20) * 5;
    for(k = 0; k < 5; ++k) {
        screen[i + k] = font5x8[j + k];
    }
}

// ADC Interrupt Service Routine
void adc_int(void) interrupt 15 {
    AD0INT = 0; // Clear ADC interrupt flag
    long dataOut = (ADC0H << 8) | ADC0L; // Combined ADC data
    
    // Logic to switch between temperature and potentiometer readings
    if (adcIndex < 256) {
        tempSum += ((dataOut - 2475) * 12084L) >> 16;
        if (adcIndex == 255) {
            AMX0SL = 0x08; // Switch to potentiometer for next reading
        }
    } else {
        potSum += dataOut; // Adjust this calculation as needed
        if (adcIndex == 511) {
            AMX0SL = 0x00; // Switch back to temperature sensor
            averaged = 1; // Set flag indicating averaging can proceed
            tempAvg = tempSum >> 8; // Calculate average
            potAvg = potSum >> 8;
            tempSum = 0; // Reset sums for next cycle
            potSum = 0;
            adcIndex = -1; // Reset index for next cycle
        }
    }
    adcIndex++;
}

// Device Initialization
void init_device() {
    WDTCN = 0xDE;  // Disable watchdog timer
    WDTCN = 0xAD;
    XBR2 = 0x40;   // Enable crossbar and weak pull-ups
    XBR0 = 0x04;   // Enable UART 0
    OSCXCN = 0x67; // Start external oscillator
    TMOD = 0x20;   // Timer 1 mode 2, auto-reload
    TH1 = -167;    // Timer reload value for 1ms (assuming 24.5MHz oscillator)
    TR1 = 1;       // Start Timer 1
    while (!TF1);  // Wait until timer overflow
    TF1 = 0;       // Clear overflow flag
    while (!(OSCXCN & 0x80)); // Wait for oscillator to stabilize
    OSCICN = 0x08; // Switch to the stabilized external oscillator
    SCON0 = 0x50;  // Configure serial port (UART)
    TH1 = -6;      // Baud rate for UART
    init_lcd();    // Initialize LCD
}

// ADC Initialization
void init_adc() {
    AMX0CF = 0x00;  // Set AMUX to single-ended input
    ADC0CF = (1 << 2) | 0x01; // Set ADC conversion rate
    ADC0CN = 0x80;  // Enable ADC and continuous conversion
    REF0CN = 0x07;  // Enable on-chip voltage reference and temperature sensor
}

// Timer Initialization
void init_timer() {
    TMR2H = 0xFF;  // High byte of auto-reload value
    TMR2L = 0x00;  // Low byte of auto-reload value
    TMR2CN = 0x04; // Enable Timer 2
    IE |= 0x20;    // Enable Timer 2

     // Enable Timer 2 interrupts (using IE.5)
    EIE2 |= 0x02;  // Alternative way to enable Timer 2 interrupts (if applicable)
}

// Main loop with logic to display values and control LEDs
void main(void) {
    init_device();
    init_adc();
    init_timer();
    while(1) {
        // Once averaging is complete, display values and control LEDs
        if(averaged) {
            blank_screen(); // Clear the screen for fresh display

            // Displaying temperature value as "ACTUAL TEMP"
            disp_char(0, 0, 'A');
            disp_char(0, 6, 'C');
            disp_char(0, 12, 'T');
            disp_char(0, 18, 'U');
            disp_char(0, 24, 'A');
            disp_char(0, 30, 'L');
            disp_char(1, 0, 'T');
            disp_char(1, 6, 'E');
            disp_char(1, 12, 'M');
            disp_char(1, 18, 'P');
            disp_char(2, 0, '0' + tempTens);
            disp_char(2, 6, '0' + tempOnes);

            // Changing to display "SET_PT" for potentiometer value
            disp_char(3, 0, 'S');
            disp_char(3, 6, 'E');
            disp_char(3, 12, 'T');
            disp_char(3, 18, '_');
            disp_char(3, 24, 'P');
            disp_char(3, 30, 'T');
            disp_char(4, 0, '0' + potTens);
            disp_char(4, 6, '0' + potOnes);

            // Update LED state based on temperature and potentiometer comparison
            if(tempAvg < potAvg) {
                P3 = 0x00; // Condition met, LEDs OFF
                P2 &= ~0xC0; // Additional control if needed
            } else {
                P3 = 0xFF; // Condition not met, LEDs ON
                P2 |= 0xC0; // Additional control if needed
            }

            // Reset averaging flag after processing
            averaged = 0;

            // Refresh the screen to display new values
            refresh_screen();
        }
    }
}

