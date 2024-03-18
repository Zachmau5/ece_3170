#include <C8051F020.h>
#include <lcd.h>

void main()
{
   WDTCN = 0xde;  // disable watchdog
   WDTCN = 0xad;
   XBR2 = 0x40;   // enable port output
   XBR0 = 4;      // enable uart 0
   OSCXCN = 0x67; // turn on external crystal
   TMOD = 0x20;   // wait 1ms using T1 mode 2
   TH1 = -167;    // 2MHz clock, 167 counts - 1ms
   TR1 = 1;
   while ( TF1 == 0 ) { }          // wait 1ms
   while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
   OSCICN = 8;    // switch over to 22.1184MHz
   SCON0 = 0x50;  // 8-bit, variable baud, receive enable
   TH1 = -6;      // 9600 baud
   for ( ; ; )
   {
      // wait for data to arrive
      if ( !RI0 ) continue;
      // clear the receiver flag & echo the data
      RI0 = 0;
      SBUF0 = SBUF0;
   }
}
