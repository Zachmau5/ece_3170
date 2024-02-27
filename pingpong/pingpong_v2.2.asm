;Project: Project 1, Ping Pong
;Team: Andrew Coffel and Zachary Hallett
;Description:
;       Use two buttons and a 10 LED bar to simulate playing ping pong
;       players will use the buttons as paddels and try to return the ball to
;       the other side.
;MAIN POINTS:
;	Left Button -> 	Player 1
;	Right Button -> Player 2
; DIP 1 -> 				Subtract Player 1 window by 1 i.e POS 10 and 9
;	DIP 2 -> 				Subtract Player 1 window by 2 i.e POS 10
; DIP 3 ->				Subtract Player 2 window by 1 i.e POS 1 and 2
;	DIP 4 ->				Subtract Player 2 window by 2 i.e POS 1
; DIP 8 ->				Speed Toggle, if set, goes to faster speed
;
;VER		DATE				Description
;
; 1.0 -	2/05/2024 	-	initial project based off of tug of war
; 1.1 - 2/07/2024 	-	added serving function
; 1.2 - 2/10/2024 	- chased bugs for getting general functioning to work
; 1.3 - 2/11/2024		- Added p1 and p2 windows
; 1.4 - 2/11/2024		- Added speed control on DIPs 5-8
; 1.5 - 2/17/2024		- Removed multiple speed controls and only have fast and default speeds
; 1.6 - 2/17/2024		- Added Reset logic so serve alternates every game
; 2.0 - 2/17/2024		- Current implementation of the game, all major functions work
; 2.1 - 2/26/2024		- Final Version and Submittion. Added descriptions


$include (c8051f020.inc)

dseg at 30h
    old_buttons: 	ds 1
    position: 		ds 1
    direction: 		ds 1
    status: 			ds 1

cseg
    mov wdtcn, #0DEh     					; disable watchdog
    mov wdtcn, #0ADh							; disable watchdog
    mov xbr2, #40h       					; enable port output
    mov A, R3
    cpl A
    mov R3, A
    acall start_pos     					; Initializes the position to the center LEDs
    mov status, #0								; 
    mov old_buttons, #00h   			; Sets the saved data of the buttons to zero
    mov direction, #1       			; Sets the direction to the right (R=0, L=1)

begin:
    acall delay        						; Delay to prevent mechanical bouncing
    acall check_buttons     			; Check if either button or both are pressed
    acall disp_led      					; Check location of the first LED and display it
    mov A, status
    cjne A, #0, loop     					; Leave start loop if left button is pressed
    jmp begin

loop:
    mov P3, #0FFh      						; Clear previous LEDs
    orl P2, #03h									; Clear LEDs on different port than the other 8
    acall check_buttons     			; Check if either button or both are pressed
    acall move_ball       				; Call the subroutine to move the ball
    jmp loop

move_ball:
    mov A, direction       				; Pull the direction of move_ball into the accumulator
    jnz move_ball_left     				; If the accumulator is zero move_ball right
    dec position					 				; Move to the right
    acall win_state_check	 				; Check position and see if 10 < LED <1 
    acall disp_led     		 				; Check location of the first LED and display it
    acall move_delay_def   				; Check if DIP 8 is toggled for speed change
    ret


move_ball_left:
    inc position       						; Move position left
    acall win_state_check					; Check position and see if 10 < LED <1 
    acall disp_led   							; Check location of the first LED and display it
    acall move_delay_def  				; Check switches for move speed
    ret

;------DISPLAY LED SUBROUTINE--------
; DISP_LED controls which LED is turned on based on the accumulator's value (A),
; which is set from the 'random' variable. It maps specific values of A to corresponding
; LEDs across ports P2 and P3, turning on one LED at a time while turning others off.
disp_led:
    mov A, position    						; Displaying LEDs based on position.
    cjne A, #1, check_led2
    clr P2.0
check_led2:
    cjne A, #2, check_led3
    clr P2.1
    ret
check_led3:
    cjne A, #3, check_led4
    clr P3.0
    ret
check_led4:
    cjne A, #4, check_led5
    clr P3.1
    ret
