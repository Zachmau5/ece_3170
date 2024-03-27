#include <C8051F020.h>
#include "lcd.h"

// Global Variables Initialization
int adcToggleCounter = 0; // Counter for ADC checks
int lastTempReading = 0; // Last temperature reading from ADC
unsigned long totalTemp = 0; // Total temperature for averaging
unsigned long totalPot = 0; // Total potentiometer reading for averaging
int averageTemp = 0; // Average temperature
int averagePot = 0; // Average potentiometer reading
long tempValForDisplay = 0; // Temperature value for display
long potValForDisplay = 0; // Potentiometer value for display
unsigned char displayCharTempTens = 0;
unsigned char displayCharTempUnits = 0;
unsigned char displayCharPotTens = 0;
unsigned char displayCharPotUnits = 0;

// Function to display a character on the LCD
void disp_char(unsigned char row, unsigned char col, char ch) {
    int i; // Index of screen
    int j;
    int k;
    i = 128*row + col;
    j = (ch-0x20)*5;
    for(k=0; k<5; k++) {
        screen[i+k] = font5x8[j+k];
    }
}

// Function to update LED states
void LED_update(void) {
    if (tempValForDisplay < potValForDisplay) {
        P3 = 0xFF; // Turn on all LEDs on Port 3
        P2 |= 0xC0; // Turn on P2.6 and P2.7
    } else {
        P3 = 0x00; // Turn off all LEDs on Port 3
        P2 &= ~0xC0; // Turn off P2.6 and P2.7
    }
}

// ADC Interrupt Service Routine
void ADC_ISR(void) interrupt 15 {
    adcToggleCounter++;
    AD0INT = 0; // Clear ADC interrupt flag
    if (adcToggleCounter % 2 == 0) {
        lastTempReading = (ADC0H << 8) | ADC0L;
        totalTemp += (long)lastTempReading;
        AMX0SL = 0x00; // Set ADC to read temperature sensor
    } else {
        lastPotReading = (ADC0H << 8) | ADC0L;
        totalPot += (long)lastPotReading;
        AMX0SL = 0x08; // Set ADC to read potentiometer
    }
}

void config()
{
    WDTCN = 0xde; // disable watchdog
    WDTCN = 0xad;
    XBR0 = 0x04; // enable uart 0
    OSCICN |= 0x03; // 11 turns on internal oscillator
    TMOD = 0x20; // timer 1 in mode 2
    TH1 = -3; // 9600 baud
    CKCON |= 0x10; // T1 uses system clock
    TR1 = 1; // start timer 1
    SCON = 0x50; // mode 1, 8-bit UART, enable receiver
    IE |= 0x10; // enable UART0 interrupts
    EIE1 |= 0x08; // enable ADC interrupts
    ADC0CN = 0x40; // enable ADC
    // setup the crossbar and ports
    P0MDOUT |= 0x10; // set P0.4 as push-pull output
    XBR2 |= 0x40; // enable crossbar
    // Initialize ADC
    ADC0CF = (0 << 2) | (1 << 1) | 0; // SAR clock = 2.5MHz, right justified
    ADC0CF |= 0x01; // Gain of 1
    AMX0SL = 0x02; // Select Potentiometer input (P0.2)
    ADC0CN = 0x80; // Enable ADC conversion
    REF0CN = 0x03; // Enable internal VREF
    // Set up Timer 3 for auto-reload, to trigger ADC at interval
    TMR3CN = 0x00; // Stop Timer3; Clear TF3;
    TMR3CF = 0x08; // use SYSCLK as source
    RCAP3L = 0xb8; // Init reload values
    RCAP3H = 0xff; // for 10ms period
    TMR3L = 0xb8; // set to reload immediately
    TMR3H = 0xff;
    EIE1 |= 0x80; // Enable Timer3 interrupts
    TMR3CN |= 0x04; // start Timer3
    // SPI Init
    SPI0CFG = 0x40; // Master mode
    SPI0CN = 0x01; // SPI enabled, in 3-wire mode
    SPI0CKR = 0x09; // SPI clock rate divider
    P1MDOUT |= 0x40; // SCK, MOSI, P1.6 push-pull
    XBR0 |= 0x02; // Enable SPI on P0.0, P0.1, P0.2
	init_lcd();
}



void main(void) {
    config(); // Perform initial configuration
    while(1) {
        if (adcToggleCounter == 512) {
            // Calculate averages
            averageTemperature = (totalTemperature >> 8);
            averagePotentiometer = (totalPotentiometer >> 8);
            // Reset for next calculation cycle
            totalTemperature = 0;
            totalPotentiometer = 0;
            adcToggleCounter = 0;
            // Determine values for display
            temperatureValueForDisplay = ((averageTemperature - 2475) * 12084L) >> 16;
            potentiometerValueForDisplay = 55L + ((averagePotentiometer * 30L) >> 12);
            LED_update(); // Update LEDs based on new values

            // Calculate display characters for temperature
            tempDisplayTens = (temperatureValueForDisplay / 10) % 10;
            tempDisplayUnits = temperatureValueForDisplay % 10;
            
            // Calculate display characters for potentiometer
            potDisplayTens = (potentiometerValueForDisplay / 10) % 10;
            potDisplayUnits = potentiometerValueForDisplay % 10;

            // Clear screen for fresh display
            blank_screen();
            
            // Display "ACTUAL TEMP" using character prints
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
            
            // Display calculated temperature
            disp_char(3, 24, '0' + tempDisplayTens);
            disp_char(3, 30, '0' + tempDisplayUnits);
            disp_char(3, 36, 0x7F); // Assume this is for a degree symbol or separator

            // Display "THERMO" using character prints for potentiometer reading label
            disp_char(0, 64, 'T');
            disp_char(0, 70, 'H');
            disp_char(0, 76, 'E');
            disp_char(0, 82, 'R');
            disp_char(0, 88, 'M');
            disp_char(0, 94, 'O');
            
            // Display calculated potentiometer reading
            disp_char(3, 88, '0' + potDisplayTens);
            disp_char(3, 94, '0' + potDisplayUnits);
            disp_char(3, 100, 0x7F); // Assume this is for a degree symbol or separator

            // Refresh screen to show new values
            refresh_screen();
        }
    }
}
