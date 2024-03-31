#include <C8051F020.h>
#include <stdbool.h>
#include "bell.h"

code unsigned char sine[] = { 176, 217, 244, 254, 244, 217, 176, 128, 80, 39, 12, 2, 12, 39, 80, 128 };

unsigned char phase = sizeof(sine)-1;	// current point in sine to output

unsigned int duration = 0;		// number of cycles left to output
unsigned int delay=0;
sbit BUTTON_1 = P2^7;
sbit BUTTON_2 = P2^6;
bool isReady = true;

void timer2(void) interrupt 5
{
	TF2 = 0;
	DAC0H = sine[phase];
	if ( phase < sizeof(sine)-1 )	// if mid-cycle
	{				// complete it
		phase++;
	}
	else if ( duration > 0 )	// if more cycles left to go
	{				// start a new cycle
		phase = 0;
		duration--;
	}
}

void button_1_tone(void)    {
    RCAP2L = -144; // set up for 800Hz for BUTTON_1
    duration = 800; // one second
    // Delay or wait code to maintain the tone for the duration
    while (duration); // Wait for the duration to elapse
        RCAP2L = -181; // set up for 635Hz for BUTTON_1
        duration = 635; // one second
    // Delay or wait code to maintain the tone for the duration
    //while (duration); // Wait for the duration to elapse

}
void button_2_tone(void)    {
    RCAP2L = -181; // set up for 635Hz first for BUTTON_2
    duration = 635; // one second
    // Delay or wait code to maintain the tone for the duration
    while (duration); // Wait for the duration to elapse
        RCAP2L = -144; // then set up for 800Hz for BUTTON_2
        duration = 800; // one second
    // Delay or wait code to maintain the tone for the duration
    while (duration); // Wait for the duration to elapse
        //BUTTON_2=0;
}

void delay_ms(unsigned int milliseconds) {
    unsigned int i, j;
    for (i = 0; i < milliseconds; i++)
        for (j = 0; j < 120; j++) // This loop count needs calibration
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
	REF0CN = 3;	// turn on voltage reference
	DAC0CN = 0x9C; 	// update on timer 2, left justified
	IE = 0xA0;	// enable timer 2 only

 isReady = true; // Initially, the system is ready to process button presses.
    while (1)
    {
        // Check for BUTTON_1 press and release cycle
        if (BUTTON_1)
        {
            while (BUTTON_1); // Wait for BUTTON_1 to be released
            delay_ms(20); // Debounce delay after release
            button_1_tone(); // Process button 1 action
            delay_ms(20); // Short delay after processing to stabilize system state
        	
		}

        // Check for BUTTON_2 press and release cycle
        if (BUTTON_2)
        {
            while (BUTTON_2); // Wait for BUTTON_2 to be released
            delay_ms(20); // Debounce delay after release
            button_2_tone(); // Process button 2 action
            delay_ms(20); // Short delay after processing to stabilize system state
        }
    }
}
