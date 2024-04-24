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
void sound();                // Manages sound effects for game events using hardware interfaces.
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
void resetPaddlePosition();  // Resets the paddle to a default or central position on the screen.
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
sbit dip8 = P1^7;
sbit but_1 =P2^6;
sbit but_2 =P2^7;		//Slow vs Fast Speed i.e easy vs hard
//---------------CONFIGS----------------------------------------------------------------------------------------------//




//////////////////I GOT TO HERE////////////////////


// Instantiate all SFRs and timers
void initDevice() {
	WDTCN = 0xDE;  // Disable watchdog timer
    WDTCN = 0xAD;
    XBR2 = 0x40;   // Enable crossbar and weak pull-ups
    //XBR0 = 0x04;   // Enable UART 0
    OSCXCN = 0x67; // Start external oscillator
    TMOD = 0x20;   // Timer 1 mode 2, auto-reload
    TH1 = -167;    // Timer reload value for 1ms
    TR1 = 1;       // Start Timer 1
    while ( TF1 == 0 ) { }          // wait 1ms
    while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
    OSCICN = 0x08; // Switch to the stabilized external oscillator
}

void init_adc(void) {
	ADC0CN = 0x8C;
	REF0CN = 0x07; // Enable on-chip voltage reference and temperature sensor.
	ADC0CF = 0x40; // setting values to read from temp sensor
	AMX0SL = 0x00;
	AMX0CF = 0x00; // Set AMUX to single-ended input mode for all inputs.
	}

void init_timer(void) {
	IE = 0xA0; // Enable Timer 2 interrupts (ET2 = 1).
	EIE2 = 0x06;  // Alternative way to enable Timer 2 interrupts (if applicable)
	// Setup auto-reload value for Timer 2 to overflow at desired intervals.
	// Assuming a 24.5 MHz system clock, for a 1ms overflow interval as an example:
	T2CON=0x00;
	RCAP2H = -2211 >> 8; // High byte of auto-reload value. .
	RCAP2L = -2211; // Low byte of auto-reload value.



	//setting up the DAC
	DAC0CN = 0x9c;
	RCAP4H = -1;
	RCAP4L = -144;


	//set up timer 4
	TR2 = 1;
	T4CON = T4CON & 0x04;
	CKCON = 0x40;
	// Enable flash writes, set necessary flags
   //  PSCTL = 0x01; // PSWE = 1, PSEE = 0

}


//---------------INTERRUPTS------------------------------------------------------------------------------------------//

