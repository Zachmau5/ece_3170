$include (C8051F020.inc)    ; Include the microcontroller-specific file

cseg at 0                   ; Code segment begins

; Initialization
mov wdtcn, #0DEH            ; Clear watchdog
mov wdtcn, #0ADH
mov xbr2, #40H              ; Enable Port Output
setb P2.6                   ; Declare P2.6 as input (left button)
setb P2.7                   ; Declare P2.7 as input (right button)

; Main Program Start
start:
    call initialize_state   ; Initialize the state
    call display            ; Display the initial LED state

main_loop:
    call delay_15ms         ; Call delay routine for debouncing
    call check_buttons      ; Check button states

    ; Check if both buttons are pressed
    mov A, old_buttons
    jb ACC.6, both_pressed  ; If left button is pressed, check right button
    jb ACC.7, both_pressed  ; If right button is pressed, check left button

    ; Only left button logic
    jnb ACC.6, check_right  ; Jump if left button not pressed
    cjne position, #1, move_left ; Check if not at left extreme
    jmp update_display

move_left:
    dec position            ; Move left
    jmp update_display

check_right:
    ; Only right button logic
    jnb ACC.7, update_display ; Jump if right button not pressed
    cjne position, #8, move_right ; Check if not at right extreme
    jmp update_display

move_right:
    inc position            ; Move right
    jmp update_display

both_pressed:
    ; Do nothing if both buttons are pressed
    jmp update_display

update_display:
    call display            ; Update LED display
    jmp main_loop           ; Repeat loop

; Initialization Routine
initialize_state:
    mov position, #5        ; Set position for the first of the center LEDs (LED5)
    ret

; Display Routine
display:
    orl P3, #0FFH           ; Turn off all LEDs on P3
    orl P2, #03H            ; Turn off LEDs on P2.0 and P2.1
    mov A, position
    call disp_led           ; Display LED for the current position
    inc A
    call disp_led           ; Display the next LED
    ret

; LED Control Logic
disp_led:
    ; Special handling for LEDs 1 and 2
    cjne A, #1, not_led1
    clr P2.0                ; Turn on LED1
    ret
not_led1:
    cjne A, #2, not_led2
    clr P2.1                ; Turn on LED2
    ret
not_led2:
    ; Handling for LEDs 3 to 10
    dec A                   ; Adjust position for zero-based index
    mov B, #80H             ; Load B with binary 10000000
    mov R4, A               ; Move adjusted position to R4 for shifting
    jz disp_finish          ; If position is 0 (LED3), skip shifting
shift_loop:
    rr B                    ; Rotate right through carry
    djnz R4, shift_loop     ; Repeat shift for the number of times in R4
disp_finish:
    orl P3, B               ; Set the corresponding bit in P3
    ret

; Button Check and Debounce Routine
check_buttons:
    mov A, P2
    cpl A                   ; Complement to make buttons active-high
    XCH A, old_buttons
    XRL A, old_buttons      ; XOR to find changed states
    ANL A, old_buttons      ; AND to filter out only pressed states
    mov old_buttons, A      ; Update old_buttons
    ret

; Delay Routine (approximately 15ms)
delay_15ms:
    mov R2, #50
loop1:
    mov R3, #200
loop2:
    djnz R3, loop2
    djnz R2, loop1
    ret

end                         ; End of the program
