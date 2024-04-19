#include <C8051F020.h>
#include "lcd.h"

//----------------VARIABLES-------------------------------------------------------------------------------------------//

// Variables for screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 60
#define BRICK_ROWS 3
#define BRICK_COLS 10


#define BRICK_START_X 10
#define BRICK_START_Y 10
#define BRICK_WIDTH 5
#define BRICK_HEIGHT 3
#define BUFFER 1
#define ROWS 5
#define COLS 13

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
int i;
int j;
int x;
int y;
unsigned char row, col, shift, count;
// Variables to store game settings
unsigned int paddle_len = 8;
unsigned int two_player_mode = 0;

sbit dip1 = P1^0;
#define ROWS 5
#define COLS 13







cur_player=1;
// Declare a 2D array to store the presence (1) or absence (0) of bricks


// Variables for ball
code unsigned char ball[] = {0x0C, 0x1F, 0x1F, 0x1F, 0x0E};


// Variables for paddle position ADC
bit toggle = 0;             // Used as a toggle for ADC reading, to match the logic
unsigned int paddle_data = 0;   // Data read from the ADC for paddle position
unsigned int SampleCount = 0;   // Counter for ADC samples
unsigned paddle_div = 0;        // Divider for paddle position calculation

int hit_location = 0; //saves hit brick position

// Variables for 1 or 2 players
unsigned int player_count = 1;  // Number of players (1 or 2)
unsigned int player_score[2] = {0}; // Scores for player 1 and player 2

// Variables for fast or slow ball speed
unsigned int speed_toggle = 0;    // Speed of the ball

unsigned int paddle_pos = 0;   // Initial position of the paddle

//Ball Variables
int ball_x = SCREEN_WIDTH / 2; // Initial x position of the ball
int ball_y = SCREEN_HEIGHT / 2; // Initial y position of the ball
int x_vel = 1; // Ball movement along x-axis (1 for right, -1 for left)
int y_vel = 1; // Ball movement along y-axis (1 for down, -1 for up)



xdata char p1_bricks[13][5] = {
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1}
};

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
	RCAP2H = -843 >> 8; // High byte of auto-reload value. .
	RCAP2L = -843; // Low byte of auto-reload value.
	TR2=1;
	
	// Enable flash writes, set necessary flags
   // PSCTL = 0x01; // PSWE = 1, PSEE = 0
	
}


//---------------INTERRUPTS------------------------------------------------------------------------------------------//


