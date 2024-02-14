; This program is designed for a C8051F020 microcontroller. It aims to detect button presses,
; control LEDs based on a random selection, and send predefined messages through serial communication.
; The program includes debouncing for reliable button press detection, and a simple scheme for
; cycling through messages and LEDs based on the button presses.


$include (c8051f020.inc)

; Constants and variables are defined at the beginning for easy management and adjustment.
; DEBOUNCE_THRESHOLD is used to fine-tune the debounce logic, ensuring accurate button press detection.
; The data segment holds runtime variables such as the old_button state for change detection,
; debounce_counter for implementing debouncing, and a position or random counter for selecting messages and LEDs.

DEBOUNCE_THRESHOLD EQU 3 ; Adjust this if/when necessary

dseg at 20h
Position:   ds 1
old_button: ds 1
debounce_counter: ds 1 ; For button debounce

; Code Segment
cseg

; Initialization
; The initialization section disables the watchdog timer to prevent unwanted resets during debugging or initial setup.
; It configures the microcontroller to use an external oscillator for accurate timing, crucial for serial communication and timing functions.
; The serial port is configured for 9600 baud, 8 data bits, no parity, and one stop bit (8N1) — a common configuration for UART communication.
; LEDs are initialized to an off state to ensure a known startup condition.
; Initial carriage return and line feed are sent over serial to indicate the program has started and is ready to operate.

ORG 0
mov wdtcn,#0DEh 	; disable watchdog
mov wdtcn,#0ADh
mov xbr2,#40h	    ; enable port output
mov xbr0,#04h	    ; enable uart 0
mov oscxcn,#67H	    ; turn on external crystal
mov tmod,#20H	    ; wait 1ms using T1 mode 2
mov th1,#256-167	; 2MHz clock, 167 counts = 1ms
setb tr1

wait1:
	jnb tf1,wait1
	clr tr1		    ; 1ms has elapsed, stop timer
	clr tf1
wait2:
	mov a,oscxcn	; now wait for crystal to stabilize
	jnb acc.7,wait2
	mov oscicn,#8	; engage! Now using 22.1184MHz

	mov scon0,#50H	; 8-bit, variable baud, receive enable
	mov th1,#-6	    ; 9600 baud
	setb tr1	    ; start baud clock

; Start of code memory for messages, 256 in decimal.
;placed away from all initial instructions
ORG 100H

MSG_TABLE:
    DB "It is certain", 0DH, 0AH, '$'
    DB "You may rely on it", 0DH, 0AH, '$'
    DB "Without a doubt", 0DH, 0AH, '$'
    DB "Yes", 0DH, 0AH, '$'
    DB "Most Likely", 0DH, 0AH, '$'
    DB "Reply hazy, try again", 0DH, 0AH, '$'
    DB "Concentrate and ask again", 0DH, 0AH, '$'
    DB "Very Doubtful", 0DH, 0AH, '$'
    DB "My reply is no", 0DH, 0AH, '$'

START:

    ; Initialize LEDs to be off
    MOV P1, #0x00 ; Assuming LEDs are connected to P1

    ; Initialize variables
    MOV random, #1
    MOV old_button, #0FFH ; Assume all buttons unpressed initially
    MOV debounce_counter, #0


cycle:
    CALL delay_10ms
    CALL Check_Buttons
    JNB RI0, cycle    ; wait for data to arrive on UART
    CLR RI0		    ; clear flag for next time
    MOV A, SBUF0	    ; get the incoming data...
    MOV SBUF0, A	    ; ...and send it back
    SJMP cycle


;------10ms DELAY SUBROUTINE--------
; delay_10ms provides 10ms pause in program execution.
; It uses nested loops with the outer loop (R2) and inner loop (R3) counters
; to achieve the delay, calibrated based on the microcontroller's clock speed.

delay_10ms:
    MOV R2, #67             ; Outer loop count
DELAY_OUTER_LOOP:
    MOV R3, #100            ; Inner loop count
DELAY_INNER_LOOP:
    DJNZ R3, DELAY_INNER_LOOP
    DJNZ R2, DELAY_OUTER_LOOP
    RET

;------BUTTON CHECK SUBROUTINE--------
; Check_buttons monitors the state of buttons connected to P2.
; It implements a debounce mechanism by using debounce_counter to ensure
; the button state is stable before recognizing a press. Once a stable press
; is detected and surpasses the DEBOUNCE_THRESHOLD, handle_button_press is called.
Check_buttons:
    MOV A, P2
    CPL A                   ; Complement inputs since active low
    MOV B, A                ; Store current state in B
    XRL A, old_button       ; XOR to find changed states
    MOV old_button, B       ; Update old_button state
    JZ NoAction             ; If no change, do nothing
    INC debounce_counter    ; Increment debounce counter for debouncing
    CJNE debounce_counter, #DEBOUNCE_THRESHOLD, NoAction ; Check if debounce count is met
    CLR debounce_counter    ; Reset debounce counter
    CALL handle_button_press ; Handle the button press after debouncing