check_led5:
    cjne A, #5, check_led6
    clr P3.2
    ret
check_led6:
    cjne A, #6, check_led7
    clr P3.3
    ret
check_led7:
    cjne A, #7, check_led8
    clr P3.4
    ret
check_led8:
    cjne A, #8, check_led9
    clr P3.5
    ret
check_led9:
    cjne A, #9, check_led10
    clr P3.6
    ret
check_led10:
    cjne A, #10, disp_end
    clr P3.7
    ret
disp_end:
    ret


; -------------Time Delay = 10 ms------------
; DELAY - Creates a fixed delay using nested loops, used for debouncing the button inputs.
; Clock is 11.0592 MHz. Clock/12 = machine cycle. 1/machine cycle = machine period approx 1.085 usec.
delay:
    mov R1, #67      							; 1 machine cycle x 1.085 usec
outer_loop:
    mov R2, #100									; 1 machine cycle x 1.085 usec
inner_loop:
    djnz R2, inner_loop 					; 2 machine cyle x 100 = 1.085 usec =
    djnz R1, outer_loop						; 2 machine cycle x 100 x 67 x 1.085 usec = 
    ret

move_delay:
delay_loop_1:
    acall check_buttons   				; Sampling of buttons
    acall delay      							; 10ms delay
    djnz R4, delay_loop_1   			; Multiple of 10ms given ball speed
    ret

check_buttons:
    mov A, P2      			
    cpl A													; make buttons active high
    xch A, old_buttons						; save new button state, retrieve old
    xrl A, old_buttons						; new xor old, so 1s= changed buttons
    anl A, old_buttons						; mask off released buttons
    jb ACC.6, check_p2_window     ; Check Window Right
    jb ACC.7, check_p1_window     ; Check Window Left
no_switch:
    ret

; ------------- Ball Speed declarations------------
move_delay_def:
		jnb P1.7, i_wanna_go_fast
    mov R4, #50 ; Slower Speed
		sjmp apply_delay

i_wanna_go_fast:
		mov R4, #10
		sjmp apply_delay  

apply_delay:
    acall move_delay     					; Call the generic delay function with R4 set
    ret


check_p2_window:
    mov A, position
    cjne A, #3, p2_window_sub1    ; If position is not at 3, jump to medium window
    jnb P1.2, no_switch    				; Are we in easy?
    jmp swap_direction_left
    ret

p2_window_sub1:
    mov A, position
    cjne A, #2, p2_window_sub2   ; If position is not at 2, then move on
    jnb P1.3, no_switch   			 ; Are we in medium?
    jmp swap_direction_left
    ret

p2_window_sub2:
    mov A, position
    cjne A, #1, no_switch  			 ; If position is not at 1, then move on
    jmp swap_direction_left
    ret

check_p1_window:
    mov A, position
    cjne A, #8, p1_window_sub1   ; If position is not at 8, then move on
    jnb P1.0, no_switch    			 ; Are we in easy?
    jmp swap_direction_right
    ret

p1_window_sub1:
    mov A, position
    cjne A, #9, p1_window_sub2   ; If position is not at 9, then move on
    jnb P1.1, no_switch    			 ; Are we in medium?
    jmp swap_direction_right
    ret

p1_window_sub2:
    mov A, position      ; If position is not at 10, then move on
    cjne A, #0Ah, no_switch
    jmp swap_direction_right
    ret

swap_direction_left:
    mov direction, #1    ; Swap direction variable
    mov status, #1   ; Starting move direction
    ret

swap_direction_right:
    mov direction, #0    ; Swap direction variable
    mov status, #1   ; Start move direction
    ret

start_pos:
    cjne R3, #00h, p2_start
    mov position, #0Ah
    ret
p2_start:
    mov position, #01h
    ret

win_state_check:
    mov R6, position    ; Has either side won?
    cjne R6, #00h, p1_winning
    mov position, #1
    jmp win
p1_winning:
    cjne R6, #0Bh, mid_game
    mov position, #10
    jmp win
mid_game:
    ret

win:
    acall disp_led
    jmp win
end
