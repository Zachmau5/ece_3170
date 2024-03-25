#ifndef THERMOSTAT_H  // Include guard to prevent multiple inclusions
#define THERMOSTAT_H

#include <C8051F020.h>  // Include the MCU-specific header

// LCD Interface Functions
void init_lcd(void);  // Initialize LCD - sets up hardware, blanks shadow memory, displays on screen
void refresh_screen(void);  // Copy shadow memory to LCD screen
void blank_screen(void);  // Clear the shadow memory

// LCD Shadow Memory
extern xdata char screen[1024];  // 1024 bytes for 64x128 LCD module, maps bytes to screen pixels

// Font data
extern code char font5x8[];  // Font data for 5x7 characters, useful for later labs

// ADC and Temperature Reading Functions
void init_adc(void);  // Initialize ADC settings for reading analog inputs
unsigned int read_adc(unsigned char channel);  // Read ADC value from specified channel
float convert_temp(unsigned int adc_value);  // Convert ADC value to temperature in Fahrenheit
float read_temperature(void);  // Read and convert temperature from internal sensor
float read_set_point_from_potentiometer(unsigned char channel);  // Read set point value from potentiometer

// Display and Control Functions
void update_display(float temperature, float set_point);  // Update LCD with temperature and set point
void control_leds(float temperature, float set_point);  // Control LEDs based on temperature comparison

// Define ADC channels for temperature sensor and potentiometers
#define TEMP_SENSOR 0x0F  // ADC channel for the internal temperature sensor
#define POTENTIOMETER_CHANNEL_0 0x00  // ADC channel for the first potentiometer (AIN0.0)
#define POTENTIOMETER_CHANNEL_1 0x01  // For future use (AIN0.1)

#endif // THERMOSTAT_H
