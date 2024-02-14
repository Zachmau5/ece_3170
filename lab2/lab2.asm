$include(C8051F020.inc)

cseg at 0
mov wdtcn, #0DEH					;Clear watchdog
mov wdtcn, #0ADH	
mov xbr2, #40H						;Enable Port Output

mov A, #0FFH							;Filling Accum w/1's
mov P1, A									

setb P2.6   							;declare as Input
setb P2.7									;declare as Input 


loop:		mov P3, P1				;DIP -> LED 3-10

				mov C, P2.6				;Button 1 -> Carry Flag
				mov P2.0, C				;Carry Flag -> LED 1

				mov C, P2.7				;Button2 -> Carry Flag
				mov P2.1,C				;Carry Flag -> LED 2
				
				jmp loop					;Always run
				
				END								;All true code ends with "END"
