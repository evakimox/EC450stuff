#include <msp430.h>

int main(void) {
	unsigned int delay=65000;

	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	P1DIR |= 0x01;					// Set P1.0 to output direction

	for(;;) {
		volatile unsigned int i;	// volatile to prevent optimization

		P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR

		i = delay;					// SW Delay
		do i--;
		while(i != 0);
	}
	
	return 0;
}
