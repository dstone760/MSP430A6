#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <libemb/shell/shell.h>
#include <math.h>
#include <stdlib.h>

//forward declarations
int shell_cmd_help(shell_cmd_args *args);
int shell_cmd_reset(shell_cmd_args *args);
int shell_cmd_set(shell_cmd_args *args);
int shell_cmd_current(shell_cmd_args *args);
int shell_cmd_frequency(shell_cmd_args *args);
int shell_cmd_speaker(shell_cmd_args *args);
int shell_cmd_speed(shell_cmd_args *args);

//globals
static volatile int currentNumber = 0;
static volatile int primeFlag = 0;
static volatile int twoFlag = 0;
static volatile int tensFlag = 0;
static volatile long int counter = 0; //this is long because once it got past 2^15, it would go negative
static volatile int divisor = 1;


//shell command structure
shell_cmds my_shell_cmds = {
	.count = 7,
	.cmds  = {
		{
			.cmd  = "help",
			.desc = "list available commands\n\r	'help'\n\r",
      			.func = shell_cmd_help
    		},
		{
			.cmd = "reset",
			.desc = "reset the MSP430\n\r	'reset'\n\r",
			.func = shell_cmd_reset
		},
		{
			.cmd = "set",
			.desc = "set the value\n\r	'set [0-999]'\n\r",
			.func = shell_cmd_set
		},
		{
			.cmd = "current",
			.desc = "get the current value\n\r	'current'\n\r",
			.func = shell_cmd_current
		},
		{
			.cmd = "frequency",
			.desc = "set the speaker frequency\n\r	'frequency [0-32000]'\n\r",
			.func = shell_cmd_frequency
		},
		{
			.cmd = "speaker",
			.desc = "toggle speaker\n\r	'speaker'\n\r",
			.func = shell_cmd_speaker
		},
		{
			.cmd = "speed",
			.desc = "set speed\n\r	'speed [1-16]'\n\r",
			.func = shell_cmd_speed
		}
  	}
};



int shell_cmd_help(shell_cmd_args *args){
	//I kept the help function because it's actually pretty useful
  	int k;
  	for(k = 0; k < my_shell_cmds.count; k++) {
    		cio_printf("%s: %s\n\r", my_shell_cmds.cmds[k].cmd, my_shell_cmds.cmds[k].desc);
  	}
  	return 0;
}

int shell_cmd_reset(shell_cmd_args *args){
	//turn on watchdog + infinite loop = reset
	WDTCTL &= ~(WDTPW | WDTHOLD);
	for(;;){};
	return 0;
}

int shell_cmd_set(shell_cmd_args *args){
	//set the current value based on my program's logic
	long int k = atoi(args->args[0].val); //long int because it has to be compatible with long int counter
	if(k >=0 && k <= 999){
		counter = k * 100 / divisor;
	}
	return 0;
}

int shell_cmd_current(shell_cmd_args *args){
	//self-explanatory
	cio_printf("\n\rThe current value is %i \n\r", currentNumber);
	return 0;
}

int shell_cmd_frequency(shell_cmd_args *args){
	//basically call my frequency function from the menu
	int k = atoi(args->args[0].val);
	if(k > 0 && k <= 32000){
		setFrequency(atoi(args->args[0].val));
	}	
	return 0;
}

int shell_cmd_speaker(shell_cmd_args *args){
	//turn speaker on
	P2SEL ^= BIT6;
	return 0;
}

int shell_cmd_speed(shell_cmd_args *args){
	//change the speed by changing the divisor. infact, the divisor exists only for this command
	int k = atoi(args->args[0].val);
	if(k >= 1 && k <= 16){
		divisor = k;
		counter = currentNumber * 100 / divisor;
	}
	return 0;
}


int shell_process(char *cmd_line){
  	return shell_process_cmds(&my_shell_cmds, cmd_line);
}

