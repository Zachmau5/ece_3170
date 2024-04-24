//	Project:	Project 2: Breakout
//	Team:		Andrew Choffel and Zachary Hallett
//	Description:
//
//
//	Version		Date			Description
//	0.1			4/2/2024		Initial Submit
//	0.2			4/16/2024		Create Border
//	0.3			4/16/2024		Paddle Draw
//	0.4			4/20/2024		Ball Moving
//	0.5			4/20/2024		Ball Moves and bounces
//	1.0			4/20/2024		Ball stops moving when not hit by paddle
//	1.1			4/20/2024		Ball functionality is there, 2 velocities based on paddle hit position
//	1.2			4/20/2024		Added pregame and proper reset for ADC/pot
//	1.3         4/21/2024       Added 2 player mode
//
//	@PDR
//	1.4         4/22/2024       Added:
//	                            Separate paddle lengths for each player
//	                            Speed for when ball gets 3 rows 'deep'
//	                            High Score
//
//	                            Corrected:
//	                            Brick Position on screen i.e. leave room on top of brick array
//	                            Brick array spacing: 1 pixel between bricks
//	@Customer Delivery
//	2.0         4/24/2024       Cleaned up code and added more function explanations
//
//	Compliance Matrix
//	Passing req:										Comp.			Explanation
//	1)Draw Border										Full			Play area meets min requirments
//	2)Draw Ball											Full			Ball is 5x5 and displays correctly
//		min of 5 x 5
//	3)Draw Bricks										Full			Bricks are 3x5
//		3 high 5/6 wide
//	4)Bricks SHALL have 1 space			Full			1 pixel apart top AND bottom
//	5) Draw Score										Full			Score is drawn for respective player when brick is hit
//		When reset they go to zero							goes to 0 on reset. When player score > high score.
//																						Highscore <-- cur player sc
//	6) Paddle must move with POT		Full			Moves with pot, only within playable area bounds
//	7) Bricks Disapear when hit			Full			when collision with brick is hit, brick rebounds
//																						and score increments
//	8) Score ++ when brick hit			Full			When brick is hit, score[]++;
//
//	20%
//	1) no sound 										Full			multiple sounds, for brick, wall, paddle, and game over
//	2) Ocassional Crash/wedge				Full			no crashes
//
//	10%
//	1)Paddle is one size						Full		 	Each player has 4 sizes. 8,12 and a 1x or 2x multiplier
//	2)Richochet's wrong							Full			Richochet logic is solid and broken apart in 1/4s for paddle
//	3)One player only								Full			2 player and single player modes
//
//
//	5%
//	1)Ball Fails to speed up				Partial		Ball speeds up, but there is a flaw in logic.
//																						This can be solved with 2 hours of scope
//	2)High Score Fails to track			Full			As stated above, high score tracks
//

//----------------Libraries--------------------------------------------------------------------------------//
#include <C8051F020.h>		//standard 8051 header
#include "lcd.h"			//include file for ASM file
#include <stdbool.h>		//include file for bool logic
#include <stdlib.h>			//this
#include <math.h>
#include <intrins.h>  // Include intrinsic functions for the Keil compiler

//----------------VARIABLES-------------------------------------------------------------------------------//

// Variables for screen dimensions
#define SCREEN_WIDTH 128			//screen is 128 pixels wide
#define SCREEN_HEIGHT 64			//screen is 64 pixels tall
#define INFO_START_COL 106		//this holds the start of our info section
#define LINE_HEIGHT 1  //This is used for ease of readability when incrementing rows
//# is the character in the .asm file edited to be the ball
static unsigned int volume;		//This will be used to not use all 256b for sound
int left_bound = 2;						//Defines left edge of playable area (left edge +2)
int right_bound = 98;				//Defines right edge of playable area (right edge -2)
int bottom_bound = 64;				//Defines bottom edge of playable area (bottom)
int top_bound= 9;						//defines top edge of pal(1 row-1)
int duration = 40;           // Duration for each step in milliseconds

//unfiltered
unsigned int pot_sum = 0; 	// Total potentiometer reading for averaging
unsigned int pot_avg = 0;		// Average potentiometer reading
int adcIndex = 0; // Counter for ADC checks, matching the role of 'index'


long dataOut = 0;		// Read ADC data
int i,j,k,y;		// for loops

unsigned int currentPlayer = 1; // Player 1 starts first

unsigned char row, col, shift, count;
// Variables to store game settings
unsigned int paddle_len;		//var for holding paddle len
unsigned int paddle_len_p1;		//p1 paddle length
unsigned int paddle_len_p2;		//p2 paddle length
unsigned int game_mode = 0;	//0 for single player, 1 for 2 player

int starting_position_y = 40;		//24 pixles from bottom for serv pos
int starting_position_x = 50;		//50 pixles in from left for serv pos

//these next two lines are necessary for audio functioning of speakers, provided by Dr. Brown.
code unsigned char sine[] = { 176, 217, 244, 254, 244, 217, 176, 128, 80, 39, 12, 2, 12, 39, 80, 128 };
unsigned char phase = sizeof(sine)-1;	// current point in sine to output


// Variables for ball, this prints the ball as a 5x5
code unsigned char ball[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x0E};


// Variables for paddle position ADC
bit toggle = 0;             // Used as a toggle for ADC reading, to match the logic
unsigned int paddle_data = 0;   // Data read from the ADC for paddle position
unsigned int SampleCount = 0;   // Counter for ADC samples
unsigned paddle_div = 0;        // Divider for paddle position calculation

bit hit_flag = 0;  // 'bit' type if using a bit-addressable 8051 register, else use 'int'

unsigned int speed;	//this holds speed of refresh rate of screen which directly controls ball speed

