
#include <msp430g2553.h>

#define BUTTON 0x08

volatile unsigned int blink_interval;  // number of WDT interrupts per blink of LED
volatile unsigned int blink_counter;   // down counter for interrupt handler
volatile unsigned int mode;
volatile unsigned char last_button; // the state of the button bit at the last interrupt

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

	  P1DIR |= 0x01;					// Set P1.0 to output direction
	  P1DIR &= ~BUTTON;


	  P1OUT |= BUTTON;
	  P1REN |= BUTTON;
	  // initialize the state variables
	  blink_interval=67;                // the number of WDT interrupts per toggle of P1.0
	  blink_counter=blink_interval;     // initialize the counter
	  mode = 0;

	  _bis_SR_register(GIE+LPM0_bits);  // enable interrupts and also turn the CPU off!
}

interrupt void WDT_interval_handler(){
  	unsigned char b;

  	b= (P1IN & BUTTON);  // read the BUTTON bit
  	if (last_button && (b==0)){ // button debouncer
  		mode += 1;
  		P1OUT ^= 1;
  		if(mode == 1){
  			//P1OUT = 1;
  			mode =2;
  		}
  		else{
  			//P1OUT = 0;
  			mode = 0;
  		}
  	}
  	last_button=b;    // remember button reading for next time.
}

interrupt void blink(){
  if (--blink_counter==0){          // decrement the counter and act only if it has reached 0
	  P1OUT ^= 1;                   // toggle LED on P1.0
	  blink_counter=blink_interval; // reset the down counter
  }
}

ISR_VECTOR(WDT_interval_handler, ".int10")
