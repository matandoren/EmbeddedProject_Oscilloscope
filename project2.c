#include <p32xxxx.h>

#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8

char FIRST_GRAPH=1;

char readSwitches();
void lightLEDS(char LEDS);
void busy(char choice);	//1- checks busy_flag of LCDT or LCDG; 2- checks display_flag of LCDG; 3- checks both;
void displayString(char origin, char rs, char *str);
void initPortB(void);
void displayPeak(short peak, char line);
void ReadVoltage(short *buffer, short size, short data[], char sampleRate); //data[0]: min; data[1]: max;
void plotPixel(char x, char y, char L_D, char changeX, char changeY);//x: 0-127; y: 0-63; L_D: 1/0: 1- lights a pixel. 0- darkens a pixel
																	 //changeX: 1/0: change X-page Yes/No
																	 //changeY: 1/0: change Y-address Yes/No
void plotGraph(short *buffer, short *prevBuffer, short size, char yScale, short data[]);//yScale: 0 - auto; 1 - x1, 2 - x2, 3 - x4, ...
																						//data[0]: min; data[1]: max;
void enable();
void initLcdg(char side);//Clears the LCDG DDRAM and turns on the display on the chosen side: side==0 => left; else => right;
void delayT1(char preScalar, unsigned short pr);

/**************************************** MAIN ***************************/
void main()
{
	short buffer[2][128], size=128, data[4];
	char bufferPage=0, prevSwitches;
	char control[7]={0x38,0x38,0x38,0x0e,0x06,0x1, 0};
	char minStr[]="Min     V";
	char maxStr[]="Max     V";
	char sampleRate;
	char yScale;
	unsigned char switches;
	
	initPortB();
	displayString(0, 0, control);// LCDT Wake-up
	initLcdg(0);
	initLcdg(1);
	displayString(0, 1, maxStr);
	displayString(0xc0-0x80, 1, minStr);

	while (1)
	{
		switches=readSwitches();
		lightLEDS(switches);
		if (((prevSwitches<<4)!=(switches<<4)) && !FIRST_GRAPH)
		{
				initLcdg(0);
				initLcdg(1);
		}
		sampleRate=switches>>4;
		yScale=((switches&0x08)>>3)*(-8)+(switches&0x07);
		ReadVoltage(&buffer[bufferPage][0], size, data, sampleRate);
		displayPeak(data[1], 0);
		displayPeak(data[0], 1);
		plotGraph(&buffer[bufferPage][0], &buffer[bufferPage^1][0], size, yScale, data);
		bufferPage^=1;
		delayT1(3, 0x716A); //0.75[sec] delay;
		prevSwitches=switches;
	}
}
/************************************************************************/

char readSwitches()
{
	short tmp;

	TRISF&=0xfff8;
	TRISE|=0x00ff;
	TRISD&=0xffcf;
    tmp=PORTF & 0xfff8;
	PORTF=tmp | 0x3;
	PORTDbits.RD4=1;
    PORTDbits.RD4=0;
	return PORTE;
}

void lightLEDS(char LEDS)
{
	short tmp;

	TRISF&=0xfff8;
	TRISE&=0xff00;
	TRISD&=0xffcf;
    tmp=PORTF & 0xfff8;
	PORTF=tmp | 0x4;
	tmp=PORTE &0xff00;
	PORTE=tmp | LEDS;
	PORTDbits.RD4=1;
    PORTDbits.RD4=0;
}

void busy(char choice)	//1- checks busy_flag of LCDT or LCDG; 2- checks display_flag of LCDG; 3- checks both;
{
    unsigned char tmp, input;
	
	tmp=PORTBbits.RB15;       
	TRISE=TRISE | 0xff;
    PORTDbits.RD5 = 1;//w/r
    PORTBbits.RB15 = 0;//rs 
	do
	{
		PORTDbits.RD4=1;//enable=1
		PORTDbits.RD4=0;//enable=0
		input=PORTE;
	} while(((choice & 1) && (input&0x80)) || ((choice & 2) && (input&0x10)));//בדיקה דגל*/
	PORTDbits.RD5=0; 
	PORTBbits.RB15=tmp;
	TRISE=TRISE & 0xff00; 
}

void enable()
{
	TRISD&=0xffef;
	PORTDbits.RD4=1;
	PORTDbits.RD4=0;
}