// Global score array
unsigned int scores[3] = {0, 0, 0};  // scores[0]= Player 1, [1]=Player 2, and [2]=High score
int lives[2] = {3, 3};  // Initial lives for Player 1 and Player 2

unsigned int paddle_pos = 50;   // Initial position of the paddle, used to flash middle postiion

//Ball Variables
int ball_x = 50; // Initial x position of the ball
int ball_y = 40; // Initial y position of the ball
int x_vel = 0; // Ball movement along x-axis (1 for right, -1 for left)
int y_vel = 1; // Ball movement along y-axis (1 for down, -1 for up)
int health = 3;		//3 lives/turns




bool speedIncreased = false;  // Track if speed boost is applied

// Functions declarations
void initDevice();
void init_adc();
void init_timer();
void disp_char(unsigned char row, unsigned char column, char character);
void drawBorders();
void hitBrick();
void updateBall();
void drawPaddle();
unsigned char drawBall(int x, int y);
void blank_screen();
void refresh_screen();


// Functions declarations
//Initialization Functions:
void initDevice();           // Initializes device settings and configurations.
void init_adc();             // Initializes the Analog-to-Digital Converter settings.
void init_timer();           // Configures and starts timers used in the system.

//ISR/ADC
void beep();                // Manages sound effects for game events using hardware interfaces.
void t2_int();               // Interrupt service routine for Timer 2.


//Ball Functions:
bool ballHitsPaddle();       // Checks if the ball has collided with the paddle.
void updateBall();           // Updates the position and state of the ball.
void checkWallCollisions();  // Checks and handles collisions between the ball and game area boundaries.


//Display Functions:
void drawBorders();          // Draws borders around the display area.
void pregame();              // Displays pre-game information or screen.
void mid_game();             // Possibly handles mid-game transitions or displays.
void refresh_screen();       // Refreshes the screen to update the display with changes.
void blank_screen();         // Clears the entire screen.
void drawBall(int x, int y); // Draws the ball at a specified position.
void drawPaddle();           // Draws the paddle on the screen.
void drawInfoPanel();        // Draws an information panel, typically showing scores, player lives, or game status.
void repopulateBricks(char bricks[4][16]);     // Repopulates the game area with bricks.


//Game Logic Operations

void hitBrick();             // Handles the logic for when a ball hits a brick.
void update_score(unsigned int playerIndex); // Updates the score for a player and checks for new high score.
void gameOver();             // Handles game-over logic and displays.
void switchPlayer();         // Switches control between players.
bool areAllBricksCleared(char bricks[4][16]);  // Checks if all bricks have been cleared.
void parameter_check();      // Checks and adjusts game parameters based on input.
void delay(int ms);          // Provides a delay specified in milliseconds.

