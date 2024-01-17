$include (c8051f020.inc) ; Include the C8051F020 definitions

ORG 0000h ; Start of program

; Initialization
mov wdtcn, #0DEh ; disable watchdog
mov wdtcn, #0ADh
mov xbr2, #40h ; enable port output

; Configure ports
mov P1MDIN, #0xFF ; Set P1 as input
mov P2MDIN, #0xFF ; Set P2 as input
mov P3MDOUT, #0xFF ; Set P3 as output
mov P4MDOUT, #0xFF ; Set P4 as output

MAIN_LOOP:
; Read state of pushbuttons
mov A, P1 ; Load pushbutton states into accumulator
; Invert the bits since a pushbutton press is active low
cpl A
; Reflect state on the first two LEDs
mov P3, A ; Only P3.0 and P3.1 are affected

; Read state of DIP switches
mov A, P2 ; Load DIP switch states into accumulator
; Invert the bits since a DIP switch ON is active low
cpl A
; Reflect state on the eight LEDs
mov P4, A ; Reflect on P4.0 to P4.7

SJMP MAIN_LOOP ; Loop back to start

END ; End of program
