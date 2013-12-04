#define SW_TIMING_ENABLE 0
#define SW_TIMING_ENABLE_CT 8000
#define SW_TIMING_READING 1
#define SW_TIMING_READING_CT 2000

// the RCVINDI-Pin goes low when the MCU starts capturing a new Packet.
// It goes high when a complete Packet has been received
// it can be used to connect a LED which glows when a successfull data
// connection has been established. Probe it with a scope or a Frequency
// Counter to get an indication of the data update rate
#define SW_RCVINDI_DDR DDRB
#define SW_RCVINDI_PORT PORTB
#define SW_RCVINDI_P PB4

// the CLKINDI-Pin pulses whenever the MCU perceives a clock pulse from
// the Joystick. Probe it with an oscilloscope together with the Clock-Signal
// coming from the gameport to see, if your Interrupt-Code is fast enough to
// cope with the Clock rate
#define SW_CLKINDI_DDR DDRH
#define SW_CLKINDI_PORT PORTH
#define SW_CLKINDI_P PH6

// The Pin to which the Clock-Line (SUBD-Pin 2) from the Joystick is connected to
// Attention: The Code blow assumes this to be the INT5-Line. You need to
//   connect this Signal to one of the Pins capable of triggering external
//   interrupts and change all Lines denoted with INT5 to whichever INTn-Pin
//   you used
#define SW_CLK_DDR DDRE
#define SW_CLK_PIN PINE
#define SW_CLK_P PE5

// The Pin to which the Data-Line (SUBD-Pin 7) from the Joystick is connected to
// Can be any GPIO-Pin
#define SW_DTA_DDR DDRB
#define SW_DTA_PIN PINB
#define SW_DTA_P PB6

// The Pin on which the MCU sends its Trigger-Signal (SUBD-Pin 3)
// Can also be any GPIO-Pin
#define SW_TIMING_DDR DDRB
#define SW_TIMING_PORT PORTB
#define SW_TIMING_P PB5

// data structure as sent from my sidewinder device, 48 bits total
// this datatype is accessable via two ways: an integer-array and a struct
// the former is used to used to manipulate the data at a bit-level
// while the latter is used to access individual pieces of data
typedef union
{
	// the struct consisting of the various data-fields sent by the sidewinder device
	struct
	{
		unsigned int btn_fire:1;      // bit  1

		unsigned int btn_top:1;       // bit  2
		unsigned int btn_top_up:1;    // bit  3
		unsigned int btn_top_down:1;  // bit  4

		unsigned int btn_a:1;         // bit  5
		unsigned int btn_b:1;         // bit  6
		unsigned int btn_c:1;         // bit  7
		unsigned int btn_d:1;         // bit  8
		
		unsigned int btn_shift:1;     // bit  9

		unsigned int x:10;            // bits 10-19
		unsigned int y:10;            // bits 20-29
		unsigned int m:7;             // bits 30-36
		unsigned int r:6;             // bits 37-42

		unsigned int head:4;          // bits 43-46

		unsigned int reserved:1;      // bit 47
		unsigned int parity:1;        // bit 48
	};

	// the ints are used to access the struct-data at bit-level
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

		// clear INT5 interrupt flag
		SETBIT(EIFR, INTF5);

		// enable external interrupt INT5
		SETBIT(EIMSK, INT5);

		// release timing line high again
		SETBIT(SW_TIMING_PORT, SW_TIMING_P);
	}

	// restore system state
	SREG = sreg_tmp;
}





// Interrupt-Handler for external Interrupt INT5
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