//Holds Array for bricks, as the bricks break, values go to 0.
xdata char bricksPlayer1[4][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

xdata char bricksPlayer2[4][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

//Dip Switch Declarations
sbit dip1 = P1^0;		//P1 Paddle Lengths, 8/12
sbit dip2 = P1^1;		//P1 Paddle Lenth multiplier
sbit dip3 = P1^2;		//Game Mode Selection
sbit dip4 = P1^3;		//P2 Paddle Lengths, 8/12
sbit dip5 = P1^4;		//P2 Paddle Lenth multiplier
sbit dip6 = P1^5;
sbit dip7 = P1^6;
sbit dip8 = P1^7;       //Slow vs Fast Speed i.e easy vs hard
sbit but_1 =P2^6;       //Button to start Game
sbit but_2 =P2^7;		//Button to serve ball (only 2 player mode)
//---------------CONFIGS----------------------------------------------------------------------------------------------//


//Necessary Formulas:
// Baud Rate: 9600
// SYSCLK:    22.1184 MHz
//Xtal = 11.0592 MHz
//
// Machine Cycle: 1.0825 usec


// initDevice :
// Initialize device hardware and start timer 1
// Also enable and stabilize oscillator 
void initDevice() {
	WDTCN = 0xDE;   // Disable watchdog timer
    WDTCN = 0xAD;   // Disable watchdog timer
    XBR2 = 0x40;   // Enable crossbar and weak pull-ups
    //XBR0 = 0x04;   // Enable UART 0
    OSCXCN = 0x67; // Start external oscillator
    TMOD = 0x20;   // Timer 1 mode 2, auto-reload
    TH1 = -167;    // Timer reload value for 1ms. 167 counts =1ms
    TR1 = 1;       // Start Timer 1
    while ( TF1 == 0 ) { }          // wait 1ms
    while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
    OSCICN = 0x08; // Switch to the stabilized external oscillator
}

// int_adc:
// This function configures the analog to digital conv.
void init_adc(void) {
	ADC0CN = 0x8C;  //Config ADC control
	REF0CN = 0x07; // Enable on-chip voltage reference.
	ADC0CF = 0x40; // setting values to read from temp sensor
	AMX0SL = 0x00;  //Select input channel for ADC read: AIN0
	AMX0CF = 0x00; // Set AMUX to single-ended input mode for all inputs.
	}
//init_timer:
//This sets up the timer2, timer 4, and DAC and all necessary interupts
void init_timer(void) {
	IE = 0xA0; // Enable Timer 2 interrupts (ET2 = 1).
	EIE2 = 0x06;  // Alternative way to enable Timer 2 interrupts (if applicable)
    //06: 0000 0110
    //Bit2: Enable Timer 4 interupts
    //Bit1: Enable ADC0 requests generated by the ADC0 conversion interrupt.
	
    // Setup auto-reload value for Timer 2 to overflow.

    //reload val = (Time in seconds/Machince cycle time)
    //then split to get high/low bye
	T2CON=0x00; //16 bit, 0.1 ms auto reload
    // We want it to cycle 2000 times (0.108 ms)
	RCAP2H = -2000>>8; // High byte of auto-reload value. .
	RCAP2L = -2000; // Low byte of auto-reload value.

	//setting up the DAC
	DAC0CN = 0x9c;  // update on timer 2, left justified
	
   // 
    RCAP4H = -1;    // 
	RCAP4L = -144;  // 
	
    //Start Timer 2
    TR2 = 1; //
	
    //Timer 4
    //Timer 4 uses SYSCLK or SYSCLK/12. We use prev
	T4CON = T4CON & 0x04; // Only checks Bit3 is high
    //Bit3 high to low transitions cause reload
	CKCON = 0x40;

}


//---------------INTERRUPTS------------------------------------------------------------------------------------------//
// ISR based off Timer 4. interrupt 16 --> ISV
// Used when generating sound effect is neccessary
// Toggles interupt flag and outputs sine wave (from above) through the DAC.
void beep() interrupt 16 {
	volume=200;
    T4CON = T4CON ^ 0x80; 	// Toggle the interrupt flag to clear it
    DAC0H = sine[phase]; 	// Output the current phase of the sine wave to the DAC

    if (phase < sizeof(sine) - 1) {
        phase++; 			// Increment the phase of the sine wave
    } else if (duration > 0) {
        phase = 0;  		// Reset the phase to the start of the sine wave
        duration--; 		// Decrement the duration for how long the sound should play
    }
    if (duration == 0) {
        T4CON = 0x00;  		// Disable the timer if the duration is 0, stopping the sound
    }
}

// ADC ISR based of Timer 2
// Processes ADC results and calculates the paddles postion.
// Magic Number Explanations:
// This calculation takes the ADC data and multiplies it by a scalar. 100L equates to the right edge of
// our payable area, so the paddle can only encompass the total area (100 Pixels)

void adc_int(void) interrupt 15 {
	AD0INT = 0;                                             // Clear ADC interrupt flag
	dataOut = (ADC0H << 8) | ADC0L;                         // Read ADC data
    dataOut = (dataOut * ((100L - paddle_len)+1)) >> 12;		//100 is the largest value (col) the paddle can be
	pot_sum += dataOut;
	count ++;

	if(count % 8 == 0)
	{
		//take the average.
		pot_avg = pot_sum/8 ;
		pot_sum = 0;
	}
}

// Timer loop used for the main loop. This loop controls game speed by throttle refresh
// rates based off DIP 8 or if the ball has hit row[1], 3rd row deep
void t2_int(void) interrupt 5 {
    TF2 = 0; // Clear Timer 2 interrupt flag
	adcIndex = 0; // Reset for the next cycle of readings
	ADC0CF=0x40;
	AMX0SL=0x00;
}
//-----------------------------------MAIN FUNCTIONS----------------------------------------------------------------------//

// Function: delay
// Description:
// This delay uses t2_int ~t2 isr. since we calculated T2 to refresh approx every 0.1ms
// We pass how many ms we want for a delay and this function will delay 
void delay(int ms) {
    ADC0CN &= ~0x80;
	i, j;

    for (i = 0; i < ms; i++)
        for (j = 0; j < 1275; j++) {
		 /* Do nothing */
		  }
	ADC0CN = 0x8C;
}

// Function: display_char
// Description:
// This displays a single character on the LCD when passing the arguments seen below:
// row--> row number on LCD where Char will be displayed Top=0, bottom=7
// column --> col number on LCD where Char will be displayed L=0, R=128
// character --> Calls ASCII char from table in ASM file.
void disp_char(unsigned char row, unsigned char column, char character) {
    int i, j;
    unsigned char k;
    i = 128 * row + column;
    j = (character - 0x20) * 5;
    for(k = 0; k < 5; ++k) {
        screen[i + k] = font5x8[j+k];
    }
}
// Function: parameter_check
// Description:
// Parameter Check calls the states of the DIPs, constantly in main, so the 
// player has constant access to be able to change the paddles size during gameplay
// Dips 1 coorelates to P1 paddle_len. If Dip1 is high (on) paddle size is 12, off=8
// Dip 2 is a multiplier (*2) to get the 4 sizes: 8, 12, 16, 24
// Dip 4 and 5 do the same for P2
// Dip 3 defines 1 or 2 player mode
// The last bit of code handles the handshake of the respective players paddle len to the 
// to the paddle_len variable used. Game was designed to implement one player first.
void parameter_check() {
    P1=0xFF;
	//Set paddle_len based on dip1
    if (!dip1) {  // If dip1 is high
        paddle_len_p1 = 12;
    } else {
        paddle_len_p1 = 8;
    }
    // Double the paddle length if dip2 is high
    if (!dip2) {
        paddle_len_p1 *= 2;
    }
    //Set Game Mode
    if (!dip3) {        //Dip 3 high (on)
        game_mode=0;  	// Single Player
    } else {
        game_mode=1;	//Two Player
    }
    //Set P2 paddle_len
	if (!dip4) {
       paddle_len_p2=12;  // Double the paddle length
    } else {
       paddle_len_p2=8;
    }
    //Double P2 padle_len if neccessary
	if (!dip5) {
        paddle_len_p2 *= 2;
    }
    //Slow vs Fast Speed
	if (dip8) {
        speed=40;
    }
	else {
       speed=70;
    }
	if (game_mode == 1) { // Two-player mode
		if (currentPlayer == 1) {
		        paddle_len = paddle_len_p1; // Paddle length for player 1
		    } else {
		        paddle_len = paddle_len_p2; // Paddle length for player 2
		    }
		} else {
		    paddle_len = paddle_len_p1; // Default to player 1's paddle length in single-player mode
		}


    }


// Function: drawborders
// Description:
// This function calls the LCD .asm subroutine to turn on certain pixels
// This draws the pixels at each respective border 
void drawBorders() {
	unsigned char col;
	unsigned char row;
	//Left Border, Draws FF on each row, first (0th Col)
    for (row = 0; row<8; row++ ) {
		screen[128*row]=0xFF;
    }
	//Right Border, Draws FF on 100th Col
	for (row = 0; row<8; row++ ) {
		screen[128*row+100]=0xFF;
    }
	//Top Border
	// Top Border with a pattern repeating every four columns
	for (col = 1; col < 100; col++) {
    	switch (col % 4) {  // Determine the column pattern by modulo 4
        	case 0:  // Fourth column in the group
           		screen[col] = 0xC7;  // All pixels set
            break;
        	case 1:  // First column in the group
            	screen[col] = 0xEF;  // Only the top pixel set
            break;
        	case 2:  // Second column in the group
            	screen[col] = 0xEF;  // Top two pixels set
            break;
        	case 3:  // Third column in the group
            	screen[col] = 0xC7;  // Top three pixels set
            break;
		}
	}
}

// Function: drawPaddle
// Description:
// This function draws the paddle on the bottom row, the mask draws the paddle 2 pixels tall
// The averaged ADC value, pot_avg, is applied at the in
void drawPaddle() {
    unsigned char row = 7;
    unsigned char mask = 0xC0;  // Full byte mask to draw the paddle 2 pixels tall
    int i;
    //iterrates over the length of the paddle
    for (i = 0; i < paddle_len; i++) {
        // finds where the paddle should be drawn. essentially screen index buffer
        int index = row * 128 + col + i; 
        // Draw the paddle at postion + pot avg, mask does what is listed above
        screen[index+pot_avg] |= mask;  
    }
}

// Function: switchPlayer
// Description:
// This changes the player to the next player
// Toggle to swap, if p1 then next is p2 and vice versa
void switchPlayer() {
    if (game_mode == 1) {  // Only switch players if in two-player mode
        currentPlayer = (currentPlayer == 1) ? 2 : 1; 
	}

}

// Function: update_score
// Description:
// Function to update the score for a player and check for a new high score
void update_score(unsigned int playerIndex) {
    // Increment the score for the current player
    scores[playerIndex]++;
    // Check if the current player's score is greater than the high score
    if (scores[playerIndex] > scores[2]) {
        scores[2] = scores[playerIndex];  // Update the high score
    }
}

// Function: drawBricks
// Description:
//  Renders bricks on the LCD display for the game. It will also draw the bricks for the respective player.
//  The brick arrays are boolean in nature, if the brick is active, 1, it will draw it, 0 it wont
//  It will work through the array and draw them.
// Magic Numbers:
//  128 = width of page in bytes
//  6 = horizontal width of each brick
//  3 = centers the brick in their space
//  0x07 & 0x70 = masks bit for drawing 2 bricks in a row by displaying top/bottom half of a display byte
void drawBricks() {
    int i, j, k;
    // Choose the current brick array based on the active player
    char (*currentBricks)[16] = (currentPlayer == 1) ? bricksPlayer1 : bricksPlayer2;

    for (i = 0; i < 16; i++) {  // Assuming 16 bricks wide as per your array size
        for (j = 0; j < 4; j++) {  // 4 rows of bricks, fitting within two "pages"
            int page = (j / 2 + 2) * 128;  // Each page starts 128 bytes apart, '+1' shifts everything down by one page
            int index = page + i * 6 + 3;  // Position for bricks, +3 for horizontal offset
            int mask = (j % 2 == 0) ? 0x07 : 0x70;  // Top or bottom half of the page

            for (k = 0; k < 5; k++) {  // Each brick is 5 pixels wide
                if (currentBricks[j][i] == 0) {  // If brick needs to be erased
                    screen[index + k] &= ~mask;  // Clear the bits using mask inversion
                } else if (currentBricks[j][i] == 1) {  // If brick needs to be drawn
                    screen[index + k] |= mask;  // Set the bits using the mask
                }
            }
        }
    }
}


// Function: drawBall
// Description:
// Draws the balll on the scree at specified coordinates and checks & returns any collision with any objects
// Provided in class by Dr. Brown
// Returns hit--> 0 = no hit, 1 - hit
unsigned char drawBall(int x, int y)	{
    unsigned char row, col, shift, j;
    unsigned char hit = 0;  //initialize detection flaag
    int i;
    //check boundaries to maek sure ball doesn't get drawn outside play area
	if (x <= 1 || x >= 100 || y<=8 || y>= 61) return 0;
	col = x-2; // get to edge of ball
	row = y-2; // get to edge of ball
	//finds position to draw the ball at
    shift = row % 8;
	row = row >> 3;
	hit =0;
    //Loop for ball for each col of the 5x5 ball 
	for (j=0, i = row * 128 + col; j<5; i++, j++)	{
		int mask= (int) ball [j] << shift;

		hit |= screen[i] & (unsigned char)mask;
		screen[i] |= (unsigned char) mask;
		if (mask & 0xFF0C) {
			hit |= screen[i + 128] & (unsigned char)(mask >> 8);
			screen[i + 128] |= (unsigned char)(mask >> 8);
		}
	}
	return hit;
}

// Function: ballHitsPaddle()
// BOOLEAN
// Description:
// Checks to see if the ball collides with the paddle. Then, it will calculate the center/half/and quarters of the paddle
// to determine what rebounce incident angle.
// Depending on which quarter it hits
//  | x |   |   |   | =(up 1, left 2)
//  |   | x |   |   | =(up 1, left 1)
//  |   |   | x |   | =(up 1, right 1)
//  |   |   |   | x | =(up 1, right 2)
//  |--paddle_len--|
//  This function will also set or clear hit flags as well as play a tone when a collisioni is hit
bool ballHitsPaddle() {
    int paddle_center = pot_avg + (paddle_len / 2);  // Calculate the center of the paddle
    int paddle_quarter = paddle_len / 4;  // One quarter of the paddle length

    // Calculate the relative position of the ball to the paddle's left edge
    int relative_position = ball_x - (paddle_center - paddle_len / 2);

    if (ball_x >= (paddle_center - paddle_len / 2) && ball_x <= (paddle_center + paddle_len / 2) && ball_y == bottom_bound) {
        hit_flag = 1;  // Set the flag if the ball hits the paddle

        //RCAP = [22.118MHz/ ( Freq Des in Hz *16)] /2

        // D note 293 Hz    
        RCAP4H = -2354 >> 8;   
        RCAP4L = -2354;
        duration = 40;
		T4CON = T4CON^0x04;
        // Determine the ball's new velocities based on hit position
        if (relative_position < paddle_quarter) {
            // Ball hits the first quarter
            x_vel = -2;  // Sharper angle to the left
            y_vel = 1;  // Standard bounce upward
        } else if (relative_position < 2 * paddle_quarter) {
            // Ball hits the second quarter
            x_vel = -1;  // Moderate angle to the left
            y_vel = 1;  // Standard bounce upward
        } else if (relative_position < 3 * paddle_quarter) {
            // Ball hits the third quarter
            x_vel = 1;  // Moderate angle to the right
            y_vel = 1;  // Standard bounce upward
        } else {
            // Ball hits the fourth quarter
            x_vel = 2;  // Sharper angle to the right
            y_vel = 1;  // Standard bounce upward
        }

        return true;
    } else {
        hit_flag = 0;  // Clear the flag if there is no hit
        return false;
    }
}


// Function: checkWallCollisions
// Description:
// Checks for collisions of the ball on the walls of the play area. If a collision is detected,
// each case handles the bouce accordingly
// Sound will also play
void checkWallCollisions() {
    // Left wall collision
    if (ball_x <= left_bound) {
        x_vel = -x_vel;  // Reverse the horizontal velocity
        ball_x = left_bound;  // Prevent the ball from going out of bounds
	    //A = 220Hz
        RCAP4H = -3142 >> 8;		
        RCAP4L = -3142;
        duration = 25;
        T4CON = T4CON^0x04;
      
    }

    // Right wall collision
    else if (ball_x >= right_bound) {
        x_vel = -x_vel;  // Reverse the horizontal velocity
        ball_x = right_bound;  // Prevent the ball from going out of bounds
		
	    //A = 220Hz
        RCAP4H = -3142 >> 8;		
        RCAP4L = -3142;
        duration = 25;
        T4CON = T4CON^0x04;
    }

    // Top wall collision
    if (ball_y <= top_bound) {
        y_vel = -y_vel;  // Reverse the vertical velocity
        ball_y = top_bound;  // Prevent the ball from going out of bounds
		
	    //A = 220Hz
        RCAP4H = -3142 >> 8;		
        RCAP4L = -3142;
        duration = 25;
        T4CON = T4CON^0x04;
    }
}

// Function: areAllBricksCleared
// Description: Checks if all bricks for the current player have been cleared.
//  Parameters:
//   currentPlayer - the current player's turn.
// Returns:
//  bool - true if all bricks are cleared, false otherwise.
bool areAllBricksCleared() {
    char (*currentBricks)[16] = (currentPlayer == 1) ? bricksPlayer1 : bricksPlayer2; // Choose the correct brick array based on currentPlayer

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 16; j++) {
            if (currentBricks[i][j] == 1) {
                return false; // Active brick found, return false
            }
        }
    }
    return true; // No active bricks found, return true
}

