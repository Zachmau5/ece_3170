#include <C8051F020.h>
#include "lcd.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
//----------------VARIABLES-------------------------------------------------------------------------------------------//
/*
Project:	Project 2
Team:		Andrew Choffel and Zachary Hallett
Description:


Version 	Date			Description
0.1			4/2/2024		Initial Submit
0.2			4/16/2024		Create Border
0.3			4/16/2024		Paddle Draw
0.4			4/20/2024		Ball Moving
0.5			4/20/2024		Ball Moves and bounces
1.0			4/20/2024		Ball stops moving when not hit by paddle	
1.1			4/20/2024		Ball functionality is there, 2 velocities based on paddle hit position
1.2			4/20/204		Added pregame and proper reset for ADC/pot

TODO to meet minimum passing req:				Compliance
1) Draw Border									Full
2) Draw Ball									Full
	min of 5 x 5 
3) Draw Bricks									Full
	3 high 5/6 wide
4) Bricks SHALL have 1 space					Full
5) Draw Score									Full
	When reset they go to zero
	Except High Score
6) Paddle must move with POT					Full
7) Bricks Disapear when hit						Full
8) Score increments when brick hit				Full

20%
1) no sound 									Non Comp
2) Ocassional Crash/wedge

10%
1)Paddle is one size							Non Comp
2)Richochet in wrong direction					Partial
3)One player only


5%
1)Ball Fails to speed up						Non Comp
2)High Score Fails to track						Non Comp



For Sunday 4/21/2024
Priority:
1) Calculate timer stuff
1) Sound
3) Dip Switches
4) 2 player mode
	cause memory issues probably


*/



// Variables for screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BRICK_ROWS 3
#define BRICK_COLS 10
#define INFO_START_COL 106
#define INFO_WIDTH 27  // You have columns 101 to 128, inclusive
#define LINE_HEIGHT 1  // Assuming each line of text uses 8 rows (adjust as per your font)
// Define max and min macros
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define BRICK_START_X 10
#define BRICK_START_Y 10
#define BRICK_WIDTH 5
#define BRICK_HEIGHT 3
#define BUFFER 1
#define ROWS 5
#define COLS 13


int left_bound = 5;
int right_bound = 100;
int bottom_bound = 60;
int top_bound= 20;

//unfiltered
unsigned int pot_sum = 0; // Total potentiometer reading for averaging
unsigned int pot_avg = 0; // Average potentiometer reading
unsigned int pot_val = 0; 
int adcIndex = 0; // Counter for ADC checks, matching the role of 'index'
bit averaged = 0; // Flag to indicate completion of averaging
bit hit1;
bit hitBorder_flag = 0;
bit hitBrick_flag = 0;
long dataOut = 0;
int i,j,k,y;


unsigned char row, col, shift, count;
// Variables to store game settings
unsigned int paddle_len = 12;
unsigned int two_player_mode = 0;

int starting_position_y = 40;
int starting_position_x = 50;





// Declare a 2D array to store the presence (1) or absence (0) of bricks


// Variables for ball
code unsigned char ball[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x0E};


// Variables for paddle position ADC
bit toggle = 0;             // Used as a toggle for ADC reading, to match the logic
unsigned int paddle_data = 0;   // Data read from the ADC for paddle position
unsigned int SampleCount = 0;   // Counter for ADC samples
unsigned paddle_div = 0;        // Divider for paddle position calculation
int paddle_border_left=0;
int paddle_border_right=0;
int hit_location = 0; //saves hit brick position
bit hit_flag = 0;  // 'bit' type if using a bit-addressable 8051 register, else use 'int'

// Variables for 1 or 2 players
unsigned int player_count = 1;  // Number of players (1 or 2)
// Global score array
unsigned int scores[3] = {0, 0, 0};  // scores[0] for Player 1,

// Variables for fast or slow ball speed
unsigned int speed_toggle = 0;    // Speed of the ball

unsigned int paddle_pos = 50;   // Initial position of the paddle

//Ball Variables
int ball_x = 50; // Initial x position of the ball
int ball_y = 40; // Initial y position of the ball
int x_vel = 0; // Ball movement along x-axis (1 for right, -1 for left)
int y_vel = 1; // Ball movement along y-axis (1 for down, -1 for up)
int health = 3;




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



