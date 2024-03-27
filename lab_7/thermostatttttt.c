;Project: Lab 7, Thermostat
;Team: Andrew Coffel and Zachary Hallett
;Description:
;       The student should be able to write and test a program that reads
;       an A/D converter, scales the data as appropriate and displays it on
;       the LCD, then uses that data to make control decisions.
;
;VER		DATE				Description
;
; 0.0 -	3/25/2024 	-	initial project based off of Lab 6
; 1.0 - 2/27/2024 	-	Project Submit

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


// Display Character Function
void disp_char(unsigned char row, unsigned char column, char character) {
    int i, j;
    unsigned char k;
    i = 128 * row + column;
    j = (character - 0x20) * 5;
    for(k = 0; k < 5; ++k) {
        screen[i + k] = font5x8[j+k];
    }
}

// ADC Interrupt Service Routine
void adc_int(void) interrupt 15 {
    AD0INT = 0; // Clear ADC interrupt flag
    long dataOut = (ADC0H << 8) | ADC0L; // Read ADC data
    averaged = 0;

    if (adcIndex < 256) { // Temperature sensor reading cycle
        // Perform the temperature conversion and accumulation
        long tempVal = ((dataOut - 2475) * 12084L) >> 16;
        tempSum += tempVal;
        
        if (adcIndex == 255) {
            // Switch to potentiometer reading for the next ADC conversion
            ADC0CF = 0x40; // Assuming this configures the ADC for potentiometer readings
            AMX0SL = 0x08; // Change ADC multiplexer to potentiometer channel
        }
    } else if (adcIndex < 512) { // Potentiometer reading cycle
        // Direct accumulation of potentiometer readings, adjust if conversion is needed
        potSum += dataOut; 
        
        if (adcIndex == 511) {
            // Switch back to temperature sensor readings
            AMX0SL = 0x00; // Change ADC multiplexer to temperature sensor channel
            adcIndex = -1; // Reset index for the next cycle
            averaged = 1; // Indicate that averaging can now occur
            // Calculate the averages
            tempAvg = tempSum >> 8;
            potAvg = potSum >> 8;
            // Reset sums for the next averaging cycle
            tempSum = 0;
            potSum = 0;
        }
    }
    adcIndex++; // Increment the index for the next ADC conversion
}

// Timer 2 Interrupt Service Routine
void t2_int(void) interrupt 5 {
    TF2 = 0; // Clear Timer 2 interrupt flag

    // Logic to manage cycles of ADC readings for temperature and potentiometer
    if (adcIndex < 256) {
        toggle = 0; // Preparing for temperature sensor reading
    } else if (adcIndex < 512) {
        //do nothing?
        toggle = 1; // Preparing for potentiometer reading
    } else {
        adcIndex = 0; // Reset for the next cycle of readings
        ADC0CF=0x41;
        AMX0SL=0x08;
        //toggle = 0; // Start again with temperature sensor
    }
}


// Device Initialization
void init_device() {
    WDTCN = 0xDE;  // Disable watchdog timer
    WDTCN = 0xAD;
    XBR2 = 0x40;   // Enable crossbar and weak pull-ups
    XBR0 = 0x04;   // Enable UART 0
    OSCXCN = 0x67; // Start external oscillator
    TMOD = 0x20;   // Timer 1 mode 2, auto-reload
    TH1 = -167;    // Timer reload value for 1ms
    TR1 = 1;       // Start Timer 1
    while (TF1==0){};  // Wait until timer overflow
    TF1 = 0;       // Clear overflow flag
    while (!(OSCXCN & 0x80)); // Wait for oscillator to stabilize
    //while ( TF1 == 0 ) { }          // wait 1ms MAYBE PUT THIS IN
    //while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
    OSCICN = 0x08; // Switch to the stabilized external oscillator
    SCON0 = 0x50;  // Configure serial port (UART)
    TH1 = -6;      // Baud rate for UART
    //init_lcd();    // Initialize LCD

}


void init_adc(void) {
    ADC0CN=0x8C;
    REF0CN = 0x07; // Enable on-chip voltage reference and temperature sensor. 
    ADC0CF=0x41; // setting values to read from temp sensor
    AMX0SL=0x08;
    AMX0CF = 0x00; // Set AMUX to single-ended input mode for all inputs.
}


void init_timer(void) {
    IE = 0x80; // Enable Timer 2 interrupts (ET2 = 1).
    EIE2 = 0x02;  // Alternative way to enable Timer 2 interrupts (if applicable)

    // Setup auto-reload value for Timer 2 to overflow at desired intervals.
    // Assuming a 24.5 MHz system clock, for a 1ms overflow interval as an example:
    // The formula is: ReloadValue = 65536 - (Sysclk / 12 / DesiredFrequency)

    T2CON=0x00;
    
    //TMR2H = RCAP2H; // Load the high byte of the auto-reload value into Timer 2 high byte
    //TMR2L = RCAP2L; // Load the low byte of the auto-reload value into Timer 2 low byte
    RCAP2H = -843 >> 8; // High byte of auto-reload value. Adjust the calculation as necessary.
    RCAP2L = -843; // Low byte of auto-reload value.

    TR2=1;
}

// Main loop with logic to display values and control LEDs
void main(void) {
    init_device();
    init_adc();
    init_timer();
    init_lcd();    // Initialize LCD

    while(1) {
        // Once averaging is complete, display values and control LEDs
        if(averaged==1) {
            blank_screen(); // Clear the screen for fresh display
		tens = (pot_avg / 10) % 10;
		ones = (pot_avg  % 10);

		temp_tens = (temp_avg / 10) % 10;
		temp_ones = (temp_avg % 10);
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
            disp_char(4, 0, '0' + tens);
            disp_char(4, 6, '0' + ones);

            // Update LED state based on temperature and potentiometer comparison
            if(tempAvg < potAvg) {
                P3 = 0x00; // Condition met, LEDs OFF
                P2 &= ~0xC0; // Additional control if needed
            } else {
                P3 = 0xFF; // Condition not met, LEDs ON
                P2 |= 0xC0; // Additional control if needed
            }

            // Reset averaging flag after processing
            averaged = 0;  //maybe comment this out

            // Refresh the screen to display new values
            refresh_screen();
        }
    }
}

// Timer Initialization
// void init_timer() {
//     RCAP2H=-843 >>8; overflows 512 times / 400 ms
//     RCAP2L=-843;
//     TF2=0;
//     TR2=1
//     //TMR2H = 0xFF;  // High byte of auto-reload value
//     //TMR2L = 0x00;  // Low byte of auto-reload value
//     //TMR2CN = 0x04; // Enable Timer 2
//     IE = 0x80;    // Enable Timer 2

//      // Enable Timer 2 interrupts (using IE.5)
//     EIE2 = 0x02;  // Alternative way to enable Timer 2 interrupts (if applicable)
//     init_lcd();    // Initialize LCD

// }

// ADC Initialization
// void init_adc() {
//     ADC0CN=0x8C;
//     AMX0CF = 0x00;  // Set AMUX to single-ended input
//     AMX0SL=0x00;
//     ADC0CF = 0x40; // Set ADC conversion rate
//    // ADC0CN = 0x80;  // Enable ADC and continuous conversion
//     //REF0CN = 0x07;  // Enable on-chip voltage reference and temperature sensor
// }