void sound() interrupt 16{
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

void adc_int(void) interrupt 15 {
	AD0INT = 0; // Clear ADC interrupt flag
	dataOut = (ADC0H << 8) | ADC0L; // Read ADC data
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


void t2_int(void) interrupt 5 {
    TF2 = 0; // Clear Timer 2 interrupt flag
	adcIndex = 0; // Reset for the next cycle of readings
	ADC0CF=0x40;
	AMX0SL=0x00;
}


//---------------LCD ROUTINES---------------------------------------------------------------------------------------//
void delay(int ms) {
    ADC0CN &= ~0x80;
	i, j;

    for (i = 0; i < ms; i++)
        for (j = 0; j < 1275; j++) {
		 /* Do nothing */

		  }
	ADC0CN = 0x8C;
}



void disp_char(unsigned char row, unsigned char column, char character) {
    int i, j;
    unsigned char k;
    i = 128 * row + column;
    j = (character - 0x20) * 5;
    for(k = 0; k < 5; ++k) {
        screen[i + k] = font5x8[j+k];
    }
}

void parameter_check() {
    P1=0xFF;

	  // Initially set paddle_len based on dip1
    if (!dip1) {  // If dip1 is low
        paddle_len_p1 = 12;
    } else {
        paddle_len_p1 = 8;
    }

    // Double the paddle length if dip2 is high
    if (!dip2) {
        paddle_len_p1 *= 2;
    }

    if (!dip3) {
        game_mode=0;  	// Single Player
    } else {
        game_mode=1;	//Two Player
    }

	if (!dip4) {
       paddle_len_p2=12;  // Double the paddle length
    } else {
       paddle_len_p2=8;
    }
	if (!dip5) {
        paddle_len_p2 *= 2;
    }
	/*if (!dip7) {
       speed=60;  // Double the paddle length
    } else {
       speed=80;
    }
	*/
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




//--------------PRIMARY FUNCTIONS-----------------------------------------------------------------------------------//


// Draw borders
void drawBorders() {
	unsigned char col;
	unsigned char row;
	//Left Border
    for (row = 0; row<8; row++ ) {
		screen[128*row]=0xFF;
    }
	//Right Border
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

void drawPaddle() {


    unsigned char row = 7;
    unsigned char mask = 0xC0;  // Full byte mask to draw the paddle
    int i;

    // Ensure the paddle doesn't go off the screen edges
    //if (col < 1) col = 1;
    //if (col + PADDLE_WIDTH >= 100) col = 100 - PADDLE_WIDTH;

    for (i = 0; i < paddle_len; i++) {
        int index = row * 128 + col + i;
        screen[index+pot_avg] |= mask;  // Draw the paddle by setting bits
    }
}
void resetPaddlePosition() {
    pot_avg = 50; // Reset the paddle position to the center or desired default position
}

void switchPlayer() {
    if (game_mode == 1) {  // Only switch players if in two-player mode
        currentPlayer = (currentPlayer == 1) ? 2 : 1;
		resetPaddlePosition();
	}

}

// Function to update the score for a player and check for a new high score
void update_score(unsigned int playerIndex) {
    // Increment the score for the current player
    scores[playerIndex]++;

    // Check if the current player's score is greater than the high score
    if (scores[playerIndex] > scores[2]) {
        scores[2] = scores[playerIndex];  // Update the high score
    }
}

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


//Draw Ball
unsigned char drawBall(int x, int y)	{
    unsigned char row, col, shift, j;
    unsigned char hit = 0;
    int i;
	if (x <= 1 || x >= 100 || y<=8 || y>= 61) return 0;
	col = x-2;
	row = y-2;
	shift = row % 8;
	row = row >> 3;
	hit =0;
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


bool ballHitsPaddle() {
    int paddle_center = pot_avg + (paddle_len / 2);  // Calculate the center of the paddle
    int paddle_quarter = paddle_len / 4;  // One quarter of the paddle length

    // Calculate the relative position of the ball to the paddle's left edge
    int relative_position = ball_x - (paddle_center - paddle_len / 2);

    if (ball_x >= (paddle_center - paddle_len / 2) && ball_x <= (paddle_center + paddle_len / 2) && ball_y == bottom_bound) {
        hit_flag = 1;  // Set the flag if the ball hits the paddle

        RCAP4H = -3142 >> 8;		//A note
        RCAP4L = -3142;
        duration = 25;
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



void checkWallCollisions() {
    // Left wall collision
    if (ball_x <= left_bound) {
        x_vel = -x_vel;  // Reverse the horizontal velocity
        ball_x = left_bound;  // Prevent the ball from going out of bounds
		RCAP4H = -2354 >> 8;   // D note
        RCAP4L = -2354;
        duration = 40;
		T4CON = T4CON^0x04;
    }

    // Right wall collision
    else if (ball_x >= right_bound) {
        x_vel = -x_vel;  // Reverse the horizontal velocity
        ball_x = right_bound;  // Prevent the ball from going out of bounds
		        RCAP4H = -2354 >> 8;   // D note
        RCAP4L = -2354;
        duration = 40;
		T4CON = T4CON^0x04;
    }

    // Top wall collision
    if (ball_y <= top_bound) {
        y_vel = -y_vel;  // Reverse the vertical velocity
        ball_y = top_bound;  // Prevent the ball from going out of bounds
		RCAP4H = -2354 >> 8;   // D note
        RCAP4L = -2354;
        duration = 40;
		T4CON = T4CON^0x04;
    }
}

// Function: areAllBricksCleared
// Description: Checks if all bricks for the current player have been cleared.
// Parameters:
//   currentPlayer - the current player's turn.
// Returns:
//   bool - true if all bricks are cleared, false otherwise.
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
// Description: Resets all bricks to active for the current player.
// Parameters:
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
    //resetPaddlePosition();
    drawPaddle();
}

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



    drawBall(50,40);
    //resetPaddlePosition();
    drawPaddle();
}


void drawInfoPanel() {
    unsigned int row = 0;
    unsigned int col = INFO_START_COL;
	int i;
    // Display "P1" and P1's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '1');
    disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[0] / 10) % 10);  // Tens
    disp_char(row, col + 6, '0' + scores[0] % 10);  // Units
    row += LINE_HEIGHT;

    // Display "P2" and P2's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '2');
    disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[1] / 10) % 10);  // Tens
    disp_char(row, col + 6, '0' + scores[1] % 10);  // Units
    row += LINE_HEIGHT;

    // Display "High Score" and the actual high score
    disp_char(row, col, 'H');
    disp_char(row, col + 5, 'i');
    disp_char(row, col + 9, 'g');
    disp_char(row, col + 15, 'h');

    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (scores[2] / 10) % 10);  // Tens
    disp_char(row, col + 6, '0' + scores[2] % 10);  // Units
    row += LINE_HEIGHT;
    // Display "Health" and icons
    disp_char(row, col, 'H'); // Simplified display of "Health"
    disp_char(row, col+6, 'P'); // Simplified display of "Health"
    disp_char(row, col+12, ':'); // Simplified display of "Health"
    row += LINE_HEIGHT;

	for (i = 0; i < currentPlayer[lives-1]; i++) {
        disp_char(row, col + i * 6, '#'); // Display one '#' for each life
    }
    row += LINE_HEIGHT;
}