void displayString(char origin, char rs, char *str)
{
	TRISD=0;
	TRISF=0;
	TRISE=0;
	TRISB=0;
	PORTF=0;
	PORTDbits.RD5=0;
	if (rs)
	{
		PORTBbits.RB15 = 0;//rs
		PORTE= 0x80 + origin;
		PORTDbits.RD4=1;
    	PORTDbits.RD4=0;
		busy(1);
	}
	PORTBbits.RB15 = rs;//rs
	while (*str)
	{
		PORTE=*str;
		PORTDbits.RD4=1;
    	PORTDbits.RD4=0;
		busy(1);
		str++;
	}
}

void initPortB(void)
{              
	unsigned int portMap;
               
	portMap = TRISB;
	portMap &= 0xFFFF7FFF;
    portMap |= 0xF;
	TRISB = portMap;
	AD1PCFG |= 0x800f; //Select PORTB to be digital port input
	CNCONbits.ON = 0; //Change Notice Module On bit CN module is disabled
	CNEN = 0x3C;	
	CNPUE = 0x3C;  	//Set RB0 - RB3 as inputs with weak pull-up
	CNCONbits.ON = 1; //1 = CN module is enabled
}
	
void displayPeak(short peak, char line)
{
	int tmp;
	float num;
	char mes[5];
	mes[4]=0;
	num=peak;
	num=(num*3.3)/1023;
	tmp=(((int)(num*100))%10);
	mes[3]=tmp+'0'; 
	tmp=(((int)(num*10))%10);
    mes[2]=tmp+'0';
    mes[1]='.';
	tmp=(((int)num)%10);
    mes[0]=tmp+'0';
	displayString((0xc0-0x80)*line+4,1,mes);
}

void ReadVoltage(short *buffer, short size, short data[], char sampleRate)	//data[0]: min; data[1]: max;
{
	short j, ADCValue, i;

	AD1PCFG =0xFBFF; // PORTB = Digital; RB8 = analog
	AD1CON1 = 0x0000; // SAMP bit = 0 ends sampling ...
	// and starts converting
	AD1CHS=0x000A0000;// in this example RB8AN8is the input
	AD1CSSL = 0x400;//Select  RB8
	AD1CON3 = 0x0002; // Clock select
	AD1CON2 = 0;//Vref-not Scan-BUFM  Result –MUX  A
	AD1CON1SET = 0x8000; // turn ADC ON
	data[1]=0;
	data[0]=0x03ff;
	for (i=0; i<size; i++) // repeat continuously
	{
		TRISB|=0x400;
		AD1CON1SET = 0x0002; // start sampling ...
		for(j=0;j<5;j++);//DelayNmSec(100); // for 100 mS
		AD1CON1CLR = 0x0002; // start Converting
		while (!(AD1CON1 & 0x0001))// conversion done?
			if (sampleRate)
				delayT1(2, sampleRate);
		buffer[i] = ADC1BUF0; // yes then get ADC value
		if (buffer[i]>data[1])
			data[1]=buffer[i];
		if (buffer[i]<data[0])
			data[0]=buffer[i];
	}
}

void plotPixel(char x, char y, char L_D, char changeX, char changeY) //x: 0-127; y: 0-63; L_D: 1/0: 1- lights a pixel. 0- darkens a pixel
																	 //changeX: 1/0: change X-page Yes/No
																	 //changeY: 1/0: change Y-address Yes/No
{
	short tmp;

	TRISF&=0xfff8;
	TRISE&=0xff00;
	TRISB&=0x7fff;
	TRISD&=0xffcf;
	
	tmp=PORTF & 0xfff8;	//Chooses the LCDG side (left or right) depending on x
	if (x>63)
	{
		PORTF=tmp | 0x2;
		x-=64;
	}
	else
		PORTF=tmp | 0x1;

	PORTDbits.RD5=0;	//R/W
	if (changeX)
	{
		PORTBbits.RB15=0;	//RS
		tmp=PORTE & 0xff00;	//Sets the X-page to the appropriate row depending on y
		PORTE=(tmp | (0xb8|(7-(y>>3))));
		enable();
		busy(1);
	}
	if (changeY)
	{
		PORTBbits.RB15=0;	//RS
		tmp=PORTE & 0xff00;	//Sets the Y-address to the appropriate column depending on x
		PORTE=(tmp | (0x40 | x));
		enable();
		busy(1);
	}
	PORTBbits.RB15=1;	//RS
	tmp=PORTE & 0xff00;	// Lights the appropriate pixel in the 8-pixel stack depending on y or darkens the whole 8-pixel stack depending on L_D
	PORTE=tmp|(L_D * (0x80>>(y%8)));
	enable();
	busy(1);
}

