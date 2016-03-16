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

volatile int PlayingNote = 0;
unsigned int playflag=0;
volatile unsigned int restartflag = 0;
volatile float TimeCount=0;
volatile unsigned int speedDivisor=3;
volatile unsigned int scorechanger = 1;

const unsigned int toneArray[4]={497, 526, 591, 663};
const float notelength[4]={2, 1.5, 0.5, 4};

//functions used but defined later
void initTimerA();
void initializebuttons();
void PlaySong();

void main(void){
	//Use WDT to control how long each tone is
	WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 1);
    IE1 |= WDTIE;

	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	//Call timers, buttons interrupts
	initializebuttons();
	initTimerA();
	TimeCount=0;

	_bis_SR_register(GIE);

	//call to play song
	while(1){
		if (playflag){
			PlaySong();
			playflag=0;
		}	//play only once
	}
}

void initTimerA(){
	//set timer basics
	TA0CTL |= TACLR;
	TA0CTL = TASSEL1+ID_0+MC_1;
	TA0CCTL0 = CCIE;
	TA0CCR0 = 500;

	//Connect Timer output to pin P1.1
	P1SEL |= TA0_BIT;
	P1DIR |= TA0_BIT;
}

//playsong function
void PlaySong(){
	//Internal Variables
	unsigned int playingSong=1;

	//Play until the counter hits the end
	while (playingSong){
		TACCR0 = (toneArray[PlayingNote]/scorechanger)-1;
		TimeCount=0;													//set TimeCount to 0
		TACCTL0 = CCIE + OUTMOD_4;
		while (TimeCount < notelength[PlayingNote]/speedDivisor){
			//catch the restart flag at any time
			if(restartflag){
				PlayingNote = -1;
				restartflag = 0;
			}
		}
		TimeCount=0;				//set TimeCount to 0
		TACCTL0 = CCIE;				//turn sound off when one tone finishes
		while(TimeCount < 0.08){
			//do nothing and wait, this is notes seperator
		}
		TimeCount = 0;
		if (PlayingNote >= 3){	//when the whole piece is finished
			playingSong = 0;
			PlayingNote = 0;
		}
		else{
			PlayingNote += 1;
		}
	}//end outer while loop
}//end PlaySong


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
		playflag = 0;		//play and stop
		TACCTL0 = CCIE;	//turn off sound
	}
	//restart
	else if(P1IFG & START){
		P1IFG &= ~START;	//reset the interrupt flag
		playflag = 1;
		restartflag = 1;
	}
	//resume from pause
	else if(P1IFG & RESUME){
		P1IFG &= ~RESUME;	//reset the interrupt flag
		playflag = 1;
		TACCTL0 = OUTMOD_4;		//turn on the sound and actually do nothing else
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
		if(scorechanger == 1){
			scorechanger = 2;
		}
		else{
			scorechanger = 1;
		}
	}
	else{
		//do nothing
	}
}//end button_handler


void interrupt AnotherWDT(){
}

void interrupt WDT_interval_handler(){
	//count the time that a note is played
	if (playflag){
		TimeCount += 0.01;
	}
	else{
		TimeCount=0;	//reset counter
	}
}

/*------------------------------------------*/
//Declare interrupt vectors.
ISR_VECTOR(button_handler,".int02")
ISR_VECTOR(AnotherWDT,".int09")
ISR_VECTOR(WDT_interval_handler,".int10")

