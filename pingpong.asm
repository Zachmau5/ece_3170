;Header File Crap
;Project: Ping Pong
;Team: Andrew Coffel and Zachary Hallett
;Description:
;       Use two buttons and a 10 LED bar to simulate playing ping pong
;       players will use the buttons as paddels and try to return the ball to
;       the other side.
; VERSION 1.0 - initialized project based of tug of war

$include (c8051f020.inc)

; Data Segment for variables
dseg at 20h
Position:  ds 1           ; Position of the ball
Direction: ds 1           ; Direction of the ball, 1 for right (initially)
old_button: ds 1          ; Previous state of the buttons

; Code Segment
cseg

; Initialization
org 0h
START:
    mov SP, #7Fh           ; Initialize stack pointer
    mov wdtcn, #0DEh       ; Disable watchdog timer
    mov wdtcn, #0ADh
    mov xbr2, #40h         ; Enable crossbar and weak pull-ups
    mov p2mdout, #0FCh     ; Set P2.0 and P2.1 as push-pull output for LEDs 1 and 2
    mov p3mdout, #0FFh     ; Set P3 as push-pull output for LEDs 3 to 10
    mov Position, #5       ; Start with the first LED for scrolling effect
    mov Direction, #1      ; Direction is not used in this simple scroll but kept for possible future use

; Main Game Loop
MainLoop:
    LCALL Check_buttons    ; Check button states to start scrolling
    LCALL Move_Ball        ; Move the ball based on button press
    LCALL Display          ; Update the LED display based on the ball's position
    LCALL DELAY            ; Delay for debouncing and speed control
    SJMP MainLoop

; Check Buttons Subroutine
Check_buttons:
    MOV A, P2
    ANL A, #0C0h           ; Isolate the button states (P2.7 and P2.6)
    MOV B, A
    CPL B                  ; Complement since buttons are active low
    MOV A, Position
    CJNE A, #2, Check_Right_Button
    JNB B.6, Left_Button   ; If left button (P2.6) is pressed in buffer zone
Check_Right_Button:
    CJNE A, #9, No_Button_Pressed
    JNB B.7, Right_Button  ; If right button (P2.7) is pressed in buffer zone
    SJMP No_Button_Pressed
Left_Button:
    ; If the ball is in the left buffer zone, reverse direction
    CJNE Position, #2, RET
    CLR Direction          ; Change direction to left
    SJMP No_Button_Pressed
Right_Button:
    ; If the ball is in the right buffer zone, reverse direction
    CJNE Position, #9, RET
    SETB Direction         ; Change direction to right
No_Button_Pressed:
    RET

; Move Ball Subroutine (Adjusted for win condition beyond visible LEDs)
Move_Ball:
    MOV A, Direction
    CJNE A, #0, Move_Right
    DEC Position
    CJNE Position, #0FFh, Update_Display  ; Allows decrementing past 1 for game over condition
    SJMP Game_Over_Right_Wins
    RET
Move_Right:
    INC Position
    CJNE Position, #11, Update_Display    ; Allows incrementing past 10 for game over condition
    SJMP Game_Over_Left_Wins
    RET
Update_Display:
    LCALL Display
    RET

; Display Subroutine
Display:
    MOV A, Position
    MOV B, #1
    MOV C, A
    CJNE A, #1, CHECK_LED2
    MOV P2, #0FDh           ; Turn on LED 1 on P2
    SJMP Display_End
CHECK_LED2:
    CJNE A, #2, CHECK_LED3_10
    MOV P2, #0FBh           ; Turn on LED 2 on P2
    SJMP Display_End
CHECK_LED3_10:
    SUBB A, #2              ; Adjust position for LEDs on P3
    MOV P3, B
    RLC C
    MOV P3, C               ; Update LEDs on P3 based on position
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

; Game Over Subroutines
Game_Over_Left_Wins:
    ; Blink or set a specific pattern to indicate left side wins
    ; Example: Light up all LEDs
    MOV P2, #0FC ; Assuming LEDs 1 and 2 are on P2 and need to be turned off
    MOV P3, #00  ; Turn on all LEDs on P3 to indicate win
    SJMP $       ; Stay in this state indefinitely

Game_Over_Right_Wins:
    ; Blink or set a specific pattern to indicate right side wins
    ; Example: Turn off all LEDs
    MOV P2, #0FF ; Turn off LEDs 1 and 2 on P2
    MOV P3, #0FF ; Turn off all LEDs on P3 to indicate win
    SJMP $       ; Stay in this state indefinitely

End_Game:
    SJMP End_Game           ; Loop indefinitely once the game is over

END START
