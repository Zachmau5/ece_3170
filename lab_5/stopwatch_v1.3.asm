; Project: Lab 5, Stop Watch w/Serial Interface (stopwatch.asm)
; Description: The student should be able to write, debug and test a program that
; uses timer and the serial port interrupts without busy/wait loops.

; Authors: 
; Andrew Coffel: andrewchoffel@mail.weber.edu
; Zachary Hallett: zacharyhallett@mail.weber.edu

; Course: ECE 3710 Section #2
; VER       Date        Description
; 0.1       2/28/2024   Created Project

$include (c8051f020.inc)  ; Include the microcontroller-specific definitions

DSEG at 30h
old_button:     DS 1
count:          DS 1
running:        DS 1    ; Use running.0 to determine the running state
prescaler_count: DS 1  ; Add this line to define the prescaler counter

CSEG
org 0000h
    LJMP    main
org 0023h
; Serial Interface Interrupt
ser_int:
    JBC     RI, RX_INT
    JBC     TI, TX_INT
    RETI

org 002Bh
; Timer 2 Interrupt for Stopwatch Functionality
Timer2_int:
    CLR     TF2    ; Clear Timer 2 Overflow Flag
    LJMP    T2_INT

; Main Program Initialization
main:
    MOV     wdtcn, #0DEh    ; Disable the watchdog timer
    MOV     wdtcn, #0ADh
    MOV     xbr2, #40h      ; Enable crossbar for port output
    SETB    P2.7            ; Set P2.7 as input (reset button)
    SETB    P2.6            ; Set P2.6 as input (start/stop button)
		MOV			prescaler_count, #10

    ; Timer 2 Configuration for 10Hz updates
     mov     RCAP2H, #HIGH(-18432) ; High byte of reload value
    mov     RCAP2L, #LOW(-18432)  ; Low byte of reload value

    setb    TR2                   ; Start Timer 2
    mov     IE, #10100000b         ; Enable Timer 2 and Serial interrupts

    ; Initialization for Serial communication
    MOV     SCON0, #50H     ; Setup UART for 8-bit data, 1 stop bit
    MOV     TH1, #-6        ; Set baud rate for UART
    SETB    TR1             ; Start timer for UART
    MOV     old_button, #0FFh  ; Initialize the old_button state to not pressed

    ; Main Loop
here:   
    CALL    Check_buttons   ; Check the button states in the main loop
    SJMP    here

; RX Interrupt Service Routine
RX_INT: 
    CLR     RI                    ; Clear Receive Interrupt flag
    MOV     A, SBUF0               ; Move received byte into accumulator
    CJNE    A, #'R', NOT_R        ; Check if 'R' was received
    MOV     A, #01h     ; Load 1 into A
		MOV     running, A  ; Set 'running' to 1
               						; Set the stopwatch running
		SJMP    RX_END
NOT_R:
    CJNE    A, #'S', NOT_S        ; Check if 'S' was received
    MOV     A, #00h     					; Load 0 into A
		MOV     running, A  					; Set 'running' to 0
              										; Stop the stopwatch
    SJMP    RX_END
NOT_S:
    CJNE    A, #'C', RX_END       ; Check if 'C' was received
		MOV     A, #00h     ; Load 0 into A
		MOV     running, A  ; Set 'running' to 0
    ;CPL     running               ; Stop the stopwatch
    MOV     count, #0x00          ; Reset the stopwatch count
    CALL    DISP_LED              ; Update the display immediately
RX_END:
    RETI

; TX Interrupt Service Routine
TX_INT:
    CLR     TI                    ; Clear Transmit Interrupt flag
    RETI

; Timer 2 ISR for Stopwatch Increment and Button Handling
; Timer 2 Interrupt Service Routine for Stopwatch
T2_INT:
    CALL    Check_buttons      ; Check the state of the buttons

    ; Check if the stopwatch is running
    MOV     A, running         ; Load 'running' into A
    ;ORL     A, #0              ; Logical OR A with 0 (this operation affects the zero flag)
    JZ      NOT_RUNNING        ; If Z flag is set (A was 0), jump to NOT_RUNNING

    ; If we're here, the stopwatch is running
    ; Increment the BCD count for the stopwatch
    MOV     A, count
		DJNZ		prescaler_count, NOT_RUNNING
		MOV			prescaler_count, #10
    ADD     A, #1              ; Add 1 to the BCD count for tenths of seconds
    DA      A                  ; Adjust for BCD
    MOV     count, A
    ;CJNE    A, #0A0h, DISPLAY_UPDATE  ; Check if we need to increment seconds
    ;ANL     A, #0Fh            ; Reset tenths to 0, keep seconds unchanged
    ;MOV     count, A           ; Store back the updated count
    ;INC     count              ; Increment the second

DISPLAY_UPDATE:
    ; Update the display regardless of whether we've incremented the second
    CALL    DISP_LED

NOT_RUNNING:
    ; Continue here if the stopwatch is not running
    RETI

; Button Check Routine
Check_buttons:
    MOV     A, P2                  ; Load the current state of the buttons
    CPL     A                      ; Invert since buttons are active-low
    MOV     B, A                   ; Copy current state to B
    XRL     A, old_button          ; Find changed buttons
    ANL     A, B                   ; Find buttons that are pressed
    MOV     old_button, B          ; Update old_button state

    JNB     B.6, NOT_START_STOP    ; Check if start/stop button is pressed
    ; If execution reaches here, the start/stop button has been pressed
    MOV     A, running             ; Load 'running' into A
    CPL     A                      ; Complement A (toggle the running state)
    MOV     running, A             ; Store back to 'running'
NOT_START_STOP:
    JNB     B.7, NO_RESET          ; Check if reset button is pressed
    MOV     A, #01h                ; Load 0 into A, preparing to stop the stopwatch and reset count. when =1, just starts. KEEP FOR TROUBLESHOOTING
    MOV     running, A             ; Stop the stopwatch by setting 'running' to 0
    ;MOV     count, A               ; Reset the counter to 0
    CALL    DISP_LED               ; Update LED display immediately
NO_RESET:
    RET


;------DISPLAY LED SUBROUTINE--------
DISP_LED:
    MOV     A, count      ; Load the count into A
    ;MOV     B, A          ; Store original count in B for later
		SWAP 		A
		CPL 		A
    ; Extract and display tenths of seconds (lower nibble)
    ;ANL     A, #00h       ; Isolate tenths (lower nibble for P3.0 to P3.3)
    MOV     P3, A         ; Display tenths on P3.0 to P3.3 (assuming lower nibble controls these LEDs)

    ; Extract and display seconds (upper nibble)
    ;MOV     A, B          ; Retrieve original count
    ;ORL     A, #00h      ; Isolate seconds (upper nibble)
    ;SWAP    A             ; Swap to bring seconds into lower nibble
    ;ORL     P3, A         ; Combine with tenths already on P3 (assuming upper nibble controls P3.4 to P3.7)

    RET
END
