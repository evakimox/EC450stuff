/* 10-30-13
 * This is an example of using the ADC to convert a single
 * analog input. The external input is to ADC10 channel A4.
 * This version triggers a conversion with regular WDT interrupts and
 * uses the ADC10 interrupt to copy the converted value to a variable in memory.
 * ALSO -- this one sends the conversion number and value to the UART at 9600 baud
 * every 500 conversions.
 * NOTE: formatted string output used here requires that the printf support move
 *       up from the default "minimal" to (at least) "nofloat".  This is in the
 *       CCS advanced project options, Library Function Assumptions.
*/

#include "msp430g2553.h"

// Define bit masks for ADC pin and channel used as P1.4
#define ADC_INPUT_BIT_MASK 0x10
#define ADC_INCH INCH_4

 /* declarations of functions defined later */
 void init_adc(void);
 void init_wdt(void);
 void initializebuttons(void);

// declarations for UART calls
#include "uart_out.h"
// include declarations for snprintf from the library & generic string stuff
#include <string.h>
#include <stdio.h>

// =======ADC Initialization and Interrupt Handler========

// Global variables that store the results (read from the debugger)
volatile int latest_result;   // most recent result is stored in latest_result
volatile unsigned long conversion_count=0; //total number of conversions done

// Globals for printing
unsigned int conPrint=500; // number of conversions for each print line
unsigned int conPrintCounter; // down counter for printing conversions
unsigned int conPrintSequence;// output line numbers

// a buffer to construct short text output strings
#define CBUFLEN 20
#define TempUp 0x01		// P1.0
#define TempDown 0x02		// P1.1
#define UnitSelect 0x08		// P1.3
#define FanSwitch 0x40		// P1.6
#define ALLBUTTONS 0x0B		//buttons 1.0 1.1 1.3

char cbuffer[CBUFLEN]; // buffer for output of characters
char cbuffer2[CBUFLEN];
int flag =0;
int TempSelect = 1;
int handsetTemp = 20;
int Temperature[2];

/*
 * The ADC handler is invoked when a conversion is complete.
 * Its action here is to store the result in memory.
 * and possibly print.
 */
 void interrupt adc_handler(){
	 latest_result=ADC10MEM;   // store the answer
	 ++conversion_count;       // increment the total conversion count
	 if (--conPrintCounter==0){// maybe print
		 conPrintCounter=conPrint;
		 //string for latest result -- CR but NO linefeed
		 if(flag){
			 int DegreeC = (latest_result-263)/9.452055;
			 int DegreeF = (latest_result-95)/5.25114;
			 Temperature[0] = DegreeC;
			 Temperature[1] = DegreeF;
			 //add logic check temperature bits 1,2,3
			 //positive and 3 digit
			 int threeDigit = (Temperature[TempSelect]/100>0)&&(Temperature[TempSelect]>0);
			 int twoDigitNeg = (Temperature[TempSelect]<0)&&((-Temperature[TempSelect]/10)>0)&&((-Temperature[TempSelect]/10)<10);
			 threeDigit = (threeDigit||twoDigitNeg);
			 //positive and one digit
			 int oneDigit = (Temperature[TempSelect]>0)&&(!(Temperature[TempSelect]/10));
			 //  				2 digits positive			1 digit negative
			 int twoDigit = (Temperature[TempSelect]/10)||(!(-Temperature[TempSelect]/10>0));
			 if(threeDigit){
				 snprintf(cbuffer,CBUFLEN,"Temperature: %d",Temperature[TempSelect]);
			 }
			 else if(twoDigit){
				 snprintf(cbuffer,CBUFLEN,"Temperature: %d ",Temperature[TempSelect]);
			 }
			 else if(oneDigit){
				 snprintf(cbuffer,CBUFLEN,"Temperature: %d  ",Temperature[TempSelect]);
			 }
			 else{
				 //print not supported report
				 snprintf(cbuffer,CBUFLEN,"Sorry no support ");
			 }
			 flag = 0;
		 }
		 else{
			 //add logic check temperature bits 1,2,3
			 int threeDigit = (handsetTemp/100>0)&&(handsetTemp>0);
			 int twoDigitNeg = (handsetTemp<0)&&((-handsetTemp/10)>0)&&((-handsetTemp/10)<10);
			 threeDigit = (threeDigit||twoDigitNeg);
			 //positive and one digit
			 int oneDigit = (handsetTemp>0)&&(!(handsetTemp/10));
			 //  				2 digits positive			1 digit negative
			 int twoDigit = (handsetTemp/10)&&(!threeDigit);
			 int oneDigitNeg = (handsetTemp<0)&&(((-handsetTemp)/10)<1);


			 if(TempSelect){
				 if(threeDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree F %d",handsetTemp);
				 }
				 else if(twoDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree F %d ",handsetTemp);
				 }
				 else if(oneDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree F %d  ",handsetTemp);
				 }
				 else{
					 //print not supported report
					 snprintf(cbuffer,CBUFLEN,"Sorry no support ");
				 }
			 }
			 else{
				 if(threeDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree C %d",handsetTemp);
				 }
				 else if(twoDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree C %d ",handsetTemp);
				 }
				 else if(oneDigit){
					 snprintf(cbuffer,CBUFLEN,"Set Degree C %d  ",handsetTemp);
				 }
				 else{
					 //print not supported report
					 snprintf(cbuffer,CBUFLEN,"Sorry no support ");
				 }
			 }
			 flag = 1;
		 }


		 //tx_start_string(CURSOR);
		 tx_start_string(cbuffer); // initiate printed (after interrupts are allowed)
		// tx_start_string(cbuffer2);
	 }
 }
 ISR_VECTOR(adc_handler, ".int05")