xdata char bricks [4][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

sbit dip1 = P1^0;
sbit dip2 = P1^1;
sbit dip3 = P1^2;
sbit dip4 = P1^3;
sbit dip5 = P1^4;
sbit dip6 = P1^5;
sbit dip7 = P1^6;
sbit dip8 = P1^7;
sbit but_1 =P2^6;
sbit but_2 =P2^7; 
//---------------CONFIGS----------------------------------------------------------------------------------------------//

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
	EIE2 = 0x02;  // Alternative way to enable Timer 2 interrupts (if applicable)
	// Setup auto-reload value for Timer 2 to overflow at desired intervals.
	// Assuming a 24.5 MHz system clock, for a 1ms overflow interval as an example:
	T2CON=0x00;
	RCAP2H = -2211 >> 8; // High byte of auto-reload value. .
	RCAP2L = -2211; // Low byte of auto-reload value.
	TR2=1;
	
	// Enable flash writes, set necessary flags
   //  PSCTL = 0x01; // PSWE = 1, PSEE = 0
	
}


//---------------INTERRUPTS------------------------------------------------------------------------------------------//


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

void disp_char(unsigned char row, unsigned char column, char character) {
    int i, j;
    unsigned char k;
    i = 128 * row + column;
    j = (character - 0x20) * 5;
    for(k = 0; k < 5; ++k) {
        screen[i + k] = font5x8[j+k];
    }
}

