#include <msp430.h> 

/*
 * main.c
 */
int main(void) {
	unsigned int delay = 60000;
	volatile unsigned int mode;
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    P1DIR |= 0x01;

    P1OUT = 0x08;
    P1REN = 0x08;

    mode = 1;

    for(;;){
    	volatile unsigned int i;

    	if(mode==1){
    		P1OUT ^= 0x01;
    	} else {
    		P1OUT |= 0x01;
    	}
    	i = delay;
    	unsigned int changemode = 0;
    	while(i!=0){
    		i--;
    		if(P1IN&0x08){
    		//not pressed do nothing
    		} else{
    		//pressed - change mode flag
    		changemode = 1;
    		}
    	}
    	mode = changemode?(!mode):mode;
    }
	
	return 0;
}
