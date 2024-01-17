$include (C8051F020.inc)

        ORG 0000h ; origin

        MOV     SP, #7FH ; set stack pointer

        ; Initialize Ports
        MOV     P1MDOUT, #00h ; Set P1 as input for pushbuttons
        MOV     P2MDOUT, #00h ; Set P2 as input for DIP switches
        MOV     P3MDOUT, #0FFh ; Set P3 as output for LEDs

        ; Initialize Crossbar and Ports
        MOV     XBR2, #40h ; Enable Crossbar and weak pull-ups

        ; Disable Watchdog Timer
        MOV     WDTCN, #0DEh
        MOV     WDTCN, #0ADh

MainLoop:
        MOV     A, P1 ; Read pushbuttons
        ANL     A, #03h ; Mask for the first two bits
        CPL     A ; Invert for active-low LEDs
        MOV     B, A ; Store the state of pushbuttons
        MOV     A, P2 ; Read DIP switches
        MOV     P3, A ; Reflect DIP switches on LEDs
        ORL     P3, B ; Combine pushbutton state with DIP switches

        SJMP    MainLoop ; Repeat indefinitely

        END
