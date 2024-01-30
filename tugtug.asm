$include (c8051f020.inc) 

; Data Segment for variables
dseg at 20h
Position:  ds 1
old_button: ds 1

; Code Segment
cseg

; Initialization
mov wdtcn, #0DEh            ; disable watchdog
mov wdtcn, #0ADh
mov xbr2, #40h              ; enable port output
setb P2.7                   ; Input button (right)
setb P2.6                   ; Input button (left)
mov Position, #5          ; Starting position for P3.5 (LED3)

; Initialize and Display LEDs
LCALL Display

; Main Game Loop
MainLoop:
    LCALL DELAY             ; Delay for debouncing
    LCALL Check_buttons     ; Check button states
    ANL A, #11000000b       ; Mask to isolate button inputs
    CJNE A, #90h, LEFT      ; Check if both buttons are pressed
    SJMP MainLoop

LEFT:
    CJNE A, #80h, RIGHT     ; Check if only left button was pressed
    INC Position
    LCALL Display
    LJMP Game_over

RIGHT:
    CJNE A, #40h, NONE      ; Check if only right button was pressed
    DEC Position
    LCALL Display
    LJMP Game_over

NONE:
    SJMP MainLoop           ; Continue if no button was pressed

Game_over:
    MOV A, Position
    CJNE A, #09H, NINE      ; Check if position is at the left extreme
    SJMP OVER               ; End the game
NINE:
    MOV A, Position
    CJNE A, #01, MainLoop  ; Check if position is at the right extreme
    SJMP OVER               ; End the game

OVER:
    SJMP OVER               ; Endless loop to signify game over

; Subroutines
; ------------- Checks the Buttons ------------
Check_buttons:
    MOV A, P2
    CPL A                   ; Complement inputs since active low
    XCH A, old_button       ; Exchange new and old button states
    XRL A, old_button       ; XOR to find changed states
    ANL A, old_button       ; AND to filter out only pressed states
    RET

; ------------- Display ------------
Display:
    ORL P3, #0FFh           ; Turn off all LEDs on P3
    ORL P2, #03h            ; Turn off all LEDs on P2
    MOV A, Position
    LCALL DISP_LED
    INC A                   ; Increment to next position (LED4)
    LCALL DISP_LED
    RET

; ------------- Display LED's ----------------
DISP_LED:
    CJNE A, #1, not_led1
    CLR P2.0                ; Turn on LED1
    RET
not_led1:
    CJNE A, #2, not_led2
    CLR P2.1                ; Turn on LED2
    RET
not_led2:
    CJNE A, #3, not_led3
    CLR P3.0                ; Turn on LED3 (initially on)
    RET
not_led3:
    CJNE A, #4, not_led4
    CLR P3.1                ; Turn on LED4 (initially on)
    RET
not_led4:
    CJNE A, #5, not_led5
    CLR P3.2                ; Turn on LED5
    RET
not_led5:
    CJNE A, #6, not_led6
    CLR P3.3                ; Turn on LED6
    RET
not_led6:
    CJNE A, #7, not_led7
    CLR P3.4                ; Turn on LED7
    RET
not_led7:
    CJNE A, #8, not_led8
    CLR P3.5                ; Turn on LED8
    RET
not_led8:
    CJNE A, #9, not_led9
    CLR P3.6                ; Turn on LED9
    RET
not_led9:
    CJNE A, #10, DISP_END
    CLR P3.7                ; Turn on LED10
    RET
DISP_END:
    RET

; -------------Time Delay = 20 ms------------
DELAY:
    MOV R2, #67             ; Outer loop count
DELAY_OUTER_LOOP:
    MOV R3, #200            ; Inner loop count
DELAY_INNER_LOOP:
    DJNZ R3, DELAY_INNER_LOOP
    DJNZ R2, DELAY_OUTER_LOOP
    RET

END
