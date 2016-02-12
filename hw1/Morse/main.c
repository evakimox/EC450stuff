#include <msp430g2553.h>

volatile unsigned int blink_interval;  // number of WDT interrupts per blink of LED
volatile unsigned int blink_counter;   // down counter for interrupt handler
volatile unsigned int blink_times;
volatile unsigned int longorshort;
volatile unsigned int i=0;     			// i count flash times
volatile unsigned int j=0;              // j control the letter processing
volatile char letter;
volatile unsigned int junk=42;
char string[15] ="Hello Jingning";
const int unitlength = 50;

int main(void) {
	  // setup the watchdog timer as an interval timer
	  WDTCTL =(WDTPW + // (bits 15-8) password
	                   // bit 7=0 => watchdog timer on
	                   // bit 6=0 => NMI on rising edge (not used here)
	                   // bit 5=0 => RST/NMI pin does a reset (not used here)
	           WDTTMSEL + // (bit 4) select interval timer mode
	           WDTCNTCL +  // (bit 3) clear watchdog timer counter
	  		          0 // bit 2=0 => SMCLK is the source
	  		          +1 // bits 1-0 = 01 => source/8K
	  		   );
	  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)

	  P1DIR |= 0x01;

	  // initialize the state variables

	  blink_interval = 3*unitlength;                // the number of WDT interrupts per toggle of P1.0
	  blink_counter= unitlength;     // initialize the counter

	  letter = string[0];
	  P1OUT |=1;
	  P1OUT--;

	  _bis_SR_register(GIE+LPM0_bits);  // enable interrupts and also turn the CPU off!
}

// ===== Watchdog Timer Interrupt Handler =====
// This event handler is called to handle the watchdog timer interrupt,
//    which is occurring regularly at intervals of about 8K/1.1MHz ~= 7.4ms.

interrupt void WDT_interval_handler(){
	blink_counter--;
	if (blink_counter==0){
		switch(letter){

		case 'H':
			if(i<8){                      // flash short 4 times
				P1OUT ^= 1;
				if(i!=7){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;
		case 'e':
			if(i<2){                      // flash short 4 times
				P1OUT ^= 1;
				if(i==0){
					blink_counter=unitlength;
					i++;
				}
				else{
					blink_counter = blink_interval;// reset the down counter
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;

		case 'l':
			if(i<8){                      // flash 4 times
				P1OUT^=1;
				if(i==2||i==7){
					blink_counter=blink_interval; // L has 2nd long other short
					i++;
				}
				else{
					blink_counter=unitlength; // reset the down counter
					i++;
				}
			}
			else{
				blink_counter=blink_interval; //turn off led 3 unit
				j++;
				letter = string[j];
				i=0;
				junk++;

		case 'o':
			if(i<6){                      // flash long 3 times
				P1OUT ^= 1;
				if(i==1||i==3||i==5){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}

			}
			else{
				j++;
				letter = string[j];
				i=0;
		case ' ':
			if(i<2){                      // flash short 4 times
				//let it off
				if(i==0){
					blink_counter=unitlength;
					i++;
				}
				else{
					blink_counter = blink_interval;// reset the down counter
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;
		case 'J':
			if(i<8){                      // flash 4 times
			P1OUT ^= 1;
				if(i==0||i==1||i==3||i==5){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}
			}
			else{
				j++;
			letter = string[j];
			i=0;
		case 'i':
			if(i<4){                      // flash short 2 times
			P1OUT ^= 1;
				if(i!=3){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;
		case 'n':
			if(i<4){                      // flash 2 times
			P1OUT ^= 1;
				if(i==1||i==2){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;

		case 'g':
			if(i<6){                      // flash long 3 times
			P1OUT ^= 1;
				if(i==1||i==3||i==4||i==5){
					blink_counter=unitlength; // reset the down counter
					i++;
				}
				else{
					blink_counter=blink_interval;
					i++;
				}
			}
			else{
				j++;
				letter = string[j];
				i=0;
		case '\0':
			P1OUT |=1;
			P1OUT ^=1;
			j=0;
			letter = string[j];
			blink_counter = 7*unitlength;

			} //g
			} //n
			} //i
			} //J
			} //" "
			} //o
			} //l
			} //e
			} //H
		}//switch case end
	}
}



// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")