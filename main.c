
#define F_CPU 8000000UL

#include <avr/io.h> 
#include <util/delay.h>
#include <avr/interrupt.h>

#define timer2_stop (TCCR2 = 0b00000000)
#define ADC_stop (ADCSRA = 0b00000000)
#define adc_step 0.004882f
#define ADC4 (1 << MUX2)
#define ADC1 (1 << MUX0)
#define I 0
#define V 1

//bit patterns for all digits 0-9
const unsigned char pattern_digits[10] = {
	
	0b11000000,
	0b11111001,
	0b10100100,
	0b10110000,
	0b10011001,
	0b10010010,
	0b10000010,
	0b11111000,
	0b10000000,
	0b10010000
};

//bit patterns for anodes
const unsigned char anodes[4] = {
	
	0b00000010,
	0b00000100,
	0b00010000,
	0b00100000
};

/*array contains digits to be drawn 
  used by patter_digits array */
unsigned char digits[4];

//global iterators
uint8_t i = 0;
uint8_t imin = 0;
uint8_t imax = 4;
uint8_t j = 0;
uint8_t jmin = 0;
uint8_t jmax = 40;

//global variables
float rms = 0;
uint16_t rawADC[40];
char buffer[10];
uint16_t power = 0;
volatile char samplingDone = 0;

/* converts passed number into digits and
   stores them in digits array for drawing */   
void setDigits(uint16_t number)
{
	digits[0] = number / 1000 % 10;
	digits[1] = (number / 100 % 10) - 1;
	digits[2] = (number / 10 % 10) - 1;
	digits[3] = number % 10;
	
	
	if (!(digits[0]))
	{
		if (!(digits[1]))
		{
			if (!(digits[2]))
			{
				imin = 3;
				imax = 4;
				i = imin;
			} 
			else
			{
				imin = 2;
				imax = 4;
				i = imin;
			}
		} 
		else
		{
			imin = 1;
			imax = 4;
			i = imin;
		}
	}
	else
	{
		imin = 0;
		imax = 4;
		i = imin;
	}
}

void initTimer2()
{
		TCCR2 |= 0 << CS22 | 1 << CS21 | 1 << CS20;
		TCCR2 |= 1 << WGM21 | 0 << WGM20;
		OCR2 = 124;
		TIMSK |= 1 << OCIE2;
}

void initADC(unsigned char ADCchannel)
{
	ADMUX = 0b00000000;
	ADMUX |= 0 << REFS1 | 1 << REFS0;
	ADMUX |= ADCchannel;
	ADCSRA |= 1 << ADEN;
}

float getRms(unsigned char mode)
{
	switch(mode)
	{
		case 0: initADC(ADC4); break;
		case 1: initADC(ADC1); break;
	}
	initTimer2();
	while (!samplingDone);
	timer2_stop;
	ADC_stop;
	float mean = 0.0, sum = 0.0;
	for (unsigned char v = 0; v < jmax; v++)
	{
		if (rawADC[v] < 511){} 
		else
		{
			sum += pow((rawADC[v] - 511) * adc_step, 2);
		}
	}
	mean = sum / 20;
	switch(mode)
	{
		case 0: rms = sqrt(mean) * 5; break;
		case 1: rms = sqrt(mean) * 200; break;
	}
	samplingDone = 0;
	
	return rms;
}

void init7seg()
{
	/*initializing PORTD and PORTC for 7seg
	PORTD for cathodes, PORTC for anodes*/
	DDRD = 0xff;
	DDRB |= 1 << DDB1 | 1 << DDB2 | 1 << DDB4 | 1 << DDB5;
	
	/*initializing timer0
	multiplexing frequency ~= 488Hz*/
	TCCR0 |= 1 << CS02 | 0 << CS01 | 0 << CS00;
	TIMSK |= 1 << TOIE0;
	sei(); //enable global interrupts
	setDigits(1234);
}

int main()
{
	init7seg();
	//main loop
	while (1)
	{
		//power = getRms(V);
		//setDigits(power);
		//_delay_ms(1000);
	}
}

//Timer0 overflow interrupt ~=488Hz
ISR(TIMER0_OVF_vect)
{
	PORTB = anodes[i];
	PORTD = pattern_digits[digits[i]];
	i++;
	if (i >= imax)
	{
		i = imin;
	}
}

/*Timer2 compare match interrupt
  aka sampling frequency =1kHz*/
ISR(TIMER2_COMP_vect)
{
	//start one time conversion
	ADCSRA |= (1<<ADSC);
	//loop until conversion's done
	while(ADCSRA & (1<<ADSC));
	rawADC[j] = ADC;
	j++;
	if (j >= jmax)
	{
		j = 0;
		samplingDone = 1;
	}
}