NoAction:
    RET


;------BUTTON PRESS SUB--------
; handle_button_press sets DPTR to start of message from table when location at table
; is the same as 'random'
; serial_send is then calld to send the message, byte by byte until the terminator '$' is reached.

handle_button_press:
    MOV A, random            ; Get the current random value which indicates the message to send
    MOV R1, A                ; Copy random value to R1 for decrementing
    MOV DPTR, #MSG_TABLE     ; Point DPTR to the start of the message table

    ; Loop until we reach the correct message based on the random number
    DEC R1                   ; Decrement R1 first because DPTR is already at the first message
    MOV A, R1
    ORL A, #0FFH             ; Check if R1 has underflowed (A will be 0xFF if R1 was 0)
    JZ SEND_MSG              ; If R1 is 0, we are at the correct message, start sending it
    CLR A                    ; Clear A to ensure it's ready to load characters

NEXT_MSG:
    MOVC A, @A+DPTR          ; Move the character at DPTR into A
    INC DPTR                 ; Increment DPTR to point to the next character
    CJNE A, #’$’, NEXT_MSG   ; If not the end of a message, keep looking
    DJNZ R1, NEXT_MSG        ; Decrement R1 and if we haven't reached the 0, find the next message

;------SEND MESSAGE SUBROUTINE--------
; SEND_MSG calls the serial_send subroutine to transmit the currently
; selected message from the MSG_TABLE. It assumes DPTR is already
; pointing to the start of the desired message based on the 'random' variable's value.

SEND_MSG:
    CALL serial_send         ; DPTR now points to the start of the desired message
    RET

;------SERIAL SEND SUBROUTINE--------
; serial_send transmits characters from the current DPTR location, byte by byte,
; until it encounters the message terminator '$'. It uses the SBUF0 register for transmission
; and waits for the TI0 flag to indicate readiness for the next character.

serial_send:
    CLR A
NEXT_CHAR:
    MOVC A, @A+DPTR
    JZ EndSend               ; '$' as end of message indicator
    MOV SBUF0, A             ; Send character
WAIT_TX:
    JNB TI0, WAIT_TX         ; Wait for transmission to complete
    CLR TI0                  ; Clear transmit interrupt flag
    INC DPTR                 ; Move to next character
    SJMP NEXT_CHAR
EndSend:
    RET

;------DISPLAY LED SUBROUTINE--------
; DISP_LED controls which LED is turned on based on the accumulator's value (A),
; which is set from the 'random' variable. It maps specific values of A to corresponding
; LEDs across ports P2 and P3, turning on one LED at a time while turning others off.
Update_LED:
    MOV A, random
    CALL DISP_LED            ; Control LEDs based on 'random'
    CALL SEND_MSG            ; Send the message corresponding to 'random'
    RET

DISP_LED:
    ; Check if the LED to be controlled is within P2 range (LEDs 1-2)
    CJNE A, #1, CHECK_LED2
    CLR P2.0                ; Turn on LED1
    RET
CHECK_LED2:
    CJNE A, #2, CHECK_LED3
    CLR P2.1                ; Turn on LED2
    RET
CHECK_LED3:
    CJNE A, #3, CHECK_LED4
    CLR P3.0                ; Turn on LED3
    RET
CHECK_LED4:
    CJNE A, #4, CHECK_LED5
    CLR P3.1                ; Turn on LED4
    RET
CHECK_LED5:
    CJNE A, #5, CHECK_LED6
    CLR P3.2                ; Turn on LED5
    RET
CHECK_LED6:
    CJNE A, #6, CHECK_LED7
    CLR P3.3                ; Turn on LED6
    RET
CHECK_LED7:
    CJNE A, #7, CHECK_LED8
    CLR P3.4                ; Turn on LED7
    RET
CHECK_LED8:
    CJNE A, #8, CHECK_LED9
    CLR P3.5                ; Turn on LED8
    RET
CHECK_LED9:
    CJNE A, #9, CHECK_LED10
    CLR P3.6                ; Turn on LED9
    RET
CHECK_LED10:
    CJNE A, #10, DISP_END
    CLR P3.7                ; Turn on LED10
    RET
DISP_END:
    RET
