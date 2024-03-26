#include <C8051F020.h>
#include <lcd.h>
// Include standard I/O for sprintf
#include <stdio.h>
// Define ADC channels
#define TEMP_SENSOR 0x0F  // ADC channel for the internal temperature sensor
#define POTENTIOMETER_CHANNEL_0 0x00  // ADC channel for the first potentiometer (AIN0.0)
//#define POTENTIOMETER_CHANNEL_1 0x01  // ADC channel for the second potentiometer (AIN0.1), for future use
//#define _nop_() _asm nop _endasm

// Define strings in the external memory space
//ubyte xdata __at(0xF000) char temperatureStr[32]
//ubyte xdata __at(0xF000) char setpointStr[32]
xdata char temperatureStr[32]; // Correct usage
xdata char setpointStr[32];    // Correct usage
void init_device() {
    WDTCN = 0xDE;  // Disable watchdog timer
    WDTCN = 0xAD;
    XBR2 = 0x40;   // Enable port output
    XBR0 = 0x04;   // Enable UART 0
    OSCXCN = 0x67; // Start external oscillator
    TMOD = 0x20;   // Timer 1 mode 2, auto-reload
    TH1 = -167;    // Timer reload value for 1ms (assuming 2MHz)
    TR1 = 1;       // Start Timer 1
    while (!TF1);  // Wait until timer overflow
    TF1 = 0;       // Clear overflow flag
    while (!(OSCXCN & 0x80));  // Wait for oscillator to stabilize
    OSCICN = 0x08; // Switch to the stabilized external oscillator
    SCON0 = 0x50;  // Configure serial port (UART)
    TH1 = -6;      // Baud rate for UART

    init_lcd();    // Initialize LCD
}

void init_adc() {
    AMX0CF = 0x00;  // Set AMUX to single-ended input
    ADC0CF = 0x40;  // Set ADC conversion rate (update based on your clock)
    ADC0CN = 0x80;  // Enable ADC and start conversion
    REF0CN = 0x03;  // Enable on-chip voltage reference and temperature sensor
}

unsigned int read_adc(unsigned char channel) {
    AMX0SL = channel;             // Select ADC input channel
    ADC0CN &= ~0x20;              // Clear the flag
    ADC0CN |= 0x10;               // Start conversion
    while (!(ADC0CN & 0x20));     // Wait for conversion to complete
    return (ADC0L | (ADC0H << 8)); // Return ADC value
}
void disp_char(unsigned char row, unsigned char col, char ch) {
    int i, j, k;
    i = 128 * row + col;  // Calculate the starting index in the 'screen' array.
    j = (ch - 0x20) * 5;  // Calculate the starting index in the 'font' data.

    for (k = 0; k < 5; k++) {  // Typically, character fonts are 5 columns wide.
        screen[i + k] |= font[j + k];  // Combine the existing screen data with the font bitmap.
    }
}

float convert_temp(unsigned int adc_value) {
    // Convert ADC value to temperature in Fahrenheit.
    // Update this formula based on your sensor characteristics and calibration.
    float temperature = (adc_value * 0.805) - 50;  // Example conversion
    return temperature * 1.8 + 32;  // Convert Celsius to Fahrenheit
}

float read_temperature() {
    unsigned int adc_value = read_adc(TEMP_SENSOR);
    return convert_temp(adc_value);
}
//TODO check and see if 10 bit, I think it might be 8 bit but this is what forums say
float read_set_point_from_potentiometer(unsigned char channel) {
    unsigned int adc_value = read_adc(channel);
    // Scale the potentiometer value to the range 55-85
    return 55.0 + ((float)adc_value / 1023.0) * 30.0; // Assuming a 10-bit ADC
}

void update_display(float temperature, float set_point) {
    char temp_str[16], setpoint_str[16];
    sprintf(temp_str, "Temp: %.2f F", temperature);
    sprintf(setpoint_str, "SetPt: %.2f F", set_point);
    blank_screen();
    lcd_write_string(temp_str, 0, 0);
    lcd_write_string(setpoint_str, 1, 0);
}

void control_leds(float temperature, float set_point) {
    // LEDs are connected to Port 3.
    if (temperature <= set_point) {
        P3 |= 0x01;  // Turn LED on (set the bit to 1)
    } else {
        P3 &= ~0x01; // Turn LED off (clear the bit)
    }
}

void main() {
    //volatile unsigned int delay;  // Declare 'delay' outside of the for loop
    //init_device();  // Initialize the device
    init_lcd();     // Initialize LCD for reading temperature and potentiometers
for ( ; ; )
   {
      // wait for data to arrive
      if ( !RI0 ) continue;
      // clear the receiver flag & echo the data
      RI0 = 0;
      SBUF0 = SBUF0;
	  

   }
    //while (1) {
      //  blank_screen();    // Clear the screen
        
      //  lcd_write_string("Hello,", 0, 0);    // Write "Hello," at the first line
      //  lcd_write_string("World!", 1, 0);    // Write "World!" at the second line
       // blank_screen();    // Clear the screen again for the next message
        
       // lcd_write_string("Testing", 0, 0);   // Write "Testing" at the first line
       // lcd_write_string("123", 1, 0);       // Write "123" at the second line
}