void gameOver() {
	health=health-1;
	blank_screen();		//gtg
	drawBorders();		//gtg
	drawInfoPanel();
	 row = 5;
	 col = 38;

    // Display "P1" and P1's score
    disp_char(row, col, 'G');
    disp_char(row, col + 6, 'A');
	disp_char(row, col + 12, 'M');
	disp_char(row, col + 18, 'E');
	row += LINE_HEIGHT;
	disp_char(row, col, 'O');
    disp_char(row, col + 6, 'V');
	disp_char(row, col + 12, 'E');
	disp_char(row, col + 18, 'R');

	refresh_screen();

	delay(100);

	T4CON = T4CON^0x04;
	duration = 250;
	RCAP4H = -3533 >> 8;   // D note
	RCAP4L = -3533;
	duration = 250; // Adjusted duration for the first tone


	delay(500);
	duration = 250;
	T4CON = T4CON^0x04;
	RCAP4H = -4743 >> 8;   // D note
	RCAP4L = -4743;
	duration = 250;


	delay(500);


	T4CON = T4CON^0x04;
	duration = 250;
	RCAP4H = -4743 >> 8;   // D note
	RCAP4L = -4743;
	duration = 500;
	 // Adjusted duration for the first tone


	delay(100);


	for(;;) {

	}
}


void updateBall() {
    ball_x += x_vel;
    ball_y += y_vel;

    left_bound = 2;
    right_bound = 98;
    top_bound = 9;
    bottom_bound = SCREEN_HEIGHT - 3;

	// Check collisions with the walls
    checkWallCollisions();

    // Check and handle collisions with bricks
    hitBrick();

    // Check and handle collision with the paddle
    if (ball_y >= bottom_bound) {
        if (!ballHitsPaddle()) {  // Check if it hits the paddle
            lives[currentPlayer-1]--;  // Lose a life

				T4CON = T4CON^0x04;
				RCAP4H = -3533 >> 8;   // D note
				RCAP4L = -3533;
				duration = 50; // Adjusted duration for the first tone

				delay(25);

				T4CON = T4CON^0x04;
				RCAP4H = -6295 >> 8;   // D note
				RCAP4L = -6295;
				duration = 50;

	   			//while (duration) {};
				T4CON = T4CON^0x04;
				switchPlayer();
				//drawPaddle();

            	delay(2000);

			if (lives[currentPlayer-1] > 0) {
                // Reset the ball position or handle life loss without stopping the game
                ball_y = 40;  // Reset to starting position
                ball_x = 50;
                resetPaddlePosition();
				drawPaddle();
				//drawBall();
				//drawBall();
				if(game_mode==1) {
					while (but_1 != 0) {
					 	blank_screen();		//gtg
						drawBorders();		//gtg
						drawInfoPanel();
						parameter_check();
						mid_game();
						refresh_screen();
    					}
					delay(250);
				}
				else {
					delay(2000);
				}

				x_vel = 0;
				y_vel = 1;  // Start the ball moving down again

            } else {
                // End the game if no lives left
                gameOver();
            }
        } else {
            y_vel = -y_vel;  // Continue the bounce if it hits the paddle
            ball_y = bottom_bound - 1;
        }
    }
}


//---------------MAIN--------------------------------------------------------------------------------------------------//


int main() {
	initDevice();
	init_timer();
	init_lcd();
    init_adc();
	while (but_2 != 0) {
	 	blank_screen();		//gtg
		drawBorders();		//gtg
		drawInfoPanel();
		parameter_check();
		pregame();

        // Optionally, include a small delay to debounce the input or reduce CPU usage
        //delay(10);  // Delay for 10 milliseconds
		refresh_screen();
    }
	delay(250);

    // The button has been pressed, proceed with game start
    blank_screen();  // Clear the screen initially

    while (1) {

		if(speedIncreased==0)	{
			delay(speed*2);
		}	else {
			delay(speed);
			}


        blank_screen();		//gtg
		drawPaddle();
		drawBorders();		//gtg
		drawInfoPanel();
		parameter_check();	//gtg
		updateBall();
        drawBricks();		//partial compliance

		drawBall(ball_x,ball_y);	//change arguments to variables


        refresh_screen();
    }
    //return 0;
}
