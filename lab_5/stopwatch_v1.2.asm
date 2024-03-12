$include (c8051f020.inc)  ; Include the microcontroller-specific definitions

DSEG at 30h
old_button:     DS 1
count:          DS 1
running:        DS 1    ; Use running.0 to determine the running state

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

    ; Timer 2 Configuration for 10Hz updates
    mov     RCAP2H, #HIGH(-18432) ; Timer 2 reload value for 10Hz, high byte
    mov     RCAP2L, #LOW(-18432)  ; Timer 2 reload value for 10Hz, low byte
    setb    TR2                   ; Start Timer 2
    mov     IE, #10100000         ; Enable Timer 2 and Serial interrupts

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
    MOV     A, SBUF               ; Move received byte into accumulator
    CJNE    A, #'R', NOT_R        ; Check if 'R' was received
    SETB    running               ; Set the stopwatch running
    SJMP    RX_END
NOT_R:
    CJNE    A, #'S', NOT_S        ; Check if 'S' was received
    CLR     running               ; Stop the stopwatch
    SJMP    RX_END
NOT_S:
    CJNE    A, #'C', RX_END       ; Check if 'C' was received
    CLR     running               ; Stop the stopwatch
    MOV     count, #0x00          ; Reset the stopwatch count
    CALL    DISP_LED              ; Update the display immediately
RX_END:
    RETI

; TX Interrupt Service Routine
TX_INT:
    CLR     TI                    ; Clear Transmit Interrupt flag
    RETI

; Timer 2 ISR for Stopwatch Increment and Button Handling
T2_INT:
    CALL    Check_buttons
    JNB     running, SKIP_INCREMENT   ; Skip increment if not running
    MOV     A, count
    ADD     A, #1                    ; Add 1 to BCD count
    DA      A                        ; Adjust for BCD format
    MOV     count, A
    CJNE    A, #0A0h, UPDATE_DISPLAY ; Check for a full second
    ANL     A, #0Fh                  ; Reset tenths
    INC     A                        ; Increment seconds
    MOV     count, A                 ; Update count
UPDATE_DISPLAY:
    CALL    DISP_LED                 ; Update the display
SKIP_INCREMENT:
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
    CPL     running                ; Toggle running state
NOT_START_STOP:
    JNB     B.7, NO_RESET          ; Check if reset button is pressed
    CLR     running                ; Stop the stopwatch
    MOV     count, #0x00           ; Reset the counter
    CALL    DISP_LED               ; Update LED display immediately
NO_RESET:
    RET

;------DISPLAY LED SUBROUTINE--------
DISP_LED: 
    MOV     A, count               ; Load the count into A
    ; Display the value on LEDs connected to P3
    MOV     P3, A                  ; Display the count directly on P3
    RET

END
