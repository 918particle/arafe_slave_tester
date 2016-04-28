/*
 * timer_uart.h
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 *
 * Updates:
 * 			1: 06/17/2015 - Increased TA1CCR2 to 50000, in order to accomodate for the long time to implement flash
 *
 */

#ifndef ARAFE_SLAVE_TESTER_TYLER_TIMER_UART_H_
#define ARAFE_SLAVE_TESTER_TYLER_TIMER_UART_H_

#include <stdint.h>
#include "event2.h"
#include "usci_uart.h"
#include <msp430.h>

#ifndef TIMER_UART_C
extern int timeout_FLAG;
#else
#pragma NOINIT(timeout_FLAG) // Don't initialize to a value to anything on startup
int timeout_FLAG;
#endif


#ifndef TIMER_UART_C
extern event_t timer_uart_tx_event;
extern event_t timer_uart_rx_event;
extern uint16_t tx_char_pending;
extern uint8_t rxc;
extern volatile uint8_t rx_ok;
#else
#pragma NOINIT(timer_uart_tx_event)
event_t timer_uart_tx_event;
#pragma NOINIT(timer_uart_rx_event)
event_t timer_uart_rx_event;
#pragma NOINIT(tx_char_pending)
uint16_t tx_char_pending;
#pragma NOINIT(timer1_isr_next_step)
uint16_t timer1_isr_next_step;
#pragma NOINIT(rx_bits)
uint16_t rx_bits;
#pragma NOINIT(rxc)
uint8_t rxc;
volatile uint8_t rx_ok = 0;
#endif

#define COND_TIMER1_ISR_NEXT_STEP	&timer1_isr_next_step
#define COND_RX_BITS				&rx_bits

static inline void reset_timeout_timer () {
	// Clear everything
	TA1CCR2 = 0;
	TA1CTL = 0;
	TA1CCTL2 = 0;
}

// 104 us between bits. This should be exact.
#define UART_TBIT 104
// Time from comparator setting up time to middle of first bit.
#define UART_FIRST_BIT 147

static inline void tx_char(unsigned char tx) {
	// Stop the counter, and clear it.
	TA0CTL = 0;
	TA0CCR0 = UART_TBIT;
	TA0CCTL0 = CCIE;
	// now set it off running.
	TA0CTL = TASSEL_2 + ID_0 + MC_1 + TACLR;
	// Upshift by 1 (the start bit is 0), plus
	// add the stop bit (bit 10).
	tx_char_pending = (tx << 1) | (0x200);
}

static inline uint8_t rx_char() {
	timeout_FLAG = 0;
	// Clear receive character interrupt
	TACCTL1 &= ~CCIFG;
	// Don't need to clear cur_char, each bit gets set/cleared deterministically.
	CACTL1 = CAON + CAIES + CAIE;

	// Use Timer 1, Control Register 2
	// Initialize timer for timeout
	TA1CCR2 = 50000; // Count to 44.0 ms and then timeout (running on 1MHz clock). (Most commands get done w/n 22 ms, but flash needs longer)
	TA1CTL = TASSEL_2 + ID_0 + MC_2 + TACLR; // Start Timer A1 Control, counting up to 0xFFFFFh continuously
	TA1CCTL2 = CCIE; // Enable timer interrupt

	LPM0; // Go to sleep

	// After wake-up
	CACTL1 &= ~CAIE; // Only want to disable the comparator interrupt.  If we stop it completely, it will never wake up ever
	if ( rx_ok == 1) { // If something on the comms....
		rx_ok = 0;
		reset_timeout_timer();
		return rxc; // Return comms character
	} else {
		usci_uart_printf("Timeout Reached: Bad or no response from slave.\r\n");
		reset_timeout_timer();
		return 0;
	}
}

static inline void timer_uart_init() {
    CAPD = CAPD0 + CAPD3;
    CACTL2 = P2CA0 + P2CA2 + P2CA1;
    CACTL1 = CAON;
}

#endif /* ARAFE_SLAVE_TESTER_TYLER_TIMER_UART_H_ */