void plotGraph(short *buffer, short *prevBuffer, short size, char yScale, short data[])	//yScale: 0 -> auto; 1 -> x1, 2 -> x2, 3 -> x4, ...
																						//yScale:-1 -> x0.5, -2 -> x0.25, ...
																						//data[0]: min; data[1]: max; data[2]: previous shift;
{
	unsigned short i, tmp, max, min;
	unsigned char cur, prev, changeY=1/*, leftX*/; 
	char shift;

	if (yScale==0)
		for (max=(data[1]<<6), min=data[0]<<6, shift=4; (!((max-min)&0x8000)) && max; max<<=1, min<<=1, shift--);
	else if (yScale>0)
		shift=5-yScale;
	else
		shift=4-yScale;
		

	for (i=0; i<size; i++)
	{
		if (shift<0)
			tmp=(buffer[i]-(data[0]*(!yScale)))<<(shift*(-1));
		else
			tmp=(buffer[i]-(data[0]*(!yScale)))>>shift;
		if (tmp>63)
			cur=63;
		else
			cur=tmp;
		if (data[2]<0)
			tmp=(prevBuffer[i]-(data[3]*(!yScale)))<<(data[2]*(-1));
		else
			tmp=(prevBuffer[i]-(data[3]*(!yScale)))>>data[2];
		if (tmp>63)
			prev=63;
		else
			prev=tmp;
		if (cur==prev && !FIRST_GRAPH)
		{
			changeY=1;
			continue;
		}
		plotPixel(i, cur, 1, /*(((cur>>3)!=leftX)||(!i))*/1, changeY);
		//leftX=cur>>3;
		if ((cur>>3)!=(prev>>3) && !FIRST_GRAPH)
		//{
			plotPixel(i, prev, 0, 1, 1); //x: 0-127; y: 0-63; L_D: 1/0: 1- lights a pixel. 0- darkens a pixel
										 //changeX: 1/0: change X-page Yes/No
										 //changeY: 1/0: change Y-address Yes/No
			/*leftX=prev>>3;
		}*/
		changeY=0;
	}
	data[2]=shift;
	data[3]=data[0];
	FIRST_GRAPH=0;
}

void initLcdg(char side)//Clears the LCDG DDRAM and turns on the display on the chosen side: side==0 => left; else => right;
{
	short tmp, i, j;

	TRISF&=0xfef8;
	TRISE&=0xff00;
	TRISB&=0x7fff;
	TRISD&=0xffcf;

	//	Initializes the left or right side of the LCDG depending on 'side': side==0 => left; else => right;
	tmp=PORTF & 0xfff8;	//Chooses the side of the LCDG on the decoder
	if (side)
		PORTF=tmp | 0x2;
	else
		PORTF=tmp | 0x1;
	PORTBbits.RB15=0;	//RS
	PORTDbits.RD5=0;	//R/W
	tmp=PORTE & 0xff00;	//Sets the X-page to 0
	PORTE=(tmp|0xb8);
	enable();
	busy(1);
	tmp=PORTE & 0xff00;	//Sets the Y-address to 0
	PORTE=(tmp|0x40);
	enable();
	busy(1);
	tmp=PORTE & 0xff00;	//Sets the Z-address to 0
	PORTE=(tmp|0xc0);
	enable();
	busy(1);
	for (j=0; j<8; j++)
	{
		PORTBbits.RB15=0;	//RS
		tmp=PORTE & 0xff00;	//Sets the X-page to the next row
		PORTE=(tmp|(0xb8 | j));
		enable();
		busy(1);
		PORTBbits.RB15=1;	//RS
		for (i=0; i<64; i++)	//Clears the entire DDRAM of the chosen side
		{
			PORTE&=0xff00;
			enable();
			busy(1);
		}
	}
	FIRST_GRAPH=1;
}

void delayT1(char preScalar, unsigned short pr)
{
	T1CONbits.ON=0;
	T1CONbits.TGATE=0;
	T1CONbits.TCS=0;//in clock
	T1CONbits.TCKPS0=preScalar&1;
	T1CONbits.TCKPS1=(preScalar&2)>>1;
	T1CONbits.TSYNC=1;
	TMR1=0;
	PR1=pr;
	IFS0bits.T1IF=0;
	T1CONbits.ON=1;
	while(!IFS0bits.T1IF);
}

