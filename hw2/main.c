asm(" .length 10000");
asm(" .width 132");
/*
* calibrator.c
*
* This program is intended to be used to calibrate the system DCO clock in the
* debugger using the ACL as a time base.
*
* The procedure run_count runs a tight counting loop incrementing a 32 bit counter.
* It enables the WDT interval interrupt driven by the ACL as a source.
* The WDT handler sets a flag which is examined in the counting loop and which causes the
* main program to reset and record its count in an array in memory.
*
* Created on: Feb 9, 2016
* Author: Roscoe Giles
*/
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
	//DCOCTL= MOD_3;
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
	index=0; // initial index for storage of data
	_bis_SR_register(GIE); // enable global interrupts
	while(1){
		count[index++]=run_counter();
		if (index>=N_samples){
			index=0;
		}
	}
}
