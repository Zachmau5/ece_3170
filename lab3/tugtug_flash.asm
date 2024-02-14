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
mov Position, #5            ; Starting position for P3.5 (LED3)

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

; Modified Game_over logic
Game_over:
    MOV A, Position
    CJNE A, #09H, CHECK_RIGHT  ; Check if position is at the left extreme (LED9)
    MOV R1, #0                 ; Set R1 to 0 for flashing LEDs 1-5
    LCALL Flash_LEDs
    SJMP OVER                  ; End the game

CHECK_RIGHT:
    CJNE A, #01H, MainLoop     ; Check if position is at the right extreme (LED1)
    MOV R1, #1                 ; Set R1 to 1 for flashing LEDs 6-10
    LCALL Flash_LEDs
    SJMP OVER                  ; End the game

OVER:
    SJMP OVER                  ; Endless loop to signify game over

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

; -------------Time Delay = 20 ms------------
DELAY:
    MOV R2, #67             ; Outer loop count
DELAY_OUTER_LOOP:
    MOV R3, #200            ; Inner loop count
DELAY_INNER_LOOP:
    DJNZ R3, DELAY_INNER_LOOP
    DJNZ R2, DELAY_OUTER_LOOP
    RET

; New Subroutine to light up LEDs 1-5
Light_1_to_5:
    ORL P3, #0FFh              ; Turn off all LEDs on P3
    ANL P2, #0F8h              ; Turn on LEDs 1-5 on P2
    RET

; New Subroutine to light up LEDs 6-10
Light_6_to_10:
    ORL P2, #0FCh              ; Turn off all LEDs on P2
    ANL P3, #0F8h              ; Turn on LEDs 6-10 on P3
    RET

; Flash LEDs Subroutine
Flash_LEDs:
    MOV R4, #5                ; Number of flashes
FLASH_LOOP:
    MOV A, R1
    JZ FLASH_1_TO_5
    LCALL Light_6_to_10
    LJMP FLASH_DELAY
FLASH_1_TO_5:
    LCALL Light_1_to_5
FLASH_DELAY:
    ; Call delay subroutine for 1 second
    MOV R2, #50
DELAY_1_SEC:
    LCALL DELAY
    DJNZ R2, DELAY_1_SEC

    ; Turn off all LEDs
    ORL P2, #0FFh             ; Turn off all LEDs on P2
    ORL P3, #0FFh             ; Turn off all LEDs on P3
    
    ; Delay for 1 second (LEDs off)
    MOV R2, #50
DELAY_1_SEC_OFF:
    LCALL DELAY
    DJNZ R2, DELAY_1_SEC_OFF

    ; Repeat the flashing
    DJNZ R4, FLASH_LOOP
    RET

END