// Function: repopulateBricks
// Description: 
// Resets all bricks to active for the current player.
//   currentPlayer - the current player's turn.
void repopulateBricks() {
    char (*currentBricks)[16] = (currentPlayer == 1) ? bricksPlayer1 : bricksPlayer2; // Choose the correct brick array based on currentPlayer

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 16; j++) {
            currentBricks[i][j] = 1; // Set each brick to active
        }
    }
    speedIncreased = false; // Reset the speed increase flag upon repopulating bricks
}


// Function: hitBrick
// Description: Checks for collisions between the ball and the bricks. If a collision is detected,
//              the brick is deactivated, the ball's velocity is reversed, the score is updated,
//              and sound effects are triggered. Additionally, it checks if all bricks have been cleared
//              to possibly repopulate the game area and reset the game state.
void hitBrick() {
    // Determine which brick array to use based on the current player
    char (*currentBricks)[16] = (currentPlayer == 1) ? bricksPlayer1 : bricksPlayer2;
    bool allCleared = false;
    // Calculate the brick index from the ball's position
    int x_index = (ball_x - 3) / 6; // Horizontal index of the brick
    int y_index = (ball_y - 15) / 5; // Vertical index of the brick

    // Ensure the calculated indices are within the valid range
    if (x_index >= 0 && x_index < 16 && y_index >= 0 && y_index < 4) {
        // Check if the brick at the calculated index is active
        if (currentBricks[y_index][x_index] == 1) {
            // Deactivate the hit brick
            currentBricks[y_index][x_index] = 0;

            // Reverse the ball's vertical velocity to simulate a bounce
            y_vel = -y_vel;

            // Update the score for the current player
            update_score(currentPlayer - 1); // Assume a function update_score exists to handle score updates

            // Trigger sound effect for hitting a brick
            RCAP4H = -4321 >> 8;  // Setup timer value for sound frequency
            RCAP4L = -4321;
            duration = 25;  // Set the duration for the sound
            T4CON = T4CON^0x04;  // Toggle the sound timer to start/stop the sound

            // Check if all bricks have been cleared
            bool allCleared = areAllBricksCleared(currentBricks);
            if (allCleared) {
                // Repopulate the bricks and reset the game state if all bricks are cleared
                repopulateBricks(currentBricks);
                ball_x = starting_position_x;  // Reset the ball to the starting x position
                ball_y = starting_position_y;  // Reset the ball to the starting y position
                pot_avg = SCREEN_WIDTH / 2;  // Center the paddle
                delay(2000);  // Pause before resuming play
                x_vel = 0;  // Reset horizontal velocity
                y_vel = 1;  // Set initial downward velocity
            }

            // Exit function after handling collision to avoid further processing
            return;
        }
    }
}

