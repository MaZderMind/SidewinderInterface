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


#define XY_SZ 5


int __attribute__((OS_main))
main(void)
{
	// setup sidewinder device communication
	sw_setup();

	// initialize display
	ks0108Init(0);
	_delay_us(500);
	ks0108DrawRect(0, 0, 63, 63, BLACK);
	ks0108DrawRect(65, 0, 5, 63, BLACK);
	ks0108DrawRect(72, 0, 32, 5, BLACK);

	ks0108DrawRect(72, 0, 32, 5, BLACK);


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

			// x & y display coords
			uint8_t
				x = (((1024 - (uint16_t)c_dta.x) * (62-XY_SZ)) / 1024) + 1,
				y = (((1024 - (uint16_t)c_dta.y) * (62-XY_SZ)) / 1024) + 1;

			// clear right half of screen
			if(x > 1)
				ks0108FillRect(1, 1, x-2, 61, WHITE);

			// clear left half of screen
			if(x < 58)
				ks0108FillRect(x + XY_SZ, 1, 62 - x - XY_SZ, 61, WHITE);

			// clear part below the dot
			if(y > 1)
				ks0108FillRect(x, 1, XY_SZ - 1, y - 2, WHITE);

			// clear part above the dot
			if(y < 58)
				ks0108FillRect(x, y + XY_SZ, XY_SZ - 1, 57 - y, WHITE);


			// paint the dot
			ks0108FillRect(
				x, y,
				XY_SZ-1, XY_SZ-1,
				BLACK
			);





			// movable-member
			uint8_t
				m = (((128 - (uint16_t)c_dta.m) * 62) / 128);

			// clear movable-member bar
			if(m < 62)
				ks0108FillRect(66, m + 1, 3, 61 - m, WHITE);

			// fill movable-member bar
			if(m > 0)
				ks0108FillRect(66, 1, 3, m - 1, BLACK);





			// rotation
			uint8_t
				r = ((((uint16_t)c_dta.r) * 31) / 64);
_delay_ms(1000);
			// clear rotation bar
			ks0108FillRect(73, 1, 30, 3, WHITE);
_delay_ms(1000);
			// fill movable-member bar
			if(r > 15)
			{
				ks0108FillRect(73, 1, 73+r, 3, BLACK);
				_delay_ms(1000);
				ks0108FillRect(73+15, 1, r-15, 3, BLACK);
			}
			else
				ks0108FillRect(73+r, 1, 15-r, 3, BLACK);
		}
	}
	return 0;
}
