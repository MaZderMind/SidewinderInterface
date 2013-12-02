#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "bits.h"
#include "callbacks.h"

#include "ks0108.c"
#include "uart.c"
#include "sidewinder.c"

#define INDI_DDR DDRB
#define INDI_PORT PORTB
#define INDI_P PB7

volatile uint8_t is_data_valid = 0;

void sw_data_is_now_invalid(void)
{
	is_data_valid = 0;
}

void sw_data_is_now_valid(void)
{
	if(sw_dta.btn_fire)
		CLEARBIT(INDI_PORT, INDI_P);
	else
		SETBIT(INDI_PORT, INDI_P);

	is_data_valid = 1;
}





int __attribute__((OS_main))
main(void)
{
	// setup sidewinder device communication
	sw_setup();

	// initialize display
	ks0108Init(0);
	ks0108DrawRect(0, 0, 63, 63, BLACK);
	ks0108DrawRect(65, 0, 5, 63, BLACK);


	// led connected to that indicator as output
	SETBIT(INDI_DDR, INDI_P);

	// enable interrupts
	sei();

	while(1)
	{
		if(is_data_valid)
		{
			uint8_t sreg_tmp = SREG;
			cli();

			sw_data_t c_dta = sw_dta;

			SREG = sreg_tmp;

			ks0108FillRect(
				1,
				1,
				61, 61,
				WHITE
			);

			uint8_t
				x = (((1024 - (uint16_t)c_dta.x) * 57) / 1024) + 1,
				y = (((1024 - (uint16_t)c_dta.y) * 57) / 1024) + 1;

			ks0108FillRect(
				x, y,
				4, 4,
				BLACK
			);

			uint8_t
				m = (((128 - (uint16_t)c_dta.m) * 61) / 128);

			if(m < 62)
			{
				ks0108FillRect(
					66,   // x
					m+1,  // y
					3,    // w
					61-m, // h
					WHITE
				);
			}

			if(m > 0)
			{
				ks0108FillRect(
					66,   // x
					1,    // y
					3,    // w
					m,    // h
					BLACK
				);
			}
		}
	}
	return 0;
}