// Function: hitBrick
// Description: 
// Checks for collisions between the ball and the bricks. If a collision is detected,
// the brick is deactivated, the ball's velocity is reversed, the score is updated,
// and sound effects are triggered. Additionally, it checks if all bricks have been cleared
// to possibly repopulate the game area and reset the game state.
void hitBrick() {
    // Determine which brick array to use based on the current player
    char (*currentBricks)[16] = (currentPlayer == 1) ? bricksPlayer1 : bricksPlayer2;
    bool allCleared = false;
    // Calculate the brick index from the ball's position
    int x_index = (ball_x - 3) / 6; // Horizontal index of the brick
    int y_index = (ball_y - 15) / 5; // Vertical index of the brick

    // Ensure the calculated indices are within the valid range
    if (x_index >= 0 && x_index < 16 && y_index >= 0 && y_index < 4) {
        // Check if the brick at the calculated index is active
        if (currentBricks[y_index][x_index] == 1) {
            // Deactivate the hit brick
            currentBricks[y_index][x_index] = 0;

            // Reverse the ball's vertical velocity to simulate a bounce
            y_vel = -y_vel;

            // Update the score for the current player
            update_score(currentPlayer - 1); // Assume a function update_score exists to handle score updates

            // Trigger sound effect for hitting a brick
            RCAP4H = -4321 >> 8;  // Setup timer value for sound
            RCAP4L = -4321;
            duration = 25;  // Set the duration for the sound
            T4CON = T4CON^0x04;  // Toggle the sound timer to start/stop the sound

            // Check if all bricks have been cleared
            bool allCleared = areAllBricksCleared(currentBricks);
            if (allCleared) {
                // Repopulate the bricks and reset the game state if all bricks are cleared
                repopulateBricks(currentBricks);
                ball_x = starting_position_x;  // Reset the ball to the starting x position
                ball_y = starting_position_y;  // Reset the ball to the starting y position
                pot_avg = SCREEN_WIDTH / 2;  // Center the paddle
                delay(2000);  // Pause before resuming play
                x_vel = 0;  // Reset horizontal velocity
                y_vel = 1;  // Set initial downward velocity
            }

            // Exit function after handling collision to avoid further processing
            return;
        }
    }
}

