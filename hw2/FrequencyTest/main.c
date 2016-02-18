asm(" .length 10000");
asm(" .width 132");

#include <msp430g2553.h>
#define N_samples 15
unsigned long count[N_samples]; // count samples are stored in this array
volatile int index;

volatile int count_flag;
unsigned long run_counter(){
	unsigned long counter;
	count_flag=1;
	counter=0;
	// setup WDT: source=ACL, clear counter, divide by 32K
	WDTCTL = WDTPW+WDTTMSEL+WDTCNTCL+WDTSSEL;
	IFG1 &= ~WDTIFG; // clear WDT IFG
	IE1 |= WDTIE; // enable WDT interrupt
	while(count_flag) ++counter;
	IE1 &= ~WDTIE; //disable WDT interrupt
	return counter;
}

interrupt void WDT_handler(){
	count_flag=0;
}
ISR_VECTOR(WDT_handler, ".int10")

void main(){
	WDTCTL=WDTPW+WDTHOLD; // disable WDT for now

	//trying to adjust the clock:
	/*
	DCOCTL = 0x60;   //according to data sheet DCO=3 MOD=0 is closest to 10Mhz
	//keep the first bit unchanged
	//according to data sheet set RSEL bits (lower 4 bits) to be 1110 =14 will make it closest to 10Mhz
	BCSCTL1 = 0x8E;
	*/

	index=0; // initial index for storage of data
	_bis_SR_register(GIE); // enable global interrupts
	while(1){
		count[index++]=run_counter();
		if (index>=N_samples){
			//put a break point here
			//see data in the counter array and take notes
			index=0;
		}
	}
}
