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
mov Position, #04h	        ; Starting position (middle LEDs)

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
    CJNE A, #00H, NINE      ; Check if position is at the left extreme
    SJMP OVER               ; End the game
NINE:
    MOV A, Position
    CJNE A, #08H, MainLoop  ; Check if position is at the right extreme
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
    ORL P3, #0FFh
    ORL P2, #03h
    MOV A, Position
    LCALL DISP_LED
    INC A
    LCALL DISP_LED
    RET

; ------------- Display LED's ----------------
DISP_LED:
    ; ... [include your LED control logic here, similar to the given DISP_LED routine] ...
    ; This routine will turn on the LED corresponding to the value in the accumulator
    ; ...

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