// Function: pregame
// Description:
// Displays the border, info panel, and then the paddle. The paddle will update live so
// the player can place there paddle how they want when serving as well as show paddle_len 
// It will also show the game mode. row and col are instantiated for eas of access, and increase
// 6 pixels as characters are 5 pixels wide + 1 pixel space
void pregame() {
    unsigned int row = 1; 
    unsigned int col = 6;
    disp_char(row, col, 'P');
    disp_char(row, col + 6, 'L');
    disp_char(row, col + 12, 'A');
    disp_char(row, col + 18, 'Y');
    disp_char(row, col + 24, 'E');
    disp_char(row, col + 30, 'R');
    disp_char(row, col + 42, 'R');
    disp_char(row, col + 48, 'E');
    disp_char(row, col + 54, 'A');
    disp_char(row, col + 60, 'D');
    disp_char(row, col + 66, 'Y');
    disp_char(row, col + 72, '?');

    row += LINE_HEIGHT;
    col = 18;
    disp_char(row, col, 'P');
    disp_char(row, col + 6, 'U');
    disp_char(row, col + 12, 'S');
    disp_char(row, col + 18, 'H');
    disp_char(row, col + 30, 'L');
    disp_char(row, col + 36, 'E');
    disp_char(row, col + 42, 'F');
    disp_char(row, col + 48, 'T');
    row += LINE_HEIGHT;
    col = 6;
    disp_char(row, col, 'B');
    disp_char(row, col + 6, 'U');
    disp_char(row, col + 12, 'T');
    disp_char(row, col + 18, 'T');
    disp_char(row, col + 24, 'O');
    disp_char(row, col + 30, 'N');
    disp_char(row, col + 42, 'T');
    disp_char(row, col + 48, '0');
    disp_char(row, col + 60, 'S');
    disp_char(row, col + 66, 'T');
    disp_char(row, col + 72, 'A');
    disp_char(row, col + 78, 'R');
    disp_char(row, col + 84, 'T');

	row += LINE_HEIGHT;


    // Display game mode
	col=6;
	row += LINE_HEIGHT;
    if (game_mode == 0) {

        // Display for Single Player mode
        disp_char(row, col, 'O');
        disp_char(row, col + 6, 'n');
        disp_char(row, col + 12, 'e');
        disp_char(row, col + 18, ' ');
		row += LINE_HEIGHT;
		col=6;
        disp_char(row, col, 'P');
        disp_char(row, col + 6, 'l');
        disp_char(row, col + 12, 'a');
        disp_char(row, col + 18, 'y');
        disp_char(row, col + 24, 'e');
        disp_char(row, col + 32, 'r');
    } else {
        // Display for Two Player mode
        col=72;
		disp_char(row, col, 'T');
        disp_char(row, col + 6, 'w');
        disp_char(row, col + 12, 'o');

		row += LINE_HEIGHT;
		col=50;
 		disp_char(row, col + 6, 'P');
        disp_char(row, col + 12, 'l');
        disp_char(row, col + 18, 'a');
        disp_char(row, col + 24, 'y');
        disp_char(row, col + 30, 'e');
        disp_char(row, col + 36, 'r');
    }

    drawBall(50,40);
    drawPaddle();
}


