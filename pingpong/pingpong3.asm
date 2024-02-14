$include (c8051f020.inc)

; Data Segment for variables
dseg at 20h
pos:    DS 1                ; Position of the ball
dir:    DS 1                ; Direction (0 left, 1 right)
old_button: DS 1           ; Previous state of the buttons
game_over: DS 1            ; Game over flag
led_mask_high: DS 1        ; High byte of the LED mask (for P2)
led_mask_low: DS 1         ; Low byte of the LED mask (for P3)

; Code Segment
cseg

; Initialization
org 0h
START:
    mov SP, #7Fh            ; Initialize stack pointer
    mov wdtcn, #0DEh        ; Disable watchdog timer
    mov wdtcn, #0ADh
    mov xbr2, #40h          ; Enable crossbar and weak pull-ups
    mov p2mdout, #0FCh      ; Set P2.0 and P2.1 as push-pull output for LEDs 1 and 2
    mov p3mdout, #0FFh      ; Set P3 as push-pull output for LEDs 3 to 10
    mov pos, #5             ; Start position for the ball
    mov dir, #1             ; Initial direction to the right
    mov game_over, #0       ; Game is not over
    ; Wait for serve to start the game
    LCALL Wait_for_serve

; Main Game Loop
MainLoop:
    LCALL Check_buttons     ; Check button states to react to player actions
    LCALL Move_Ball         ; Move the ball based on direction
    LCALL Display           ; Update the LED display based on the ball's position
    LCALL Delay             ; Delay based on game speed
    SJMP MainLoop


; Check Buttons Subroutine
Check_buttons:
    MOV A, P2
    CPL A                   ; Complement inputs since active low.
    XCH A, old_button       ; Swap the new and old button states.
    XRL A, old_button       ; XOR to find changed states.
    ANL A, old_button       ; AND to filter out only pressed states.
    ANL A, #0C0h            ; Isolate the state of the two buttons.
    MOV B, A                ; Move the result to B for further action.
    RET

; Move Ball Subroutine with Game Over Check
Move_Ball:
    MOV A, dir
    CJNE A, #0, Move_Right
    ; Move Left
    DEC pos
    CJNE pos, #0FFh, Update_Display  ; Check for underflow indicating a move past the leftmost LED.
    ; Game Over Condition for Player 2 Win
    MOV game_over, #1
    LCALL Game_Over_Display
    SJMP End_Game
Move_Right:
    INC pos
    CJNE pos, #10, Update_Display    ; Check for overflow indicating a move past the rightmost LED.
    ; Game Over Condition for Player 1 Win
    MOV game_over, #1
    LCALL Game_Over_Display
    SJMP End_Game

Update_Display:
    LCALL Display
    RET

; Game Over Display Subroutine
Game_Over_Display:
    ; Example: Light up all LEDs briefly to indicate game over
    MOV P2, #0FC   ; Turn on LEDs 1 and 2 on P2
    MOV P3, #00    ; Turn on LEDs 3 to 10 on P3
    ; Delay to keep LEDs on for a moment
    MOV R2, #20
Delay_Loop_Game_Over:
    MOV R3, #255
Delay_Inner_Loop_Game_Over:
    DJNZ R3, Delay_Inner_Loop_Game_Over
    DJNZ R2, Delay_Loop_Game_Over
    ; Turn off all LEDs after delay
    MOV P2, #0FF   ; Turn off LEDs 1 and 2 on P2
    MOV P3, #0FF   ; Turn off LEDs 3 to 10 on P3
    RET

End_Game:
    SJMP End_Game  ; Infinite loop to stop game play after game over

    
    
; Adjusted Display Subroutine with 16-bit Mask Logic
Display:
    MOV A, #0FFh            ; Default mask with all LEDs off
    MOV led_mask_high, A    ; High byte for P2
    MOV led_mask_low, A     ; Low byte for P3
    MOV A, pos
    ; Calculate the mask based on pos
    ; P2.0 and P2.1 are controlled by led_mask_high (positions 1 and 2 respectively)
    ; LEDs 3 to 10 on P3 are controlled by led_mask_low (positions 3 to 10 respectively)
    CJNE A, #0, NOT_POS1    ; Skip if pos is not 1
    ANL led_mask_high, #0FEh; Clear bit for LED 1
    SJMP APPLY_MASK
NOT_POS1:
    CJNE A, #1, NOT_POS2    ; Skip if pos is not 2
    ANL led_mask_high, #0FDh; Clear bit for LED 2
    SJMP APPLY_MASK
NOT_POS2:
    ; For pos > 2, adjust for LEDs on P3
    SUBB A, #2              ; Adjust pos to start from 0 for P3
    MOV B, #1               ; Start with bit 0 set
SHIFT_MASK:
    DEC A
    JZ DONE_SHIFT           ; Done shifting
    RL B                    ; Rotate left through carry
    SJMP SHIFT_MASK
DONE_SHIFT:
    MOV led_mask_low, B     ; Apply shifted mask for LEDs on P3
APPLY_MASK:
    ; Apply the mask to the ports
    MOV A, led_mask_high
    MOV P2, A
    MOV A, led_mask_low
    MOV P3, A
    RET

END
