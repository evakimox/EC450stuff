asm(" .length 10000");
asm(" .width 132");
#include "msp430g2553.h"
#define TA0_BIT 0x02
#define BUTTON_BIT 0x08
volatile unsigned int initialHalfPeriod = 1000;
#define ADC_INPUT_BIT_MASK 0x10
#define ADC_INCH INCH_4
volatile unsigned int i = 0;

void init_timer(void); // routine to setup the timer
void init_button(void); // routine to setup the button
//adc:
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

void main(){
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;    // 1Mhz calibration for clock
	DCOCTL  = CALDCO_1MHZ;

	init_timer();  // initialize timer
	init_button(); // initialize the button
	init_adc();

	_bis_SR_register(GIE/*+LPM0_bits*/);// enable general interrupts and power down CPU

	while(1){
		start_conversion();
		latest_result = get_result();
		int i = 0;
		initialHalfPeriod=latest_result;
		while(i<500){
		TA0CCR0 = initialHalfPeriod-1;
		i++;
		}
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
	TA0CCTL0=0; // compare mode, output mode 0, no interrupt enabled
	TA0CCR0 = initialHalfPeriod-1; // in up mode TAR=0... TACCRO-1
	P1SEL|=TA0_BIT; // connect timer output to pin
	P1DIR|=TA0_BIT;
}

void init_button(){
// All GPIO's are already inputs if we are coming in after a reset
	P1OUT |= BUTTON_BIT; // pullup
	P1REN |= BUTTON_BIT; // enable resistor
	P1IES |= BUTTON_BIT; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT;// clear interrupt flag
	P1IE  |= BUTTON_BIT; // enable interrupt
}

void interrupt button_handler(){
// check interrupt comes from the desired bit...
// (if not, just ignore -- cannot happen in this case)
	if (P1IFG & BUTTON_BIT){
		P1IFG &= ~BUTTON_BIT; // reset the interrupt flag
		TACCTL0 ^= OUTMOD_4; // toggle outmod between 0 and 4 (toggle)
	}
}
ISR_VECTOR(button_handler,".int02") // declare interrupt vector
// +++++++++++++++++++++++++++