// Function: mid_game
// Description:
// If only in 2 player mode, this promopts the players to pass the board and 
// then it waits for the next player to press the right button to prompt a serve
// The paddle will be drawn as well so the next player can place the paddle where
// they want.
void mid_game() {


	unsigned int row = 1;
    unsigned int col = 12;

	disp_char(row, col, 'C');
    disp_char(row, col + 6, 'H');
    disp_char(row, col + 12, 'A');
    disp_char(row, col + 18, 'N');
    disp_char(row, col + 24, 'G');
    disp_char(row, col + 30, 'E');

    disp_char(row, col + 42, 'P');
    disp_char(row, col + 48, 'L');
    disp_char(row, col + 54, 'A');
    disp_char(row, col + 60, 'Y');
    disp_char(row, col + 66, 'E');
    disp_char(row, col + 72, 'R');

	row += LINE_HEIGHT;

    disp_char(row, col, 'P');
    disp_char(row, col + 6, 'L');
    disp_char(row, col + 12, 'A');
    disp_char(row, col + 18, 'Y');
    disp_char(row, col + 24, 'E');
    disp_char(row, col + 30, 'R');

    disp_char(row, col + 42, 'R');
    disp_char(row, col + 48, 'E');
    disp_char(row, col + 54, 'A');
    disp_char(row, col + 60, 'D');
    disp_char(row, col + 66, 'Y');
    disp_char(row, col + 72, '?');

    row += LINE_HEIGHT;
    col = 24;
    disp_char(row, col, 'P');
    disp_char(row, col + 6, 'U');
    disp_char(row, col + 12, 'S');
    disp_char(row, col + 18, 'H');
    disp_char(row, col + 30, 'R');
    disp_char(row, col + 36, 'I');
    disp_char(row, col + 42, 'G');
	disp_char(row, col + 48, 'H');
    disp_char(row, col + 54, 'T');
    row += LINE_HEIGHT;
    col = 8;
    disp_char(row, col, 'B');
    disp_char(row, col + 6, 'U');
    disp_char(row, col + 12, 'T');
    disp_char(row, col + 18, 'T');
    disp_char(row, col + 24, 'O');
    disp_char(row, col + 30, 'N');
    disp_char(row, col + 42, 'T');
    disp_char(row, col + 48, '0');
    disp_char(row, col + 60, 'S');
    disp_char(row, col + 66, 'E');
    disp_char(row, col + 72, 'R');
    disp_char(row, col + 78, 'V');
    disp_char(row, col + 84, 'E');

	row += LINE_HEIGHT;

    drawBall(50,40); // draw ball at where the serve will originate
    drawPaddle();
}


// Function: drawInfoPanel
// Description: 
// Displays the scores for Player 1, Player 2, the high score, and the current health of the active player.
// It updates the screen with text and numeric values for each player's score, the high score, and visual representation of health using '#'.
void drawInfoPanel() {
    unsigned int row = 0;
    unsigned int col = INFO_START_COL; // Starting column for the info panel
    int i; // Loop variable for displaying health icons

    // Display "P1" and Player 1's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '1');
    disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[0] / 10) % 10);  // Tens place of Player 1's score
    disp_char(row, col + 6, '0' + scores[0] % 10);     // Units place of Player 1's score
    row += LINE_HEIGHT;

    // Display "P2" and Player 2's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '2');
    disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[1] / 10) % 10);  // Tens place of Player 2's score
    disp_char(row, col + 6, '0' + scores[1] % 10);     // Units place of Player 2's score
    row += LINE_HEIGHT;

    // Display "High Score" and the actual high score
    disp_char(row, col, 'H');
    disp_char(row, col + 5, 'i');
    disp_char(row, col + 9, 'g');
    disp_char(row, col + 15, 'h');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[2] / 10) % 10);  // Tens place of the high score
    disp_char(row, col + 6, '0' + scores[2] % 10);     // Units place of the high score
    row += LINE_HEIGHT;

    // Display "Health" label and icons for the current player's remaining lives
    disp_char(row, col, 'H');
    disp_char(row, col + 6, 'P');
    disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    // Draw health icons ('#' for each remaining life)
    //# is the character in the .asm file edited to just be the ball
    for (i = 0; i < lives[currentPlayer-1]; i++) {
        disp_char(row, col + i * 6, '#');
    }
    row += LINE_HEIGHT;
}

