#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "bits.h"
#include "callbacks.h"

#include "uart.c"
#include "sidewinder.c"

#define INDI_DDR DDRB
#define INDI_PORT PORTB
#define INDI_P PB7

uint8_t is_data_valid = 0;

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
