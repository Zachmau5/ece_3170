; File Header
; - Magic 8 Ball
; - Random number generator from 1-8.
; - Zachary Hallett - zacharyhallett@mail.weber.edu
;	- Andrew Coffel		- andrewchoffel@mail.weber.edu
;
; VERSION 1.0: Initial Submit - Issues with error fixing unresolved by deadline.
; VERSION 2.0: Changed implementation from a lookup table approach.

$include (c8051f020.inc) ; Include the microcontroller specific header.

DSEG at 20h ; Data Segment starting at address 20h.
random: DS 1 ; Variable to store the random number.

; Variable to track the state of the button from the previous check.
old_button:
        DS 1

CSEG ; Code Segment.
	message_1:  db      "It is certain", 0
	message_2:  db      "You may rely on it", 0
	message_3:  db      "Without a doubt", 0
	message_4:  db      "Yes", 0
	message_5:  db      "Most likely", 0
	message_6:  db      "Reply hazy, try again", 0
	message_7:  db      "Concentrate and ask again", 0
	message_8:  db      "Don't count on it", 0
	message_9:  db      "Very doubtful", 0
	message_10: db      "My reply is no", 0
	carriage_return:   db      0DH, 0AH, 0
; Initial setup of the microcontroller.
setup:
        MOV     wdtcn, #0DEh    ; Disable the watchdog timer.
        MOV     wdtcn, #0ADh
        MOV     xbr2, #40h      ; Enable crossbar for port output.
        MOV     xbr0, #04h      ; Enable UART 0.
        SETB    P2.6            ; Set P2.6 as input (button).
        SETB    P2.7            ; Set P2.7 as input (button).
        MOV     oscxcn, #67h    ; Activate external crystal oscillator.
        MOV     TMOD, #21H      ; Configure timers.
        MOV     th1, #256-167   ; Setup timer for serial communication baud rate.
        SETB    tr1             ; Start timer 1.

; Wait for the oscillator to stabilize.
wait_for_oscillator:
wait1:  JNB     tf1, wait1		;stop timer after 1 ms
        CLR     TR1
        CLR     TF1
wait2:  MOV     A, oscxcn
        JNB     ACC.7, wait2
        MOV     OSCICN, #8      ; Switch to using the 22.1184 MHz oscillator.
        MOV     SCON0, #50H     ; Setup UART for 8-bit data, 1 stop bit.
        MOV     TH1, #-6        ; Set baud rate for UART.
        SETB    TR1             ; Start timer for UART.

; Send a carriage return and line feed to the serial port.
send_carriage_return:
        MOV     DPTR, #carriage_return  ;find address of carriage_return
        LCALL   next_char				;sends carriage return and line feed 

; Button press handling routine.
handle_button_press:
        CALL    delay_10ms
        DJNZ    random, continue
        MOV     random,#6      ; Reset random number within range

continue:
        CALL    check_buttons
        CJNE    A, #01, NEXT		;makes sure button is actally pressed
NEXT:   JB      SCON0.0, message_call
        JC      handle_button_press

; Routine to call the message associated with the LED.
message_call:
        CLR     SCON0.0				;clears UART transmitt interupt flag
        CALL    DISP_LED
        CALL    next_char
        JMP     carriage_return

; Send characters to the UART. reads characters from the message pointed to by data pointer. fetches EACH
; character, character by character, as long as fetched character is not zero.
next_char:
        CLR     A 
        MOVC    A, @A+DPTR
        JZ      end_send
        CALL    send_byte
        INC     dptr
        JMP     next_char

end_send:   
        RET

; Send a byte over UART.
; sends character into uarts buffer reg and waits a small delay to ensure transmission.
send_byte:
        MOV     SBUF0, A
        CALL    delay_5ms
        RET

; Delay routines for serial communication timing.
serial_delay:
        JNB     tf1, serial_delay
        CLR     tf1
        RET

delay_10ms:        
        MOV     TL0, #low(-9216)
        MOV     TH0, #high(-9216)
        SETB    TR0
WAIT:   JNB     TF0, WAIT
        CLR     TF0
        CLR     TR0
        RET

delay_5ms:        
        MOV     TL0, #low(-4044)
        MOV     TH0, #high(-4044)
        SETB    TR0
WAIT_5: JNB     TF0, WAIT_5
        CLR     TF0
        CLR     TR0
        RET

; Check button press status.
check_buttons: 
        MOV     A, P2
        CPL     A
        XCH     A, old_button
        XRL     A, old_button
        ANL     A, old_button
        ANL     A, #11000000b
        RET
		
;------DISPLAY LED SUBROUTINE--------
; DISP_LED controls which LED is turned on based on the accumulator's value (A),
; which is set from the 'random' variable. It maps specific values of A to corresponding
; LEDs across ports P2 and P3, turning on one LED at a time while turning others off.
; ADDED MESSAGE LINK TO EACH LED FROM PREVIOUS LABS
DISP_LED: 
        MOV     A, random
        MOV     P3, #0FFh
        ORL     P2, #03
        CJNE    A, #01, CHECK_LED2   ; Compares accumulator with 0, if true it turns on the last light and ends the game.
        mov     dptr, #message_1
        CLR     P3.0
        RET
CHECK_LED2: 
        CJNE    A, #02, CHECK_LED3  ; Compares accumulator with 1, if true it turns on the LED, if not it jumps to next bit if the accumulator bit is not 1.
        mov     dptr, #message_2
        CLR     P3.1
        RET
CHECK_LED3: 
        CJNE    A, #03, CHECK_LED4
        mov     dptr, #message_3
        CLR     P3.2
        RET
CHECK_LED4: 
        CJNE    A, #04, CHECK_LED5
        mov     dptr, #message_4
        CLR     P3.3
        RET
CHECK_LED5: 
        CJNE    A, #05, CHECK_LED6
        mov     dptr, #message_5
        CLR     P3.4
        RET
CHECK_LED6: 
        CJNE    A, #06, CHECK_LED7
        mov     dptr, #message_6
        CLR     P3.5
        RET
CHECK_LED7: 
        CJNE    A, #07, CHECK_LED8
        mov     dptr, #message_7
        CLR     P3.6
        RET
CHECK_LED8: 
        CJNE    A, #08, CHECK_LED9
        mov     dptr, #message_8
        CLR     P3.7
        RET
CHECK_LED9: 
        CJNE    A, #09, not_nine
        mov     dptr, #message_9 
        CLR     P2.0
        RET
not_nine: 
        CJNE    A, #10, CHECK_LED10
        mov     dptr, #message_10
        CLR     P2.1
        RET
CHECK_LED10: 
        MOV     dptr, #carriage_return			;clear file register
        RET

END
