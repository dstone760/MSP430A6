	This is for the MSP430G2553. The system is supposed to display numbers from 0 to 999 on the bubble display. 
	The user is supposed to press the interrupt button when theres a power of ten, a prime number, or a multiple of 10. When the user gets all three, the user has won. 
	Lights and sounds go off, there is much rejoicing, etc.
	
	Hardware: MSP430G2553, breadboard, RGB LED, speaker, bubble display, serial IO

	Software menu:
		Seven commands:
			Help – list all commands
			Reset – reset the MSP430
			Set – set the current number on the displayed
			Current – get the current number on the display
			Frequency – set the frequency of the speaker
			Speaker – toggle the speaker on and off
			Speed – set the speed at which the numbers change

	Wiring:
		RGB:
			Blue lead: P1.0
			Green lead: P1.4
			Ground lead: GND
		Speaker:
			+ lead: P2.6
			- lead: GND
		Bubble Display:
			PIN.NO  	FUNCTION		MSP430 PIN
		
			1		CATHODE 1		UNUSED
			2		ANODE E		    	P2.4
			3		ANODE C		    	P2.2
			4		CATHODE 3		P1.6
			5		ANODE DP		UNUSED
			6		CATHODE 4		P1.7
			7		ANODE G		    	P2.6
			8		ANODE D		    	P2.3
			9		ANODE F		    	P2.5
			10		CATHODE 2		P1.5
			11		ANODE B		    	P2.1
			12		ANODE A		    	P2.0
