/*   PWM period = Tcy * prescale * PTPER = 0.034us * prescaler * PTPER
     PWM pulse width = (Tcy/2) * prescale * PDCx
     POSCNT increasing CW
     POSCNT decreasing CCW
     RB0 = 1 => CW
     RB0 = 0 => CCW
     Encoder has 1200 ppr => 4800 pulse on 2 phase A & B in one rev	
*/

#include <xc.h>
#define FCY 29491200 	//7.3728Mhz XTal
#include <libpic30.h>	//for delay, uart,eeprom data manipulation
#include <stdio.h> 	//for printf()
#define baud 115200
#define FPWM 20000
#define PWM_prescaler 1
#define PWM_PTPER FCY/(FPWM*PWM_prescaler)
#define PWM_duty 0.3
#define PWM_PDC (2*FCY/FPWM)*(PWM_duty/PWM_prescaler)
#define Kp 7
#define Ki 3
#define Kd 3
#define set_point 0// simulate second tick on a clock.
#define T1INT 1	//=1: Enable T1 Interrupt. =0: Disable
// Configuration settings
// Xtal=7.3728MHz, PLL=16 => Fcy~30MIPS
// No watchdog, No Brown-out reset
// No Code protection
_FOSC( 0x8307);
_FWDT( 0x003F);
_FBORPOR(PWRT_OFF & BORV45 & PBOR_OFF & MCLR_EN & PWMxH_ACT_HI & RST_PWMPIN);
_FGS( GWRP_OFF & CODE_PROT_OFF);
int __C30_UART=2;
int count;
int e, e_sum, e_pre,delta_e;
// Function prototypes
void setup();
unsigned int read_analog_channel(int n);

void __attribute__((interrupt, __auto_psv__)) _T1Interrupt(void)
{

    	// Clear Timer 1 interrupt flag
    	float I;
    	_T1IF = 0;
    	I=((float)read_analog_channel(2)-511)*9.76;
    	printf("%0.2f | %d\n",I,POSCNT);
    	/*if(count==450)//1 second has passed.
    	{
    		POSCNT= 0;
    		e_sum=0;
    		e_pre=0;
    		e=0;
    		delta_e=0;
    		count=0;
    	}
    	else count++;*/    	
}

int main()
{ 	
    float PID;
    e = e_sum = e_pre = delta_e = 0;
    // Set up digital i/o, analog input, PWM, UART and interrupt
    setup();
    POSCNT=0;
	printf("Start!\n");
	while(1)
	{
		e = (int)POSCNT - set_point;
		e_sum += e;
		if (e_sum>50) e_sum=50;
		if (e_sum<-50) e_sum=-50;
		delta_e = e - e_pre;
		e_pre = e;
		PID = Kp * e + Ki * e_sum + Kd * delta_e;
		if(PID>0)
		{
			if (PID>PWM_PDC) PDC1=PWM_PDC;
			else PDC1=(unsigned int)PID;
			_LATB0=0;
		}
		else
		{
			if (PID<(PWM_PDC * (-1))) PDC1=PWM_PDC;
			else PDC1=(unsigned int)(PID*(-1));
			_LATB0=1;
		}
		__delay_ms(9);
	}
    return 0;
}
 
// This function sets up digital i/o, analog input,
// PWM, UART and timer interrupt.
void setup()
{
 /*	PORTS	*/
	ADPCFG=0xFB; 	//Use RB4 RB5 as digital input for
			//QEI module
	TRISBbits.TRISB0=0; //Pin RB0 = Motor Direction
	TRISBbits.TRISB2=1; 
	TRISE=0;
/* 	QEI Module	*/
	QEICON=	0x0740;//Use 4x mode
	DFLTCON=0x0060;//Use digital filter
	
    // Configure AN0-AN8 as analog inputs
    ADCON3bits.ADCS = 15;  // Tad = 266ns, conversion time is 12*Tad
    ADCON1bits.ADON = 1;   // Turn ADC ON
 
    // Configure PWM for free running mode
    //
    //   PWM period = Tcy * prescale * PTPER = 0.034us * prescaler * PTPER
    //   PWM pulse width = (Tcy/2) * prescale * PDCx
    //
    PWMCON1 = 0x00FF;     // Enable all PWM pairs in complementary mode
    PTCONbits.PTCKPS = 0; // prescale=1:1 (0=1:1, 1=1:4, 2=1:16, 3=1:64)
    PTPER = PWM_PTPER;  
    PDC1 = 0;          // 1.5ms pulse width on PWM channel 1
    PDC2 = 0;          // 1.5ms pulse width on PWM channel 2
    PDC3 = 0;          // 1.5ms pulse width on PWM channel 3
    PTMR = 0;             // Clear 15-bit PWM timer counter
    PTCONbits.PTEN = 1;   // Enable PWM time base
 
    // Setup UART
	U2BRG = (FCY/(baud*16))-1;            // 38400 baud @ 30 MIPS
	IEC1bits.U2RXIE = 1; //Enable Interrupt on Receive
	IPC6bits.U2RXIP=0b111;
	U2STAbits.URXISEL=0b11; //Interrupt everytime buffer is full = 4 characters
	U2MODEbits.UARTEN = 1; // Enable UART
 
    // Configure Timer 1
    // In this example, I'm setting PR1 and TCKPS for 8Hz
    PR1 = 34560;          // Set the Timer 1 period (max 65535)
    TMR1 = 0;             // Reset Timer 1 counter
    IEC0bits.T1IE = T1INT;    // Enable Timer 1 interrupt
    T1CONbits.TCKPS = 3;  // Prescaler (0=1:1, 1=1:8, 2=1:64, 3=1:256)
    T1CONbits.TON = 1;    // Turn on Timer 1
}
 
// This function reads a single sample from the specified
// analog input. It should take less than 5us when the
// microcontroller is running at 30 MIPS.
// The dsPIC30F4011 has a 10-bit ADC, so the value
// returned is between 0 and 1023 inclusive.
unsigned int read_analog_channel(int channel)
{
    ADCHS = channel;          // Select the requested channel
    ADCON1bits.SAMP = 1;      // Start sampling
    __delay32(30);            // 1us delay @ 30 MIPS
    ADCON1bits.SAMP = 0;      // Start Converting
    while (!ADCON1bits.DONE); // Should take 12 * Tad = 3.2us
    return ADCBUF0;
}
