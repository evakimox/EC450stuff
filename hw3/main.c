#include "msp430g2553.h"

//Define buttons and output pin
#define TA0_BIT 0x02			//P1.1 SOUND OUTPUT
#define START 0x04				//P1.2 button
#define PAUSE 0x08				//P1.3 ON BOARD button
#define RESUME 0x10				//P1.4 button
#define SPEEDUP 0x20			//P1.5 button
#define SLOWDOWN 0x40			//P1.6 button
#define SCORE 0x80				//P1.7 button
#define ALLBUTTONS 0xfc			//All the P1.x except P1.0, 1.1

unsigned int playflag=0;
volatile float TimeCount=0;
volatile unsigned int speedDivisor=3;

const unsigned int toneArray[4]={497, 526, 591, 663};
const float notelength[4]={2, 1.5, 0.5, 4};

//functions used but defined later
void initTimerA();
void initializebuttons();
void PlaySong();

void main(void){
	//Set watchdog and enable it's interrupt
	WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 1);
    IE1 |= WDTIE;

    //Set timing
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	//Initialize timers, buttons
	initializebuttons();
	initTimerA();
	TimeCount=0;
	//Enable global interrupts
	_bis_SR_register(GIE);

	//Processing Loop
	while(1){
		if (playflag){PlaySong(); playflag=0;}	//Play song then reset flag.
		else;
	}
}

void initTimerA(){
	TA0CTL |= TACLR; 	//reset clock
	TA0CTL = TASSEL1+ID_0+MC_1;					//clock source is SMCLK, clk divider =1, up mode
	TA0CCTL0 = CCIE;							//turn on output compare interrupt, initially OUTMOD
	TA0CCR0 = 500;					//when output is triggered. TO make C_tone, eq => 261Hz= 1MHz/(x) ==> x=1MHz/(2*C_tone)

	//Connect Timer output to pin TA0
	P1SEL |= TA0_BIT;
	P1DIR |= TA0_BIT;
}

/*------------------------------------------*/
void PlaySong(){
	//Internal Variables
	unsigned int playingSong=1;
	unsigned int PlayingNote=0;

	//Play until the counter hits the end
	while (playingSong){
		TACCR0 = toneArray[PlayingNote]-1;							//selects which tone
		TimeCount=0;													//set TimeCount to 0
		TACCTL0 = CCIE + OUTMOD_4;										//turn sound on for tone
		while (TimeCount < notelength[PlayingNote]/speedDivisor){
			//do nothing
			//And wait for the sound to play it's full length
		}
		TimeCount=0;													//reset TimeCount
		TACCTL0 = CCIE;													//turn sound off for rest
		if (PlayingNote >= 3){playingSong=0;}						//if we are done playing, bring flag down which will exit loop/function
		else{PlayingNote+=1;}
	}//end while

}//end PlaySong

/*------------------------------------------*/

void initializebuttons(){
    //Initialize buttons
    P1OUT |= ALLBUTTONS;				//Declare a pull up
    P1REN |= ALLBUTTONS;				//Enable PUll up
    P1IES |= ALLBUTTONS;				//set to trigger on falling edge
    P1IFG &= ~ALLBUTTONS;				//Make sure flag is cleared initially
    P1IE |= ALLBUTTONS;					//Enable interrupt for button
}


void interrupt button_handler(){
	//pause
	if (P1IFG & PAUSE){
		P1IFG &= ~PAUSE;	//reset the interrupt flag
		playflag ^= 1;		//play and stop
		TACCTL0 ^= OUTMOD_4;	//turn off sound
	}
	//restart
	else if(P1IFG & START){
		P1IFG &= ~START;	//reset the interrupt flag
		//do things according to start
	}
	//resume from pause
	else if(P1IFG & RESUME){
		P1IFG &= ~RESUME;	//reset the interrupt flag
		//do things according to resume
	}
	//speed up
	else if(P1IFG & SPEEDUP){
		P1IFG &= ~SPEEDUP;	//reset the interrupt flag
		speedDivisor = 2*speedDivisor;
	}
	//slow down
	else if(P1IFG & SLOWDOWN){
		P1IFG &= ~SLOWDOWN;	//reset the interrupt flag
		if(speedDivisor>1){
			speedDivisor = speedDivisor/2;
		}
		else{
			speedDivisor = speedDivisor;	//otherwise it will overflow and the tone will stay and never end
		}
	}
	//change the score
	else if(P1IFG & SCORE){
		P1IFG &= ~SCORE;	//reset the interrupt flag
		//changes to the score
	}
	else{
		//do nothing
	}
}//end button_handler


void interrupt AnotherWDT(){
}

void interrupt WDT_interval_handler(){
	//If playing the song, add the time between to the TimeCount
	if (playflag){
		TimeCount += 0.01;
	}
	else{
		TimeCount=0;

	}
}

/*------------------------------------------*/
//Declare interrupt vectors.
ISR_VECTOR(button_handler,".int02")
ISR_VECTOR(AnotherWDT,".int09")
ISR_VECTOR(WDT_interval_handler,".int10")

