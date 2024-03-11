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
snapshot_tenth: DS 1

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
    SETB    P2.7            ; Set P2.7 as input (right button)
    SETB    P2.6            ; Set P2.6 as input (left button)

    ; Timer 2 Configuration for 10Hz updates
    mov     RCAP2H, #HIGH(-18432) ; Timer 2 reload value for 10Hz, high byte
    mov     RCAP2L, #LOW(-18432)  ; Timer 2 reload value for 10Hz, low byte
    setb    TR2                   ; Start Timer 2
    mov     IE, #10100000         ; Enable Timer 2 and Serial interrupts

    ; Initialization for Serial communication
    MOV     SCON0, #50H     ; Setup UART for 8-bit data, 1 stop bit
    MOV     TH1, #-6        ; Set baud rate for UART
    SETB    TR1             ; Start timer for UART

    ; Main Loop
here:   
    SJMP    here

; RX Interrupt Service Routine
RX_INT: 
    CLR     RI                    ; Clear Receive Interrupt flag
    MOV     A, SBUF               ; Move received byte into accumulator
    CJNE    A, #'R', NOT_R        ; Check if 'R' was received
    SETB    running               ; Set running state to true
    SJMP    RX_END
NOT_R:
    CJNE    A, #'S', NOT_S        ; Check if 'S' was received
    CLR     running               ; Set running state to false
    SJMP    RX_END
NOT_S:
    CJNE    A, #'C', NOT_C        ; Check if 'C' was received
    MOV     count, #0             ; Clear the counter
    SJMP    RX_END
NOT_C:
    CJNE    A, #'T', RX_END       ; Check if 'T' was received
    ; Prepare the snapshot of the count for sending over serial
    MOV     A, count
    ANL     A, #0Fh
    MOV     snapshot_tenth, A
    MOV     A, count
    SWAP    A
    ORL     A, #30h
    MOV     SBUF0, A              ; Send formatted count over serial
RX_END:
    RETI

; TX Interrupt Service Routine
TX_INT:
    CLR     TI                    ; Clear Transmit Interrupt flag
    RETI

; Timer 2 ISR for Stopwatch Increment and Button Handling
T2_INT:
    CALL    Check_buttons
    JNB     running, T2_SKIP      ; Skip count increment if not running
    INC     count                 ; Increment the count if in running state
    CJNE    A, #100, T2_SKIP      ; Check if count reaches 10 seconds
    MOV     count, #0             ; Reset the count
T2_SKIP:
    RETI

; Button Check Routine
Check_buttons:
    MOV     A, P2                 ; Get the current state of the buttons
    CPL     A                     ; Invert button states (make 1=pressed, 0=not pressed)
    XCH     A, old_button         ; Exchange A and old_button
    XRL     A, old_button         ; Determine changed buttons
    ANL     A, old_button         ; Determine pressed buttons
    RET

; Send Byte Routine for Serial Communication
send_byte:
    MOV     SBUF0, A              ; Load A into SBUF0 for transmission
    JNB     TI, $                 ; Wait for transmission to complete
    CLR     TI                    ; Clear transmission interrupt flag
    RET

END
