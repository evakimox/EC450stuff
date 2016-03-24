asm(" .length 10000");
asm(" .width 132");
// Oct 2015
// TimerTone0 == produces a 1 Khz tone using TimerA, Channel 0
// Toggled on and off with a button
// Using the timer in up mode with NO INTERRUPT
//
// Sound is turned on and off by directly manipulating
// the TACCTL0 register.  The half period is not dynamically
// updated (though it can be changed in the debugger by
// changing TACCR0.
// The pushbutton is not debounced in any way!

#include "msp430g2553.h"
//-----------------------
// The following definitions allow us to adjust some of the port properties
// for the example:

// define the bit mask (within P1) corresponding to output TA0
#define TA0_BIT 0x02
#define ADC_INPUT_BIT_MASK 0x10
#define ADC_INCH INCH_4

volatile unsigned int initialHalfPeriod = 500;

void init_timer(void);
void init_adc(void);
void start_conversion(void);
int get_result(void);

volatile int latest_result;
/* basic adc operations */
void start_conversion(){
	ADC10CTL0 |= ADC10SC;
}

unsigned delay_hits=0; /* counter to estimate the busy wait time */

int get_result(){
	delay_hits=0;
	while (ADC10CTL1 & ADC10BUSY) {++delay_hits;}// busy wait for busy flag off
	return ADC10MEM;
}

void init_adc(){
	ADC10CTL1= ADC_INCH //input channel bit field
			+SHS_0 //use ADC10SC bit to trigger sampling
			+ADC10DIV_4 // ADC10 clock/5
			+ADC10SSEL_0 // Clock Source=ADC10OSC
			+CONSEQ_0; // single channel, single conversion

	ADC10AE0=ADC_INPUT_BIT_MASK; // enable A4 analog input
	ADC10CTL0= SREF_0 //reference voltages are Vss and Vcc
			+ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
			+ADC10ON //turn on ADC10
			+ENC //enable (but not yet start) conversions
			;
}

// ++++++++++++++++++++++++++
void main(){
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;    // 1Mhz calibration for clock
	DCOCTL  = CALDCO_1MHZ;
	init_adc();

	while(1){
		start_conversion();
		latest_result=get_result();
		init_timer();  // initialize timer
	}

}

// +++++++++++++++++++++++++++
// Sound Production System

void init_timer(){              // initialization and start of timer
	TA0CTL |= TACLR;            // reset clock
	TA0CTL = TASSEL_2+ID_0+MC_1;// clock source = SMCLK
	                            // clock divider=1
	                            // UP mode
	                            // timer A interrupt off
	TA0CCTL0=OUTMOD_4; // compare mode, output mode 0, no interrupt enabled
	TA0CCR0 = initialHalfPeriod-1; // in up mode TAR=0... TACCRO-1
	P1SEL|=TA0_BIT; // connect timer output to pin
	P1DIR|=TA0_BIT;
}



