public check_buttons;

$include (c8051f020.inc)

DSEG at 20h ; Data Segment starting at address 20h.
buttonState: DS 1

CSEG;
check_buttons: 
        MOV     A, P2            ; Move the value of P2 into the accumulator
        CPL     A                ; Optional: Invert if a pressed button reads as low
        MOV     buttonState, A  ; Store the processed value in buttonState
        RET                      ; Return to calling C function

END