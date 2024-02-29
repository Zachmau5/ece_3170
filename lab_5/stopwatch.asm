;Project: Lab 5, Stop Watch w/Serial Interface(stopwatch.asm)
;Description:
;The student should be able to write, debug and test a program that
;uses timer and the serial port interrupts without busy/wait loops

;Authors: 
;			Andrew Coffel:		andrewchoffel@mail.weber.edu
;		  Zachary Hallett:	zacharyhallett@mail.weber.edu

;Course: ECE 3710 Section #2
; VER		Date		Description
; 0.1		2/28/2024	Created Project


dseg at 30h
;variables here

cseg
mov wdtcn, #0DEh            ; disable watchdog
mov wdtcn, #0ADh
mov xbr2, #40h              ; enable port output
setb P2.7                   ; Input button (right)
setb P2.6                   ; Input button (left)
jmp main
org 23H

ser_int:
	jbc RI, rx_int 			; rx char
	jbc TI, tx_int			; tx complete
	reti					; should never get here but Murphy's Law

org 2BH

T2_int:
	clr TF2
	;stuff here
	reti
	
rx_int:

tx_int:

mov RCAP2H, #HIGH(-18432)
mov RCAP2l, #LOW(-18432)
setb TR2	;could also use mov TCON. #4
mov IE, #10100000







check_buttons:
    mov A, P2      			
    cpl A									; make buttons active high
    xch A, old_buttons						; save new button state, retrieve old
    xrl A, old_buttons						; new xor old, so 1s= changed buttons
    anl A, old_buttons						; mask off released buttons
    jb ACC.6, check_p2_window     			; Check Window Right
    jb ACC.7, check_p1_window     			; Check Window Left


end
