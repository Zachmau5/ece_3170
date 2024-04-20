#include <C8051F020.h>
#include "lcd.h"
#include <stdbool.h>


//----------------VARIABLES-------------------------------------------------------------------------------------------//
/*
Project:	Project 2
Team:		Andrew Choffel and Zachary Hallett
Description:


Version 	Date			Description
0.1			4/2/2024		Initial Submit
0.2			4/16/2024		Create Border
0.3			4/16/2024		Paddle Drawn
0.4			4/20/2024		Ball Moving
0.5			4/20/2024		Ball Moves and bounces
1.0			4/20/2024		Ball stops moving when not hit by paddle	

*/

// Variables for screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BRICK_ROWS 3
#define BRICK_COLS 10
#define INFO_START_COL 106
#define INFO_WIDTH 27  // You have columns 101 to 128, inclusive
#define LINE_HEIGHT 1  // Assuming each line of text uses 8 rows (adjust as per your font)


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
unsigned int paddle_len = 16;
unsigned int two_player_mode = 0;
void drawString(const char* str, int col, int row);








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
unsigned int player_score[2] = {0}; // Scores for player 1 and player 2

// Variables for fast or slow ball speed
unsigned int speed_toggle = 0;    // Speed of the ball

unsigned int paddle_pos = 0;   // Initial position of the paddle

//Ball Variables
int ball_x = SCREEN_WIDTH / 6; // Initial x position of the ball
int ball_y = SCREEN_HEIGHT -20; // Initial y position of the ball
int x_vel = 1; // Ball movement along x-axis (1 for right, -1 for left)
int y_vel = 1; // Ball movement along y-axis (1 for down, -1 for up)
int health = 3;




// Functions declarations
void initDevice();
void init_adc();
void init_timer();
void disp_char(unsigned char row, unsigned char column, char character);
void drawBorders();
void updateBall();
void drawPaddle();
unsigned char drawBall(int x, int y);
void blank_screen();
void refresh_screen();



xdata char bricks [4][16] = {
{0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,0x70,0x70,0x70,0x70,0x70,0x70},
{0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,0x70,0x70,0x70,0x70,0x70,0x70},
{0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,0x70,0x70,0x70,0x70,0x70,0x70},
{0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,0x70,0x70,0x70,0x70,0x70,0x70}};