// Initialization of the ADC
void init_adc(){
  ADC10CTL1= ADC_INCH	//input channel 4
 			  +SHS_0 //use ADC10SC bit to trigger sampling
 			  +ADC10DIV_4 // ADC10 clock/5
 			  +ADC10SSEL_0 // Clock Source=ADC10OSC
 			  +CONSEQ_0; // single channel, single conversion
 			  ;
  ADC10AE0=ADC_INPUT_BIT_MASK; // enable A4 analog input
  ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
 	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
 	          +ADC10ON	//turn on ADC10
 	          +ENC		//enable (but not yet start) conversions
 	          +ADC10IE  //enable interrupts
 	          ;
}



// ===== Watchdog Timer Interrupt Handler =====
interrupt void WDT_interval_handler(){
	//compare the temperature to the set one
	if(handsetTemp<Temperature[TempSelect]){
		P1OUT |= 0x40;
	}
	else{
		P1OUT &=~0x40;
	}
	ADC10CTL0 |= ADC10SC;  // trigger a conversion
}
ISR_VECTOR(WDT_interval_handler, ".int10")

 void init_wdt_no_interrupt(){
	// setup the watchdog timer as an interval timer
	// INTERRUPT NOT YET ENABLED!
  	WDTCTL =(WDTPW +		// (bits 15-8) password
     	                   	// bit 7=0 => watchdog timer on
       	                 	// bit 6=0 => NMI on rising edge (not used here)
                        	// bit 5=0 => RST/NMI pin does a reset (not used here)
           	 WDTTMSEL +     // (bit 4) select interval timer mode
  		     WDTCNTCL  		// (bit 3) clear watchdog timer counter
  		                	// bit 2=0 => SMCLK is the source
  		     +0x01           	// bits 1-0 = 00 => source/32K
 			 );
 }

void initializebuttons(){
    //Initialize buttons
    P1OUT |= ALLBUTTONS;				//Declare a pull up
    P1REN |= ALLBUTTONS;				//Enable PUll up
    P1IES |= ALLBUTTONS;				//set to trigger on falling edge
    P1IFG &= ~ALLBUTTONS;				//Make sure flag is cleared initially
    P1IE |= ALLBUTTONS;					//Enable interrupt for button
    P1DIR |= FanSwitch;
}

void interrupt button_handler(){
//handles 3 buttons, 1 toggle display, 2 set temperature
//pin to voltage divider and then fan
	if(P1IFG&TempUp){
		//set temp up
		P1IFG &= ~TempUp;
		handsetTemp++;
	}
	else if(P1IFG&TempDown){
		//set temp up
		P1IFG &= ~TempDown;
		handsetTemp--;
	}
	else if(P1IFG&UnitSelect){
		//set temp up
		P1IFG &= ~UnitSelect;
		TempSelect = !TempSelect;
	}
}
ISR_VECTOR(button_handler,".int02")
/*
 * The main program just initializes everything and leaves the action to
 * the interrupt handlers!
 */

void main(){

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;			// 8Mhz calibration for clock
  	DCOCTL  = CALDCO_8MHZ;

  	init_adc();
  	init_wdt_no_interrupt(); // does NOT enable the wdt interrupt yet
  	init_USCI_UART(); // initialize the UART
  	initializebuttons();
  	// Setup printing
  	conPrintCounter=conPrint;
  	conPrintSequence=1;
  	_bis_SR_register(GIE); //enable interrupts so we can print.


  	IE1 |= WDTIE; // enable WDT interrupt so that conversions can start

  	_bis_SR_register(LPM0_bits); //powerdown CPU

}