void adc_int(void) interrupt 15 {
	AD0INT = 0; // Clear ADC interrupt flag
	dataOut = (ADC0H << 8) | ADC0L; // Read ADC data
	dataOut = (dataOut * ((88L - paddle_len)+1)) >> 12;
	//averaged = 0;
	//pot_val=55+(55+(dataOut*31L) / 1028);

	pot_sum += dataOut;
	count ++;
	 
	if(count % 7 == 0)
	{
		//take the average.
		pot_avg = pot_sum/6 ;
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
    P1=0xFF;
	
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

// Not sure
//void hitOther() {

//}

// Update ball position based on speed and direction
//void ballPos() {

//    ball_x += ball_dx * ball_speed;
  //  ball_y += ball_dy * ball_speed;
//}


// Draw borders
void drawBorders() {
	unsigned char column;
	unsigned char row;
    for (column = 0; column < 100; column++ ) {	
		disp_char(0, column, '%');
    }
	for (row = 0; row <64; row++) {
		disp_char(row, 0, '@');

	}	
	for (row = 0; row <64; row++) {
		disp_char(row, 100, '?');
	}

}
void draw_bricks()
{

    int i, j, k;
    for (i = 0; i < 13; i++)
    {
        for (j = 0; j < 5; j++)
        {
            // Adjusted index calculation to fit bricks of width 5 with 1 pixel gap
            int index = (j / 2) * 128 + i * 6 + 131;  // Now `i * 6` instead of `i * 7`

            int mask = (j % 2 == 0) ? 0x07 : 0x70;   // decide the mask based on even/odd row

            for (k = 0; k < 5; k++)  // Draw each brick 5 pixels wide
            {	
				if(cur_player == 1)
				{
	                if (p1_bricks[i][j] == 0)
	                {
	                    screen[index + k] &= ~mask;  // clear bits using negated mask
	                }
	                else if (p1_bricks[i][j] == 1)
	                {
	                    screen[index + k] |= mask;  // set bits using mask
	                }
				}
				else
				{
					if (p1_bricks[i][j] == 0)
	                {
	                    screen[index + k] &= ~mask;  // clear bits using negated mask
	                }
	                else if (p1_bricks[i][j] == 1)
	                {
	                    screen[index + k] |= mask;  // set bits using mask
	                }
				}
            }
        }
    }
}

// Draw the paddle based on ADC values
void drawPaddle() {
   for (i = 901; i < (901 + paddle_len); i++)	{
        screen[i+pot_avg] |= 0xC0;
   	}
}


// Draw ball
unsigned char drawBall(int x, int y) {
    unsigned char row, col, shift, j;
    unsigned char hit = 0;
    int i;

    // Handle boundary conditions and adjust velocity or end game if needed
    if (x <= 5 || x >= 101) {  // The ball has hit the left or right wall
        x_vel = -x_vel;
    } else if (y <= 5) {  // The ball has hit the top wall
        y_vel = -y_vel;
    } else if (y >= 61) {  // The ball has gone off the bottom
        //game_over();
        return 1;  // Returning after ending game, assuming no further processing needed
    }

    // Adjust the position to handle the top-left of the ball
    col = x - 2;
    row = y - 2;
    shift = row % 8;
    row = row / 8;

    // Loop to check and write the ball on the screen (considering page overflow)
    for (j = 0, i = row * 128 + col; j < 5; i++,j++)  {  //check me 
        unsigned char mask = (unsigned char)(ball[j] << shift);
        hit |= screen[i] & mask;
        screen[i] |= mask;
		hit_location =i;
		
        // Handling overflow onto the next page of the display
        if (mask & 0xFF00) {
            hit |= screen[i + 128] & (unsigned char)(mask >> 8);
			hit_location = i+128;
            screen[i + 128] |= (unsigned char)(mask >> 8);
        }
    }
    return hit;  // Return hit status indicating any collision detection
}


/*    
// Check for collision between ball and paddle/bricks
int checkCollision() {
	row(=0; row<6; ++row) {
		for(b=0; b<13; ++b) {
			if(brick[row*13+b]) {
				hit=0;
				j=30+b*7;
			i = (row/2 +1) *12 +j;
			mask = i&|? 0xe0:0x0e;
			for(col=j; col< j+6; ++ col, i++)	{
				hit |=screen[i] mask;
				screen[i]|=mask;
			}
			if (hit)
				//do stuff
			}
		}
	}

    // Returns 1 if there's a collision, 0 otherwise
}


// React ball-to-paddle
void hitPaddle() {
    int hitPosition = ball_x - paddle_pos;
    if (hitPosition < paddle_len / 2) {
        ball_dx = -1;
    }
    else {
        ball_dx = 1;
    }
}
*/
// React ball-to-brick
void hitBrick() {

}

// Delay routines for appropriate refresh rate
void delay() {
    // Code to introduce delay in milliseconds
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
	/*
	TODO TOP Level
	- refresh screen
	- draw borders
	- draw bricks
	- draw paddle
	- DO STUFF
		- main shit
		- did a ball hit a brick?
			y = elim brick and bounce
			n = border check
			
			border?
			y - bounce
			n - this means i hit the paddle
			
			
			
	- refresh screen
	*/
    while (1) {
		
        blank_screen();		//gtg
		drawBorders();		//gtg 
		parameter_check();	//gtg
        //drawBricks();		//partial compliance
        drawPaddle();		//idk
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
