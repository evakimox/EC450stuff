#include "msp430g2553.h"

//port declarations for SPI, LED, and Button
#define RED 0x01
#define BUTTON 0x08
#define SPI_CLK 0x20
#define MISO 0x40
#define MOSI 0x80
#define BRLO (BIT_RATE_DIVISOR &  0xFF)
#define BRHI (BIT_RATE_DIVISOR / 0x100)
#define ACTION_INTERVAL 1
#define BIT_RATE_DIVISOR 32

//Global Variables
volatile unsigned char previousButton = 0;		//button tracker
volatile unsigned int randint = 0;
volatile unsigned int lowerBits = 0;		//lower bits of the guess
volatile unsigned int upperBits = 0;		//upper bits of the guess
volatile unsigned int total_guess = 0;		//actual guess
volatile unsigned int WDT_counter = 0;		//WDT counter
volatile unsigned int state = 's';			//state of the game
volatile unsigned int started = 0;			//tell if the game is running
volatile unsigned int data_received = 0;	//last byte received
volatile unsigned int data_to_send = 0;		//character to be sent
volatile unsigned int tx_mode = 0;			//transmit mode flag
volatile unsigned int rx_mode = 0;			//receive mode flag
volatile unsigned int num_guess = 0;

//function declarations
void init_button(void);
void init_spi(void);
void init_wdt(void);
unsigned int rand(void);

//main program
void main(){
	WDTCTL = WDTPW + WDTHOLD;	// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
  	DCOCTL  = CALDCO_1MHZ;

  	P1DIR &= ~BUTTON;			//button initialization
  	P1OUT |= BUTTON;
  	P1REN |= BUTTON;

  	P1DIR |= RED;				//LED initialization
  	P1OUT &= ~RED;

  	init_spi();
  	init_wdt();
 	_bis_SR_register(GIE+LPM0_bits);
}

//SPI initialization
void init_spi(){
	UCB0CTL1 = UCSSEL_2+UCSWRST;  		// Reset state machine; SMCLK source;
	UCB0CTL0 = 					// Data capture on rising edge
			   							// read data while clock high
										// lsb first, 8 bit mode,
			   +UCMST					// master
			   +UCMODE_0				// 3-pin SPI mode
			   +UCSYNC;					// sync mode (needed for SPI or I2C)
	UCB0BR0=BRLO;						// set divisor for bit rate
	UCB0BR1=BRHI;
	UCB0CTL1 &= ~UCSWRST;				// enable UCB0 (must do this before setting
										//              interrupt enable and flags)
	IFG2 &= ~UCB0RXIFG;					// clear UCB0 RX flag
	IE2 |= UCB0RXIE;
	// Connect I/O pins to UCB0 SPI
	P1SEL |=SPI_CLK+MISO+MOSI;
	P1SEL2|=SPI_CLK+MISO+MOSI;
}

//WDT initialization
void init_wdt(){
	// setup the watchdog timer as an interval timer
  	WDTCTL = (WDTPW +		// (bits 15-8) password
     	                   	// bit 7=0 => watchdog timer on
       	                 	// bit 6=0 => NMI on rising edge (not used here)
                        	// bit 5=0 => RST/NMI pin does a reset (not used here)
           	 WDTTMSEL +     // (bit 4) select interval timer mode
  		     WDTCNTCL  		// (bit 3) clear watchdog timer counter
  		                	// bit 2=0 => SMCLK is the source
  		                	// bits 1-0 = 10=> source/512
 			 );
  	IE1 |= WDTIE; // enable WDT interrupt
 }

