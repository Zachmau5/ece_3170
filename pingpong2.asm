$include (c8051f020.inc)

; Data Segment for variables
dseg at 20h
pos:    DS 1                ; Position of the ball
dir:    DS 1                ; Direction (0 left, 1 right)
old_button: DS 1           ; Previous state of the buttons
game_over: DS 1            ; Game over flag

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

; Display Subroutine
Display:
    ORL P3, #0FFh            ; Turn off all LEDs on P3
    ORL P2, #03h             ; Turn off LEDs 1 and 2 on P2
    MOV A, pos
    CJNE A, #1, CHECK_LED2
    CLR P2.0                 ; Turn on LED 1 on P2
    SJMP Display_End
CHECK_LED2:
    CJNE A, #2, CHECK_LED3_10
    CLR P2.1                 ; Turn on LED 2 on P2
    SJMP Display_End
CHECK_LED3_10:
    MOV A, #1                ; Reset A for shifting
    MOV B, pos
    SUBB B, #2               ; Adjust B for LEDs on P3
SHIFT_LED:
    DEC B
    JZ SET_LED               ; Jump if B is zero, indicating the correct LED position
    RLC A                    ; Rotate left through carry to shift the bit
    SJMP SHIFT_LED
SET_LED:
    MOV P3, A                ; Set the correct LED on P3
Display_End:
    RET


; Delay Subroutine
DELAY:
    MOV R2, #20             ; Outer loop for delay
DELAY_OUTER_LOOP:
    MOV R3, #255            ; Inner loop count for a longer delay
DELAY_INNER_LOOP:
    DJNZ R3, DELAY_INNER_LOOP
    DJNZ R2, DELAY_OUTER_LOOP
    RET

; Wait for serve to start the game
Wait_for_serve:
    MOV A, P2
    ANL A, #0C0h            ; Isolate button states
    JNZ Check_Serve         ; If either button is pressed, check who served
    SJMP Wait_for_serve     ; Continue waiting if no button is pressed

Check_Serve:
    CPL A                   ; Complement since buttons are active low
    JNB ACC.6, Player1_Serve ; If P2.6 (left button) is pressed, player 1 serves
    ; If this point is reached, player 2 (right button) is assumed to have served
    MOV dir, #0              ; Set direction for player 2 serve (left)
    MOV pos, #8              ; Set starting position towards player 2's side
    SJMP MainLoop

Player1_Serve:
    MOV dir, #1              ; Set direction for player 1 serve (right)
    MOV pos, #1              ; Set starting position towards player 1's side
    SJMP MainLoop


END
