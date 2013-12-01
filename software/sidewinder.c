#define SW_TIMING_ENABLE 0
#define SW_TIMING_ENABLE_CT 8000
#define SW_TIMING_READING 1
#define SW_TIMING_READING_CT 2000

#define SW_RCVINDI_DDR DDRB
#define SW_RCVINDI_PORT PORTB
#define SW_RCVINDI_P PB4

#define SW_CLKINDI_DDR DDRH
#define SW_CLKINDI_PORT PORTH
#define SW_CLKINDI_P PH6

#define SW_CLK_DDR DDRE
#define SW_CLK_PIN PINE
#define SW_CLK_P PE5

#define SW_DTA_DDR DDRB
#define SW_DTA_PIN PINB
#define SW_DTA_P PB6

#define SW_TIMING_DDR DDRB
#define SW_TIMING_PORT PORTB
#define SW_TIMING_P PB5

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
volatile uint8_t sw_timer_state = SW_TIMING_ENABLE; // 1 byte ram

// next awaited bit index
volatile uint8_t sw_bitcnt = 0;                 // 1 byte ram

// currently valid data
volatile sw_data_t sw_dta = {};                 // 6 bytes ram





// setup pins & ports for communicating with the sidewinder device
void sw_setup_lines(void)
{
	// time-line as output & high
	SETBIT(SW_TIMING_DDR, SW_TIMING_P);
	SETBIT(SW_TIMING_PORT, SW_TIMING_P);

	// indicator lines as out
	SETBIT(SW_RCVINDI_DDR, SW_RCVINDI_P);
	SETBIT(SW_CLKINDI_DDR, SW_CLKINDI_P);


	// clock- & data-line as input
	CLEARBIT(SW_CLK_DDR, SW_CLK_P);
	CLEARBIT(SW_DTA_DDR, SW_DTA_P);

	// enable rising edge detection for INT5
	SETBITS(EICRB, BIT(ISC51) | BIT(ISC50));
}

// setup timer/compare module to trigger every 5ms (at 200Hz)
void sw_setup_timer(void)
{
	// prescaler to 8
	SETBITS(TCCR1B, BIT(CS11));

	// auto-clear the counter on output-compare match
	SETBIT(TCCR1B, WGM12);

	// set output-compare-value
	OCR1A = SW_TIMING_ENABLE_CT;

	// enable output-compare interrupt (timer 0, compare A)
	SETBIT(TIMSK1, OCIE1A);
}

void sw_setup(void)
{
	// setup pins & ports for communicating with the sidewinder device
	sw_setup_lines();

	// setup timer/compare module to trigger every 5ms (at 200Hz)
	sw_setup_timer();
}





// evers 3ms (at 333Hz)
ISR(TIMER1_COMPA_vect)
{
	uint8_t sreg_tmp = SREG;
	cli();

	// execute an enable cycle (pull line low)
	if(sw_timer_state == SW_TIMING_ENABLE)
	{
		// set the time the timer should timing-line should stay low
		OCR1A = SW_TIMING_READING_CT;

		// switch modes
		sw_timer_state = SW_TIMING_READING;

		// rewind to bit 0

		// disable external interrupt 5
		CLEARBIT(EIMSK, INT5);

		// pull timing line down
		CLEARBIT(SW_TIMING_PORT, SW_TIMING_P);
	}

	// with the act of releasing the line high again,
	// the device begins transmitting data
	else
	{
		// set the time the timer should timing-line should stay high
		OCR1A = SW_TIMING_ENABLE_CT;

		// switch modes
		sw_timer_state = SW_TIMING_ENABLE;

		// send a callback that the data will become invalid now
		sw_data_is_now_invalid();

		// show state on output port
		CLEARBIT(SW_RCVINDI_PORT, SW_RCVINDI_P);

		// clear data storage
		sw_dta = sw_data_empty;
		sw_bitcnt = 0;

		// clear interrupt flag
		SETBIT(EIFR, INTF5);

		// enable external interrupt 5
		SETBIT(EIMSK, INT5);

		// release timing line high again
		SETBIT(SW_TIMING_PORT, SW_TIMING_P);
	}

	// restore system state
	SREG = sreg_tmp;
}





ISR(INT5_vect)
{

	if(sw_bitcnt > 48) return;

	SETBIT(SW_CLKINDI_PORT, SW_CLKINDI_P);
	CLEARBIT(SW_CLKINDI_PORT, SW_CLKINDI_P);

	// this is slower -in fact it is too slow to keep up with the clock-
	// because of the (pricy) relative bitshift required to generate the
	// mask, as the avr only implements a 1-bit shift-operation.
	// so to create a mask shifted by 48 bits, the avr loops 48 times,
	// shifting the bit by one position.
	/*
	if(BITSET(SW_DTA_PIN, SW_DTA_P))
		SETBIT(sw_dta.bits, sw_bitcnt);
	*/


	if(BITSET(SW_DTA_PIN, SW_DTA_P))
	{
		volatile uint8_t *byte = &sw_dta.bytes[sw_bitcnt / 8];

		switch(sw_bitcnt % 8)
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

	if(++sw_bitcnt == 48)
	{
		// show state on output port
		SETBIT(SW_RCVINDI_PORT, SW_RCVINDI_P);

		// send a callback that the data is become valid now
		sw_data_is_now_valid();
	}
}