//s = start, l = lower bits, u = upper bits, g = guessing state
//WDT interrupt handler, channels through the various states, receives inputs through the
//SPI interrupt handler
interrupt void WDT_interval_handler(){
	unsigned char b;
	b = (P1IN & BUTTON);
	//if the button pressed, initialize everything
	//This will not initiate command after the guessing game has started
	//This can restart the game if the game ends (push the button to restart)
	if (previousButton && (b==0) && (started != 1)){
		randint = (0xFFFF & rand());		//random integer
		data_to_send = 's';
		UCB0TXBUF = data_to_send;
		P1OUT |= RED;
		state = 's';						//starting state
		started = 1;						//guessing game has begun
		tx_mode = 1;						//sets to transmit
		WDT_counter = 1;					//WDT interrupt count
		num_guess = 0;						//number of guesses done
	}

	if (started == 1 && WDT_counter == 0){	//guessing game is running
		//lower bits state goes into receiving mode and ensures
		//it does not go to the otehr states without knowing
		if ((state == 's') && (rx_mode == 1)){
			data_to_send = 'l';
			rx_mode = 0;					//no longer receiving
			state = 'l';
		}
		//upper bits state goes into receiving mode and ensures
		//it does not go to the otehr states without knowing
		else if (state == 'l'){
			data_to_send = 'u';
			rx_mode = 0;					//no longer receiving
			state = 'u';
		}
		//guess state goes into transmit
		//it does not go to the other states without knowing
		else if (state == 'g'){
			if (total_guess < randint){		//if guess is less than
				data_to_send = '<';
			}
			else if (total_guess > randint){//if guess is greater than
				data_to_send = '>';
			}
			else if (total_guess == randint){//if guess is equal to
				P1OUT &= ~RED;				//turn off LED to tell it guessed the number
				started = 0;				//and stop running program that sending slave signals
				data_to_send = '=';			//last signal to be send
			}
			state = 's';					//Prepare the next guessing game
			tx_mode = 1;
			num_guess++;
		}
		UCB0TXBUF = data_to_send;			//send data
		WDT_counter = 1;
	}
	previousButton=b;							//button debonce stuff
	WDT_counter--;
}
ISR_VECTOR(WDT_interval_handler, ".int10")

//spi interrupt handler for the master, handles the 2 byte inputs
void interrupt spi_rx_handler(){
	//if transmitting, turn it off and go into receiving mode
	if (tx_mode){
		tx_mode = 0;
		rx_mode = 1;
	}
	//retrieves lower bits from receive buffer
	else if (state == 'l'){
		data_received = UCB0RXBUF;
		lowerBits = data_received;
	}
	//retrieves upper bits from the receive buffer
	else if (state == 'u'){
		data_received = UCB0RXBUF;
		upperBits = data_received << 8;		//shifts the upper bits
		total_guess = upperBits + lowerBits;	//creates the num to be guessed
		state = 'g';							//changes to the guess state
	}
	IFG2 &= ~UCB0RXIFG;							//turns off flag
}
ISR_VECTOR(spi_rx_handler, ".int07")

/**
* Algorithm from TI SLAA338:
* http://www.ti.com/sc/docs/psheets/abstract/apps/slaa338.htm
*
* @return 16 random bits generated from a hardware source.
*/
#ifndef __RAND_H
#define __RAND_H
//LCG Constants
#define M 49381 // Multiplier
#define I 8643 // Increment
#endif

unsigned int rand(void) {
	int i, j;
	unsigned int result = 0;
	//saves the state
	unsigned int BCSCTL3_old = BCSCTL3;
	unsigned int TACCTL0_old = TACCTL0;
	unsigned int TACTL_old = TACTL;

	//Stops the timer
	TACTL = 0x0;
	//sets up the timer
	BCSCTL3 = (~LFXT1S_3 & BCSCTL3) | LFXT1S_2; // Source ACLK from VLO
	TACCTL0 = CAP | CM_1 | CCIS_1; // Capture mode, positive edge
	TACTL = TASSEL_2 | MC_2; // SMCLK, continuous up

	//Generates bits
	for (i = 0; i < 16; i++) {
		unsigned int ones = 0;
		for (j = 0; j < 5; j++) {
			while (!(CCIFG & TACCTL0)); // Wait for interrupt
				TACCTL0 &= ~CCIFG; // Clear interrupt
			if (1 & TACCR0) // If LSb set, count it
				ones++;
		}
		result >>= 1; // Save previous bits
		if (ones >= 3) // Best out of 5
			result |= 0x8000; // Set MSb
	}
	//Restores to the original state
	BCSCTL3 = BCSCTL3_old;
	TACCTL0 = TACCTL0_old;
	TACTL = TACTL_old;
	return result;
}



