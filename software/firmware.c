#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "bits.h"
#include "uart.c"

#define TIMING_ENABLE 0
#define TIMING_ENABLE_CT 8000
#define TIMING_READING 1
#define TIMING_READING_CT 2000

#define CLK_DDR DDRE
#define CLK_PIN PINE
#define CLK_P PE5

#define INDI_DDR DDRB
#define INDI_PORT PORTB
#define INDI_P PB7

#define RCVINDI_DDR DDRB
#define RCVINDI_PORT PORTB
#define RCVINDI_P PB4

#define CLKINDI_DDR DDRH
#define CLKINDI_PORT PORTH
#define CLKINDI_P PH6

#define DTA_DDR DDRB
#define DTA_PIN PINB
#define DTA_P PB6

#define TIMING_DDR DDRB
#define TIMING_PORT PORTB
#define TIMING_P PB5


// data structure as sent from my sidewinder device, 48 bits total
// this datatype is accessable via two ways: an long integer with bits in it and a struc
typedef union
{
	// the struct consisting of the various data-fields sent by the sidewinder device
	struct
	{
		unsigned int btn_fire:1;      // edge  1

		unsigned int btn_top_large:1; // edge  2
		unsigned int btn_top_up:1;    // edge  3
		unsigned int btn_top_down:1;  // edge  4

		unsigned int btn_a:1;         // edge  5
		unsigned int btn_b:1;         // edge  6
		unsigned int btn_c:1;         // edge  7
		unsigned int btn_d:1;         // edge  8
		
		unsigned int btn_shift:1;     // edge  9

		unsigned int x:10;           // edge 10-19
		unsigned int y:10;           // edge 20-29
		unsigned int m:7;             // edge 30-36
		unsigned int r:6;             // edge 37-42

		unsigned int head:4;          // edge 43-46

		unsigned int reserved:1;      // edge 47
		unsigned int parity:1;        // edge 48
	};

	// the int used to access the data at bitlevel
	uint8_t bytes[6];
} sw_data_t;

// empty instance of the struct to reset our data-variable
static const sw_data_t sw_data_empty;        // 6 bytes ram

// internal state of the interface
volatile uint8_t timer_state = TIMING_ENABLE; // 1 byte ram

// next awaited bit index
volatile uint8_t bitcnt = 0;                 // 1 byte ram

// currently received data
volatile sw_data_t dta = {};                 // 6 bytes ram

// next awaited bit index
volatile uint8_t is_data_ready = 0;             // 1 byte ram



// setup pins & ports for communicating with the sidewinder device
void setup_lines(void)
{
	// time-line as output & high
	SETBIT(TIMING_DDR, TIMING_P);
	SETBIT(TIMING_PORT, TIMING_P);

	// indicator lines as out
	SETBIT(INDI_DDR, INDI_P);
	SETBIT(RCVINDI_DDR, RCVINDI_P);
	SETBIT(CLKINDI_DDR, CLKINDI_P);

	// clock- & data-line as input
	CLEARBIT(CLK_DDR, CLK_P);
	CLEARBIT(DTA_DDR, DTA_P);

	// enable rising edge detection for INT5
	SETBITS(EICRB, BIT(ISC51) | BIT(ISC50));
}

void setup_timer(void)
{
	// prescaler to 8
	SETBITS(TCCR1B, BIT(CS11));

	// auto-clear the counter on output-compare match
	SETBIT(TCCR1B, WGM12);

	// set output-compare-value
	OCR1A = TIMING_ENABLE_CT;

	// enable output-compare interrupt (timer 0, compare A)
	SETBIT(TIMSK1, OCIE1A);
}

// evers 3ms (at 333Hz)
ISR(TIMER1_COMPA_vect)
{
	uint8_t sreg_tmp = SREG;
	cli();

	// execute an enable cycle (pull line low)
	if(timer_state == TIMING_ENABLE)
	{
		// set the time the timer should timing-line should stay low
		OCR1A = TIMING_READING_CT;

		// switch modes
		timer_state = TIMING_READING;

		// rewind to bit 0

		// disable external interrupt 5
		CLEARBIT(EIMSK, INT5);

		// pull timing line down
		CLEARBIT(TIMING_PORT, TIMING_P);
	}

	// with the act of releasing the line high again,
	// the device begins transmitting data
	else
	{
		// set the time the timer should timing-line should stay high
		OCR1A = TIMING_ENABLE_CT;

		// switch modes
		timer_state = TIMING_ENABLE;

		// clear data storage
		dta = sw_data_empty;

		CLEARBIT(RCVINDI_PORT, RCVINDI_P);
		bitcnt = 0;
		is_data_ready = 0;

		// clear interrupt flag
		SETBIT(EIFR, INTF5);

		// enable external interrupt 5
		SETBIT(EIMSK, INT5);

		// release timing line high again
		SETBIT(TIMING_PORT, TIMING_P);
	}

	// restore system state
	SREG = sreg_tmp;
}

void data_ready(void)
{
	SETBIT(RCVINDI_PORT, RCVINDI_P);

	if(dta.btn_fire)
		CLEARBIT(INDI_PORT, INDI_P);
	else
		SETBIT(INDI_PORT, INDI_P);
}

ISR(INT5_vect)
{
/*
	if(bitcnt > 48) return;
*/

	SETBIT(CLKINDI_PORT, CLKINDI_P);
	CLEARBIT(CLKINDI_PORT, CLKINDI_P);

	// this is slower -in fact it is too slow to keep up with the clock-
	// because of the (pricy) relative bitshift required to generate the
	// mask, as the avr only implements a 1-bit shift-operation.
	// so to create a mask shifted by 48 bits, the avr loops 48 times,
	// shifting the bit by one position.
	/*
	if(BITSET(DTA_PIN, DTA_P))
		SETBIT(dta.bits, bitcnt);
	*/


	if(BITSET(DTA_PIN, DTA_P))
	{
		uint8_t *byte = &dta.bytes[bitcnt / 8];

		switch(bitcnt % 8)
		{
			case 0: SETBIT(*byte, 0); break;
			case 1: SETBIT(*byte, 1); break;
			case 2: SETBIT(*byte, 2); break;
			case 3: SETBIT(*byte, 3); break;
			case 4: SETBIT(*byte, 4); break;
			case 5: SETBIT(*byte, 5); break;
			case 6: SETBIT(*byte, 6); break;
			case 7: SETBIT(*byte, 7); break;
		}

	}

	if(++bitcnt == 48)
	{
		data_ready();
		is_data_ready = 1;
	}
}



int __attribute__((OS_main))
main(void)
{
	// setup pins & ports for communicating with the sidewinder device
	setup_lines();

	// setup timer/compare module to trigger every 5ms (at 200Hz)
	setup_timer();

	// setup uart for serial communication
	uart_setup();
	uart_puts("ready");

	sei();

	while(1)
	{
		if(is_data_ready)
		{

			uint8_t sreg_tmp = SREG;
			cli();

			sw_data_t c_dta = dta;

			SREG = sreg_tmp;

			uart_puts("x=");
			uart_puts_uint16(c_dta.x);
			uart_puts(" y=");
			uart_puts_uint16(c_dta.y);
			uart_puts(" r=");
			uart_puts_uint8(c_dta.r);
			uart_puts(" m=");
			uart_puts_uint8(c_dta.m);
			uart_putc('\n');

		}
	}
	return 0;
}