// Function: gameOver
// Description: 
// Handles the end of the game by displaying a "GAME OVER" message, playing a sound sequence, and effectively halting the game.
void gameOver() {
    health = health - 1; // Decrement the health
    blank_screen();      // Clear the screen for fresh display
    drawBorders();       // Draw the game borders again
    drawInfoPanel();     // Redraw the info panel to update scores and health

    // Set the text display position for "GAME OVER"
    unsigned int row = 5;
    unsigned int col = 38;

    // Display "GAME"
    disp_char(row, col, 'G');
    disp_char(row, col + 6, 'A');
    disp_char(row, col + 12, 'M');
    disp_char(row, col + 18, 'E');
    row += LINE_HEIGHT;

    // Display "OVER"
    disp_char(row, col, 'O');
    disp_char(row, col + 6, 'V');
    disp_char(row, col + 12, 'E');
    disp_char(row, col + 18, 'R');

    refresh_screen(); // Refresh the display to show the changes

    // Short delay before playing sound
    delay(100);

    // Sound sequence configuration
    T4CON = T4CON^0x04; // Toggle Timer 4 control register for sound
    RCAP4H = -3533 >> 8; // High byte for timer 4 capture register (D note)
    RCAP4L = -3533;      // Low byte for timer 4 capture register
    duration = 250;      // Set duration of the sound

    // Delay for half a second
    delay(500);

    // Play second sound
    T4CON = T4CON^0x04;
    RCAP4H = -4743 >> 8; // High byte for timer 4 capture register (D note)
    RCAP4L = -4743;      // Low byte for timer 4 capture register
    duration = 250;

    // Additional delay
    delay(500);

    // Continue sound
    T4CON = T4CON^0x04;
    duration = 500; // Extend the duration for the last tone

    // Final delay
    delay(100);

    // Infinite loop to halt gameplay
    for(;;) {
        // This loop will continue indefinitely, stopping the game
    }
}

// Function: updateBall
// Description: Updates the position of the ball based on its velocity, checks for collisions with the walls, bricks, and the paddle,
// and handles the game logic when the ball hits these objects or when it misses the paddle.
void updateBall() {
    // Update ball position based on its velocity
    ball_x += x_vel;
    ball_y += y_vel;

    // Define the boundaries within the screen
    left_bound = 2;
    right_bound = 98;
    top_bound = 9;
    bottom_bound = SCREEN_HEIGHT - 3;

    // Check for collisions with the walls
    checkWallCollisions();

    // Check for collisions with bricks
    hitBrick();

    // Check if the ball reaches the bottom of the screen (near the paddle)
    if (ball_y >= bottom_bound) {
        if (!ballHitsPaddle()) {  // Check if the ball does not hit the paddle
            lives[currentPlayer-1]--;  // Decrement life for the current player

            // Play a sound indicating the ball missed the paddle
            T4CON = T4CON^0x04;
            RCAP4H = -3533 >> 8;  // D note, high byte
            RCAP4L = -3533;       // D note, low byte
            duration = 50;        // Duration for the sound

            delay(25);  // Short delay before next sound

            // Play another sound for feedback
            T4CON = T4CON^0x04;
            RCAP4H = -6295 >> 8;  // Another D note, high byte
            RCAP4L = -6295;       // Another D note, low byte
            duration = 50;

            // Switch player if in two-player mode
            switchPlayer();

            delay(2000);  // Delay before restarting the action

            if (lives[currentPlayer-1] > 0) {
                // Reset ball position and game state if the player still has lives
                ball_y = 40;  // Reset to a starting position vertically
                ball_x = 50;  // Reset to a starting position horizontally
                resetPaddlePosition();  // Reset the paddle position
                drawPaddle();  // Redraw the paddle

                // Wait for player readiness in two-player mode
                if(game_mode==1) {
                    while (but_1 != 0) {
                        blank_screen();  // Clear screen
                        drawBorders();   // Redraw borders
                        drawInfoPanel(); // Redraw information panel
                        mid_game();      // Show mid-game menu
                        refresh_screen();  // Refresh the display
                    }
                    delay(250);  // Brief delay to handle button debounce
                } else {
                    delay(2000);  // Longer delay in single-player mode
                }

                x_vel = 0;  // Stop horizontal movement
                y_vel = 1;  // Start the ball moving downward again
            } else {
                // Call gameOver if no lives are left
                gameOver();
            }
        } else {
            // If the ball hits the paddle, reverse its vertical direction
            y_vel = -y_vel;
            ball_y = bottom_bound - 1;  // Adjust ball position to just above the paddle
        }
    }
}

//---------------MAIN--------------------------------------------------------------------------------------------------//
// Description: Initializes the system, displays pregame information, and handles the main game loop.
int main() {
    initDevice();        // Initialize device settings and configurations
    init_timer();        // Set up timers for game timing and events
    init_lcd();          // Initialize LCD display for output
    init_adc();          // Set up ADC for reading inputs (e.g., paddle position)

    // Wait for the start button (but_2) to be pressed
    while (but_2 != 0) {
        blank_screen();  // Clear the screen for clean drawing
        drawBorders();   // Draw game borders
        drawInfoPanel(); // Display scores and other game info
        parameter_check(); // Check and update game parameters based on settings (like dip switches)
        pregame();       // Display pregame information prompting the player to start

        // Optionally, include a small delay to debounce the input or reduce CPU usage
        refresh_screen();  // Refresh the display to show updates
    }
    delay(250);  // Short delay after button press to prevent bouncing effects or multiple reads

    // Main game loop starts after button press
    blank_screen();  // Clear the screen before starting the game

    while (1) {
        // Speed control based on whether speed has been increased
        if(speedIncreased==0) {
            delay(speed*2);  // Slower delay if speed not increased
        } else {
            delay(speed);  // Faster gameplay when speed is increased
        }

        blank_screen();    // Clear the screen before each redraw
        drawPaddle();      // Draw the paddle based on current position
        drawBorders();     // Redraw game borders
        drawInfoPanel();   // Update and display game info like scores
        parameter_check(); // Recheck parameters in case of any change during gameplay
        updateBall();      // Move the ball and handle interactions
        drawBricks();      // Draw and update the status of bricks

        drawBall(ball_x, ball_y);  // Draw the ball at its current position

        refresh_screen();  // Refresh the display to show all updates
    }
    // Note: return 0; is commented out as most embedded systems will run indefinitely
}
