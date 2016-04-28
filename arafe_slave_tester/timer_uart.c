/*
 * timer_uart.c
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 */

#define TIMER_UART_C
#include "timer_uart.h"

#pragma vector=TIMER1_A1_VECTOR
__interrupt void timeout_isr(void) {
	TA1CCTL2 &= ~CCIFG; // Clear the register flag, else it will never exit this loop thing
	timeout_FLAG = 1; // Set flag
	__bic_SR_register_on_exit(LPM0_bits); // Wake up
}
