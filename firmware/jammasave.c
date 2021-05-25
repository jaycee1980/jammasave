/*******************************************************************************************************************//**
	CCH JAMMA cabinet power saver
	\author Peter Mulholland
***********************************************************************************************************************/

// avr-gcc -mmcu=attiny13a -Os -std=c99 jammasave.c -o jammasave.elf
// avrdude -p t13 -b 19200 -c usbasp-clone -P com4 -v -U flash:w:jammasave.elf -U hfuse:w:0xF8:m -U lfuse:w:0x71:m

#define F_CPU 4800000UL

#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/pgmspace.h>

//----------------------------------------------------------------------------------------------------------------------
// Fuse settings

FUSES = {
	.low = (FUSE_SPIEN & FUSE_SUT1 & FUSE_SUT0 & FUSE_CKSEL1),		// SPI programming, 4.8MHz internal clock, minimal reset delay
	.high = (FUSE_BODLEVEL1 & FUSE_BODLEVEL0 & FUSE_RSTDISBL)		// brownout at 4.3v, reset pin disabled
};

//----------------------------------------------------------------------------------------------------------------------
// Constants and Defines

// The timer ticks at 750KHz. A count of 150 means our ISR triggers every 5KHz
#define TIMER_COUNT		(256-150)

// So, 500 ticks = 1 second.
// In reality it's a little inaccurate because the 4.8MHz internal clock isn't that accurate.
#define TIMER_TICKS		500

//----------------------------------------------------------------------------------------------------------------------
// PROGMEM data

static const char c_credits[] __attribute__((used)) PROGMEM = "CCH JAMMA Power Saver by Peter =M= built " __DATE__ " " __TIME__;

//----------------------------------------------------------------------------------------------------------------------
// EEMEM data

//----------------------------------------------------------------------------------------------------------------------
// Global variables

uint32_t g_timer = 0;
uint32_t g_timeout = 0;

//----------------------------------------------------------------------------------------------------------------------
// Local Functions

void inline stop_timer()
{
	// Disable overflow timer
	TIMSK0 &= ~(1 << TOIE0);								// overflow interrupt off
	TIFR0  &= ~(1 << TOV0);									// clear timer overflow mask
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;
}

void start_timer()
{
	cli();

	// Stop timer in case it's running already
	stop_timer();

	// Reset our timer counter
	g_timer = g_timeout;

	// Enable overflow timer
	TCNT0 = TIMER_COUNT;
	TIMSK0 = (1 << TOIE0);									// overflow interrupt on
	TCCR0B = (1 << CS01) | (1 << CS00);						// 4.8MHz / 64 = ticks at 75KHz

	sei();
}

uint16_t read_adc()
{
	ADCSRA |= (1 << ADSC);									// Start conversion
	while (!(ADCSRA & (1 << ADIF)));						// Wait for it to complete
	ADCSRA |= (1 << ADIF);									// Clear completion bit

	return ADCW;
}

void read_timeout_setting()
{
	// Set up ADC with ADC1 (PB2) as the input
	DIDR0 = (1 << ADC1D);									// Disable digital IO on ADC1
	ADMUX = (1 << MUX1);									// Use Vcc as reference, right adjusted result, input from ADC1 (PB2)
	ADCSRB = 0;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS0);		// Enable ADC, no interrupt, CLK/32 (150KHz)
	_NOP();
	_NOP();

	// Do some dummy readings to allow ADC to settle
	for (uint8_t i = 0; i < 8; i++)
		read_adc();

	// Read ADC for timeout scale value (0 - 511)
	uint16_t tcnt = read_adc() >> 1;

	// Set timer to the timeout count - between 1/2 hour and 1 1/2 hour.
	//	g_timeout =  1800 + ((((3600 * 32) / 512) * (uint32_t) tcnt) / 32);
	g_timeout  = ((225UL * (uint32_t) tcnt) / 32UL ) + 1800UL;
	g_timeout *= (uint32_t) TIMER_TICKS;

	// We don't use the ADC any more, so turn it off
	ADCSRA = 0;
	PRR = (1 << PRADC);
}

//----------------------------------------------------------------------------------------------------------------------
// ISR's

ISR(TIM0_OVF_vect)
{
	// Tick tock (for debugging, so i can see the timer running on ze 'scope)
	PINB = (1 << PINB5);

	// If we go over the timeout count, turn the relay off and stop the timer
	// We then wait for a pin change interrupt to wake things up again

	if (--g_timer == 0)
	{
		stop_timer();
		PORTB &= ~(1 << PORTB4);
	}
	else
	{
		// Go for another round
		TCNT0 = TIMER_COUNT;
		TIFR0 &= ~(1 << TOV0);
	}
}

ISR(PCINT0_vect)
{
	// Reset the timer
	start_timer();

	// Turn the relay on if it isn't already
	PORTB |= (1 << PORTB4);
}

//----------------------------------------------------------------------------------------------------------------------
// Main

void main(void) __attribute__((noreturn));

void main(void)
{
	// Tell the watchdog to bugger off
	wdt_disable();

	// Enable pullup on PB1, set output ports low
	PORTB = (1 << PORTB1);

	// Set all pins as output except PB1-3 and which are inputs
	DDRB = (1 << DDB4) | (1 << DDB5);

	// Wait for port state to synchronise before reading it
	_NOP();
	_NOP();	// two are needed even though atmel says only one!

	// Check if the "disable" jumper connected to PB1 is present (PB1 will be pulled low)
	// If it is, we don't do anything else - just turn on the relay and go to sleep permanently

	if (!(PINB & (1 << PINB1)))
	{
		// Hmm, why two separate ops to PORTB ? GCC generates two SBI's rather than an IN/ORI/OUT this way!

		// Turn relay on
		PORTB |= (1 << PORTB4);

		// Set PB5 high to show we've entered "disable" mode
		PORTB |= (1 << PORTB5);

		// We never wake up again...
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	}
	else
	{
		// Set up pin change interrupt for PB3
		PCMSK = (1 << PCINT3);
		GIMSK = (1 << PCIE);

		// Read the ADC for our timeout setting
		read_timeout_setting();
		//g_timeout = 1800;

		// Turn relay on
		PORTB |= (1 << PORTB4);

		// Start timer (which enables interrupts)
		start_timer();

		// Set our sleep mode so timer and pin change interrupts work
		set_sleep_mode(SLEEP_MODE_IDLE);
	}

	// Interrupts do the rest, sit here and sleep
	for (;;)
	{
		sleep_enable();
		sleep_cpu();
	}
}
