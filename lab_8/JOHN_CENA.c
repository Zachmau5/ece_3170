#include <C8051F020.h>
#include <stdbool.h>
//#include "bell.h"

code unsigned char sine[] = { 176, 217, 244, 254, 244, 217, 176, 128, 80, 39, 12, 2, 12, 39, 80, 128 };
unsigned int adjustedVolume;
unsigned char phase = sizeof(sine)-1;	// current point in sine to output
unsigned char mybyte=0;
unsigned int duration = 0;		// number of cycles left to output
unsigned int delay=0;
sbit BUTTON_1 = P2^7;
sbit BUTTON_2 = P2^6;
bool isReady = true;
static unsigned int volumeStep = 0;
static unsigned int decayCounter = 0;
static unsigned int volume;


void timer2(void) interrupt 5 {
    TF2 = 0; // Clear interrupt flag

    // Apply volume to the sine wave output, ensure it doesn't underflow
    adjustedVolume = (sine[phase] * volume) >> 8;
    DAC0H = adjustedVolume > 200 ? 200 : adjustedVolume;

    if (phase < sizeof(sine)-1) {
        phase++;
    } else if (duration > 0) {
        phase = 0;
        duration--;

        // Increment decay counter every time the timer interrupts
        decayCounter++;
        if (decayCounter >= 200) {
            if (volumeStep == 0) { // Only calculate step value at the beginning
                volumeStep = volume / 10 / 5; // Spread the 10% decrease over 20 interrupts (for example)
            }
            volume -= volumeStep; // Gradually decrease volume
            if (volume <= volumeStep || decayCounter >= 220) { // After full decrement or safety check
                volumeStep = 0; // Reset step calculation for the next decrement
                decayCounter = 0; // Reset decay counter
            }
        }
    }
}
void resetDecayVariables() {
    volume = 200;
    decayCounter = 0;
	phase=0;
}

void button_1_tone(void) {

	resetDecayVariables();	
    RCAP2H=-1767 >> 8;
	RCAP2L = -1767; // Set up for note for BUTTON_1
	duration = 250; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();	
    RCAP2H=-1574 >> 8;
	RCAP2L = -1574; // Set up for note for BUTTON_1
	duration = 150; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();	
    RCAP2H=-1983 >> 8;
	RCAP2L = -1983; // Set up for note for BUTTON_1
	duration = 250; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse
	
	resetDecayVariables();
	RCAP2H=-1767 >> 8;
	RCAP2L = -1767; // Set up for note for BUTTON_1
    duration = 1000; // Adjust duration for the second tone
    //RCAP2L = -181; // Set up for 635Hz for BUTTON_1
    while (duration) {}; // Wait for the duration to elapse
	
	resetDecayVariables();	
    RCAP2H=-1486 >> 8;
	RCAP2L = -1486; // Set up for note for BUTTON_1
	duration = 250; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();	
    RCAP2H=-1574 >> 8;
	RCAP2L = -1570; // Set up for note for BUTTON_1
	duration = 150; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();	
    RCAP2H=-1983 >> 8;
	RCAP2L = -1983; // Set up for note for BUTTON_1
	duration = 250; // Adjusted duration for the first tone
    while (duration) {}; // Wait for the duration to elapse
	
	resetDecayVariables();
	RCAP2H=-1767 >> 8;
	RCAP2L = -1767; // Set up for note for BUTTON_1
    duration = 800; // Adjust duration for the second tone
    while (duration) {}; // Wait for the duration to elapse

	
}

void button_2_tone(void)    {

	resetDecayVariables();
	RCAP2H = -1763 >> 8;
	RCAP2L = -1763; // Set up for approximately 659Hz for note C
	duration = 1000; // Adjusted duration for the first tone
	while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();
	RCAP2H = -1868 >> 8;
	RCAP2L = -1868; // Set up for approximately 987Hz for note B
	duration = 250; // Adjusted duration for the second tone
	while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();
	RCAP2H = -1763 >> 8;
	RCAP2L = -1763; // Set up for approximately 659Hz for note C
	duration = 250; // Adjusted duration for the third tone
	while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();
	RCAP2H = -1571 >> 8;
	RCAP2L = -1571; // Set up for approximately 740Hz for note D
	duration = 500; // Adjusted duration for the fourth tone
	while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();
	RCAP2H = -2094 >> 8;
	RCAP2L = -2094; // Set up for approximately 987Hz for note A
	duration = 750; // Adjusted duration for the fifth tone
	while (duration) {}; // Wait for the duration to elapse

	resetDecayVariables();
	RCAP2H = -2359 >> 8;
	RCAP2L = -2359; // Set up for approximately 831Hz for note G
	duration = 750; // Adjusted duration for the sixth tone
	while (duration) {}; // Wait for the duration to elapse

	


}

void delay_ms(unsigned int milliseconds) {
    unsigned int i, j;
    for (i = 0; i < milliseconds; i++)
        for (j = 0; j < 140; j++) // This loop count needs calibration
            ; // Do nothing, just wait
}

void main(void)
{
 	WDTCN=0x0DE; 	// disable watchdog
	WDTCN=0x0AD;
	XBR2=0x40;	// enable port output
	OSCXCN=0x67;	// turn on external crystal
	TMOD=0x20;	// wait 1ms using T1 mode 2
	TH1=256-167;	// 2MHz clock, 167 counts = 1ms
	TR1 = 1;
	while ( TF1 == 0 ) { }
	while ( (OSCXCN & 0x80) == 0 ) { }
	OSCICN=0x8;	// engage! Now using 22.1184MHz
	T2CON = 4;	// timer 2, auto reload
	RCAP2H = -1;
	RCAP2L = -144;	// set up for 800Hz initially
	CKCON=0x20;
	REF0CN = 3;	// turn on voltage reference
	DAC0CN = 0x9C; 	// update on timer 2, left justified
	IE = 0xA0;	// enable timer 2 only

    while (1) {
        if (!BUTTON_1) { // If BUTTON_1 is pressed
			delay_ms(30); // Debounce delay
            while (!BUTTON_1); // Wait for BUTTON_1 to be released
            button_1_tone(); // Generate tone after release
;
        }
     while (!BUTTON_2) { // Wait for BUTTON_2 to be released        
			delay_ms(30); // Debounce delay after release
            button_2_tone(); // Process button 2 action

        }
    }
}






