sbit dip1 = P1^0;
sbit dip2 = P1^1;
sbit dip3 = P1^2;
sbit dip4 = P1^3;
sbit dip5 = P1^4;
sbit dip6 = P1^5;
sbit dip7 = P1^6;
sbit dip8 = P1^7;
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
	// The formula is: ReloadValue = 65536 - (Sysclk / 12 / DesiredFrequency)
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
	dataOut = (dataOut * ((83L - paddle_len)+1)) >> 12;
	//averaged = 0;
	//dataOut= (int)(5+(96-paddle_len-4)*dataOut)/4096;

	pot_sum += dataOut;
	count ++;
	 
	if(count % 3 == 0)
	{
		//take the average.
		pot_avg = pot_sum/2 ;
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
    for (row = 0; row<8; row++ ) {	
		screen[128*row]=0xFF;
    }
	for (row = 0; row<8; row++ ) {	
		screen[128*row+100]=0xFF;
    }

}
void drawBricks() {
	int i,j,k=0; //i=row, j=col, k=width
	for (i=0; i<4; ++i) {
		for (j=0; j<16; ++j) {
			for(k=0; k<5; ++k) {
				screen[128*(i+1) + (6*j) + k + 3] = bricks[i][j];
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


void drawInfoPanel(unsigned int p1_score,unsigned int p2_score,unsigned int high_score) {
    unsigned int row = 0;
	//int i;
    int col = INFO_START_COL; // Assume this is defined as the start column for the info panel
	
    // Display "P1" and P1's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '1');
	disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    // Assuming scores are always one or two digits for simplicity
    disp_char(row, col, '0' + (p1_score / 10) % 10); // Tens
    disp_char(row, col + 6, '0' + p1_score % 10); // Units
    row += LINE_HEIGHT;

    // Display "P2" and P2's score
    disp_char(row, col, 'P');
    disp_char(row, col + 6, '2');
	disp_char(row, col + 12, ':');
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (p2_score / 10) % 10); // Tens
    disp_char(row, col + 6, '0' + p2_score % 10); // Units
    row += LINE_HEIGHT;

    // Display "High Score" and the actual high score
    disp_char(row, col, 'H'); // Simplified display of "High Score"
    disp_char(row, col+5, 'I'); // Simplified display of "High Score"
    disp_char(row, col+10, 'G'); // Simplified display of "High Score"
    disp_char(row, col+16, 'H'); // Simplified display of "High Score"
    row += LINE_HEIGHT;
    disp_char(row, col, '0' + (high_score / 10) % 10); // Tens
    disp_char(row, col + 6, '0' + high_score % 10); // Units
    row += LINE_HEIGHT;

    // Display "Health" and icons
    disp_char(row, col, 'H'); // Simplified display of "Health"
    disp_char(row, col+6, 'P'); // Simplified display of "Health"
    disp_char(row, col+12, ':'); // Simplified display of "Health"
    row += LINE_HEIGHT;

	disp_char(row, col, '#'); // Simplified display of "Health"
    disp_char(row, col+6, '#'); // Simplified display of "Health"
    disp_char(row, col+12, '#'); // Simplified display of "Health"
    row += LINE_HEIGHT;
    
}

/*
// Draw the paddle based on ADC values
void drawPaddle() {
   for (i = 0; i < ((6*128) + paddle_len); i++)	{
        screen[i+pot_avg] |= 0xC0;
   	}
}
*/

//Draw Ball
unsigned char drawBall(int x, int y)	{
    unsigned char row, col, shift, j;
    unsigned char hit = 0;
    int i;
	if (x <= 1 || x >= 100 || y<=5 || y>= 61) return 0;
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



void updateBall() {
    ball_x += x_vel;
    ball_y += y_vel;

    left_bound = 3;
    right_bound = 99;
    top_bound = 6;
    bottom_bound = SCREEN_HEIGHT - 3;

    // Wall collision detection
    if (ball_x <= left_bound) {
        x_vel = -x_vel;
        ball_x = left_bound;
    } else if (ball_x >= right_bound) {
        x_vel = -x_vel;
        ball_x = right_bound;
    }

    // Ceiling collision detection
    if (ball_y <= top_bound) {
        y_vel = -y_vel;
        ball_y = top_bound;
    }

    // Bottom collision detection
    if (ball_y >= bottom_bound) {
        if (!ballHitsPaddle()) {  // Check if it hits the paddle
            y_vel = 0;  // Stop the ball if it doesn't hit the paddle
            x_vel = 0;
            ball_y = bottom_bound;
        } else {
            y_vel = -y_vel;  // Continue the bounce if it hits the paddle
            ball_y = bottom_bound - 1;
        }
    }
}


// React ball-to-brick
void hitBrick() {

}

void delay(int ms) {
    int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 1275; j++) { /* Do nothing */ }
}

//Check win/lost and play sound
void checkWin() {

}

//---------------MAIN--------------------------------------------------------------------------------------------------//

int main() {
	initDevice();
	init_adc();
	init_timer();
	init_lcd();

    while (1) {
		//delay(50);
        blank_screen();		//gtg
		drawBorders();		//gtg
		drawInfoPanel(); 
		parameter_check();	//gtg
		updateBall();
        drawBricks();		//partial compliance
        drawPaddle();
		drawBall(ball_x,ball_y);	//change arguments to variables
		//idk
		
		delay(100);
        /*
		if (hitBorder_flag == 1 || hitBrick_flag == 1 ) {
			hitOther();
		} 
		else {
            hitPaddle();
            }

		checkWin();
        delay();
		*/
        refresh_screen();
    }
    //return 0;
}