void main(void){
	//watchdog timer, calibrations, clock and serial initialization
	WDTCTL = WDTPW | WDTHOLD;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
	BCSCTL3  = LFXT1S_2;
	serial_init(9600);

	//make sure I have the right starting environment
	P2DIR = 0b11111111;
	P2OUT = 0b00000000;
	P2SEL = 0b00000000;

	//bits 1, 2, and 3 are 0 because thats the UART and button
	P1DIR = 0b11110001;
	//same reasoning here for them being 1. Im not sure it actually matters.
	P1OUT = 0b00001110;
	//But here it definitely matters because UART has to be 1 or else the serial stuff doesnt work. Button has to be 0 or else it doesnt work as the interrupt
	P1SEL = 0b00000110;

	//Resistor on button. Doesnt work without it for some reason.
	P1REN = BIT3;
	//Enable interrupt on button, set edge select, make sure the interrupt flag is off. 
	P1IE |= BIT3;
	P1IES |= (BIT3);
	P1IFG &= ~(BIT3);

	//set the frequency (obviously)
	setFrequency(250);
	//Toggle speaker when timer passes that value.. toggle every 250 cycles
	TA0CCTL1 = OUTMOD_4;
	//use SMCLK, use "Up-To-CCR0 Mode", divide clock input by 8
	TA0CTL = TASSEL_2 + MC_1 + ID_3;
	

	//I had to find the right amount of cycles that the interrupt would trigger often enough that it wouldn't flash, but not so often that it would take up the entire CPU
	TA1CCR0 = 10000;
	//every 10000 cycles, toggle the interrupt
	TA1CCTL0 = CCIE;
	//use SMCLK, use "Up-To" mode
	TA1CTL = TASSEL_2 + MC_1;
	
	//enable interrupt
	_BIS_SR(GIE);

	//silly way for clearing the screen and making sure its divided up
	cio_print("\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
	cio_printf("\n\r*****START PROGRAM*****\n\r");
	for(;;){
		//command line logic. edited a little bit because the delete key stuff was wrong (for me)
		int m = 0;                              
    		char cmd_line[90] = {0};                
    		cio_print((char *) "\n\r# ");               
    		char c = cio_getc();                    
    		while(c != '\r') {                      	
			if(c == 0x7F) {           
        			if(m != 0) {
					cio_print("\b \b");
					cmd_line[m--] = 0;
        			}
      			} else {
				cmd_line[m++] = c;
				cio_printc(c);
      			}
     		 	c = cio_getc();                       
    		}
    		cio_print((char *) "\n\n\r");
		switch(shell_process(cmd_line))         
    		{                                       
      			case SHELL_PROCESS_ERR_CMD_UNKN:
        			cio_print((char *) "ERROR, unknown command given\n\r");
        			break;
      			case SHELL_PROCESS_ERR_ARGS_LEN:
        			cio_print((char *) "ERROR, an arguement is too lengthy\n\r");
        			break;
      			case SHELL_PROCESS_ERR_ARGS_MAX:
        			cio_print((char *) "ERROR, too many arguements given\n\r");
        			break;
      			default:
        			break;
    		}
    		cio_print((char *) "\n");
	}
}

void setFrequency(int num){
	//easy way to set frequency
	TA0CCR0 = floor(125000/num);
}

//declare the digits outside of display function so its not initialized every time it's called
//these bit patterns are designed so that digits[0] is the bit pattern on my display for 0
//digits[10] is a blank digit and digits[11] is a negative sign
static volatile int digits[12] = {0b00111111, 0b00000110, 0b10011011, 0b10001111, 0b10100110, 0b10101101, 0b10111101, 0b00000111, 0b10111111, 0b10100111, 0b00000000, 0b10000000};


void display(int num){
	//logic for getting first, second, and third digit and deciding what to put in the display
	int firstDigit = ((num - (num % 100)) / 100);
	if(firstDigit == 0){
		firstDigit = 10;
	}
	int num2 = num % 100;
	int secondDigit = ((num2 - (num2 % 10)) / 10);
	if(secondDigit == 0 & firstDigit == 10){
		secondDigit = 10;
	}
	num2 = num2 % 10;
	int thirdDigit = num2;

	//the loop for displaying digits. I had to make this loop small enough that each call to it didnt take forever, but long enough that the digits were still bright. 10 was a good number for this.. I did have 2500. 
	for(int i = 0;i < 25;i++){
		//I clear the outputs between setting the digits because the digits were leaking into eachother for some reason.
		P1OUT |= 0xE0;
		P2OUT &= BIT6;
		P1OUT &= ~(BIT5);
		P2OUT |= digits[firstDigit];
		P1OUT |= 0xE0;
		P2OUT &= BIT6;
		P1OUT &= ~(BIT6);
		P2OUT |= digits[secondDigit];
		P1OUT |= 0xE0;
		P2OUT &= BIT6;
		P1OUT &= ~(BIT7);
		P2OUT |= digits[thirdDigit];
		P1OUT |= 0xE0;
		P2OUT &= BIT6;		
	}
	return;
}

//this does a thing depending on how many numbers youve gotten
void playVictory(int num){
	switch(num){
		case 1:
			P2SEL |= BIT6;
			P1OUT |= (BIT0);
			setFrequency(500);
			__delay_cycles(30000);
			P1OUT &= ~(BIT0 + BIT4);
			P2SEL &= ~BIT6;
			break;
		case 2:
			P2SEL |= BIT6;
			P1OUT |= (BIT4);
			setFrequency(750);
			__delay_cycles(30000);
			P1OUT &= ~(BIT0 + BIT4);
			P2SEL &= ~BIT6;
			break;
		case 3:			
			cio_print((char *) "\n\r************\n\rWinner!!!!\n\r************\n\r");
			primeFlag = 0; tensFlag = 0; twoFlag = 0;
			P2SEL |= BIT6;
			P1OUT |= (BIT0);
			setFrequency(1000);
			__delay_cycles(300000);
			P1OUT &= ~(BIT0);		
			P1OUT |= (BIT4);
			setFrequency(2000);
			__delay_cycles(300000);
			P1OUT |= (BIT0);
			setFrequency(3000);
			__delay_cycles(300000);
			P1OUT &= ~(BIT0 + BIT4);
			P2SEL &= ~BIT6;
			__delay_cycles(1500000);
			if(divisor <= 8){
				cio_print("\n\rNow double speed!\n\r# ");
				divisor *= 2;
				counter = 0;
			} else {
				cio_print("\n\rHighest speed reached!\n\r# ");
			}
			break;
		default:
			break;
	}
}
			
//logic for button interrupt
#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR (void){
	int num = currentNumber; //get number as a local variable just incase it changes
	int flag = 0; //flag to see if anything changed
	
	//prime number logic. Check all numbers below square root. faster than checking all numbers
	for(int i = 1; i <= sqrt(num) && !primeFlag; i++){
		if(num % i == 0 && i != 1){
			break;
 		} else if(i == floor(sqrt(num)) || num < 4){
			primeFlag = 1; 
			flag = 1;
			cio_printf("\n\rYou got a prime: %i \n\r# ", num);
			break;
		}
	}
	int temp = num; //temp var to play around with

	//logic for checking power of two. if its even and you keep dividing by two until its two, then its a power of two. otherwise its not.
	while(temp % 2 == 0 && temp > 0  && !twoFlag){
		if(temp == 2){
			twoFlag = 1; 
			flag = 1;
			cio_printf("\n\rYou got a power of two: %i \n\r# ", num);
		}				
		temp = temp / 2;
	}
	//logic for multiple of ten. easiest one.
	if(num % 10 == 0 && !tensFlag){
		tensFlag = 1; 
		flag = 1;
		cio_printf("\n\rYou got a multiple of 10: %i\n\r# ", num);
	}
	//if you got any of the numbers on this interrupt, play a victory based on the total number of stuff you have now.
	if(flag){
		playVictory(twoFlag + tensFlag + primeFlag);
	}
	//debouncing stuff. I dont even know if I need this anymore
	while(!(BIT3 & P1IN)){}
	__delay_cycles(32000);
	P1IFG &= ~(BIT3);
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR (void)
{	
	//each time this interrupt happens..	
	currentNumber = floor(counter/(100/divisor)); //set the current number to the floor of the counter divided by (100 divided by the divisor)
	//basically this means the number on the display goes up every 100 times the interrupt happens. the divisor is there is to change how often the display goes up. if the divisor is 2, the display goes up every 50 times, 4 every 25 times, etc
	display(currentNumber);//display the number
	counter++;//increment the counter
	if(counter == (100000/divisor)){//if the counter is too high, set it back to 0
		counter = 0;
	}
}