void parameter_check(unsigned int paddle_len, unsigned int speed_toggle, unsigned int two_player_mode) {
    //P1=0xFF;
	
	if (dip1 == 0) { // Check if P1.0 is high
        paddle_len = 8;
    } else {
        paddle_len = 24;
    }

    if (P1 & 0x04) { // Check if P1.2 is high
        speed_toggle = 1; // Speed toggle is activated
    } else {
        speed_toggle = 0; // Speed toggle is deactivated
    }

    if (P1 & 0x08) { // Check if P1.3 is high
        two_player_mode = 1; // Two player mode is activated
    } else {
        two_player_mode = 0; // Two player mode is deactivated
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


void drawBricks() {
    int i, j, k; // i=row, j=col, k=width
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 16; ++j) {
            if (bricks[i][j] == 1) {  // Only draw if the brick is active
                for (k = 0; k < 5; ++k) {  // Each brick is 5 pixels wide
                    screen[128 * (i + 1) + (6 * j) + k + 3] = 0x70;  // Set the brick pixel
                }
            }
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

	for (i = 0; i < health; i++) {
        disp_char(row, col + i * 6, '#'); // Display one '#' for each life
    }
    row += LINE_HEIGHT;
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

/*
bool ballHitsPaddle() {
    int paddle_center = pot_avg + (paddle_len / 2);  // Calculate the center of the paddle
    unsigned int paddle_half_width = paddle_len / 2;  // Half of the middle half (1/4 of total length)

    // Correctly compute the paddle hit zone from the center
    int paddle_border_left= (int) paddle_center - paddle_half_width;
	
	int paddle_border_right=paddle_center + paddle_half_width;
	if (ball_x >= (paddle_border_left) && 
        ball_x <= (paddle_border_right) && 
        ball_y == bottom_bound) {
        hit_flag = 1;  // Set the flag if the ball hits the paddle
        return true;
    } else {
        hit_flag = 0;  // Clear the flag if there is no hit
        return false;
    }
}
*/
bool ballHitsPaddle() {
    int paddle_center = pot_avg + (paddle_len / 2);  // Calculate the center of the paddle
    int paddle_quarter = paddle_len / 4;  // One quarter of the paddle length

    // Calculate the relative position of the ball to the paddle's left edge
    int relative_position = ball_x - (paddle_center - paddle_len / 2);

    if (ball_x >= (paddle_center - paddle_len / 2) && ball_x <= (paddle_center + paddle_len / 2) && ball_y == bottom_bound) {
        hit_flag = 1;  // Set the flag if the ball hits the paddle

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
    }

    // Right wall collision
    else if (ball_x >= right_bound) {
        x_vel = -x_vel;  // Reverse the horizontal velocity
        ball_x = right_bound;  // Prevent the ball from going out of bounds
    }

    // Top wall collision
    if (ball_y <= top_bound) {
        y_vel = -y_vel;  // Reverse the vertical velocity
        ball_y = top_bound;  // Prevent the ball from going out of bounds
    }
}

// React ball-to-brick
void hitBrick() {
	for (i = 0; i < 4; i++) {
        for (j = 0; j < 16; j++) {
            int brick_x_start = j * (5 + 1) + 3; // Assuming each brick is 5 pixels wide and has 1 pixel space
            int brick_y_start = (i + 1) * 8; // Assuming each row starts 8 pixels lower than the previous
            int brick_x_end = brick_x_start + 5;
            int brick_y_end = brick_y_start + 8; // Assuming each brick is 8 pixels tall

            // Check collision with the ball
            if (ball_x >= brick_x_start && ball_x <= brick_x_end &&
                ball_y >= brick_y_start && ball_y <= brick_y_end) {
                if (bricks[i][j] == 1) {  // Brick is active
                    bricks[i][j] = 0;  // Deactivate the brick
                    y_vel = -y_vel;  // Reverse the ball's vertical velocity
                    // Update score when a brick is hit
                    scores[0] += 1;  // Increase score, assuming Player 1 is playing
                    return;  // Exit after handling the collision
				}
			}
		}
	}
}
/*
void hitBrick() {
    int radius = 2; // Assuming a radius of 2 for the ball

    // Calculate the extreme boundaries of the ball carefully to avoid underflow/overflow
    int ball_left = max(0, ball_x - radius);
    int ball_right = min(SCREEN_WIDTH - 1, ball_x + radius);
    int ball_top = max(0, ball_y - radius -4);
    int ball_bottom = min(SCREEN_HEIGHT - 1, ball_y + radius);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 16; j++) {
            if (bricks[i][j] == 1) {
                int brick_x_start = j * (BRICK_WIDTH + 1) + 3;
                int brick_y_start = i * 8;  // Each row is 8 pixels apart
                int brick_x_end = brick_x_start + BRICK_WIDTH;
                int brick_y_end = (brick_y_start + 8);

                if (ball_right >= brick_x_start && ball_left <= brick_x_end &&
                    ball_bottom >= brick_y_start && ball_top <= brick_y_end) {
                    bricks[i][j] = 0;  // Deactivate the brick
                    y_vel = -y_vel;  // Reverse the ball's vertical velocity

                    if (ball_left < brick_x_start || ball_right > brick_x_end) {
                        x_vel = -x_vel;  // Reverse horizontal velocity if needed
                    }

                    scores[0] += 1;  // Increment the score
                    return;  // Handle one collision at a time
                }
            }
        }
    }
}
*/


void pregame()	{
	unsigned int row = 1;
    unsigned int col = 12;
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
	col=24;
	disp_char(row, col, 'P');
    disp_char(row, col + 6, 'U');
	disp_char(row, col + 12, 'S');
	disp_char(row, col + 18, 'H');
	disp_char(row, col + 30, 'L');
	disp_char(row, col + 36, 'E');
	disp_char(row, col + 42, 'F');
	disp_char(row, col + 48, 'T');
	row += LINE_HEIGHT;
	col=12;
	disp_char(row, col, 'B');
    disp_char(row, col + 6, 'U');
	disp_char(row, col + 12, 'T');
	disp_char(row, col + 18, 'T');
	disp_char(row, col + 24, 'O');
	disp_char(row, col + 30, 'N');

    disp_char(row, col + 42, 'T');
	disp_char(row, col + 48, '0');
	disp_char(row, col + 54, 'S');
	disp_char(row, col + 60, 'T');
	disp_char(row, col + 66, 'A');
	disp_char(row, col + 72, 'R');
	disp_char(row, col + 78, 'T');

	drawBall(50,40);
	//resetPaddlePosition();
	drawPaddle();
}







void gameOver() {

	unsigned int row = 5;
    unsigned int col = 38;

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

	
}
void delay(int ms) {
    ADC0CN &= ~0x80;
	i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 1275; j++) { /* Do nothing */ }
	ADC0CN = 0x8C;
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
            health--;  // Lose a life

            delay(500);

			if (health > 0) {
                // Reset the ball position or handle life loss without stopping the game
                ball_y = starting_position_y;  // Reset to starting position
                ball_x = starting_position_x;
                resetPaddlePosition();
				drawPaddle();
				x_vel = 0;
				y_vel = 1;  // Start the ball moving down again
				//row = 5;
			    //col = 60;
				
            } else {
                // End the game if no lives left
                gameOver();  // You would need to implement this function
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
		pregame(); 

        // Optionally, include a small delay to debounce the input or reduce CPU usage
        //delay(10);  // Delay for 10 milliseconds
		refresh_screen();
    }
	delay(250);
	
    // The button has been pressed, proceed with game start
    blank_screen();  // Clear the screen initially

    while (1) {
		
		delay(150);
        blank_screen();		//gtg
		drawBorders();		//gtg
		drawInfoPanel(); 
		parameter_check();	//gtg
		updateBall();
        drawBricks();		//partial compliance
        drawPaddle();
		drawBall(ball_x,ball_y);	//change arguments to variables
		

        refresh_screen();
    }
    //return 0;
}
