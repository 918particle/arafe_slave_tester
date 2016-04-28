/*
 * usci_uart.h
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 */

#ifndef ARAFE_SLAVE_TESTER_TYLER_USCI_UART_H_
#define ARAFE_SLAVE_TESTER_TYLER_USCI_UART_H_

#include <stdint.h>
#include <stdbool.h>
#include <msp430.h>
#include "event2.h"
#include "usci_uart_config.h"

#ifndef USCI_UART_C
extern event_t usci_uart_rx_event;
extern event_t usci_uart_tx_event;
extern event_t usci_uart_rx_event;

extern uint8_t usci_uart_rx_buffer[];
extern uint8_t usci_uart_rx_buffer_write_ptr;
extern uint8_t usci_uart_rx_buffer_read_ptr;

extern uint8_t usci_uart_tx_buffer[];
extern uint8_t usci_uart_tx_buffer_write_ptr;
extern uint8_t usci_uart_tx_buffer_read_ptr;
#else
#pragma NOINIT(usci_uart_rx_event)
event_t usci_uart_rx_event;
#pragma NOINIT(usci_uart_tx_event)
event_t usci_uart_tx_event;
#pragma NOINIT(usci_uart_rx_error_event)
event_t usci_uart_rx_error_event;

#pragma NOINIT(usci_uart_rx_buffer)
uint8_t usci_uart_rx_buffer[USCI_UART_RX_BUFFER_SIZE];
uint8_t usci_uart_rx_buffer_write_ptr = 0;
uint8_t usci_uart_rx_buffer_read_ptr = 0;

#pragma NOINIT(usci_uart_tx_buffer)
uint8_t usci_uart_tx_buffer[USCI_UART_TX_BUFFER_SIZE];
uint8_t usci_uart_tx_buffer_write_ptr = 0;
uint8_t usci_uart_tx_buffer_read_ptr = 0;

#endif

//< Initialize the USCI UART.
static inline void usci_uart_init() {
	USCI_UART_CTL1 = UCSWRST + USCI_UART_CLOCK_SOURCE;
	USCI_UART_BR0 = (USCI_UART_BR & 0xFF);
#if USCI_UART_BR > 255
	USCI_UART_BR1 = ((USCI_UART_BR >> 8) & 0xFF);
#endif
	USCI_UART_MCTL = (USCI_UART_UCOS16) + (USCI_UART_BRS << 1) + (USCI_UART_BRF << 4);
	USCI_UART_CTL1 &= ~UCSWRST;
	USCI_UART_PORT_INIT;
}

//< Enable or disable the receive IRQ.
static inline void usci_uart_irq_control(bool enable) {
	IE2 |= USCI_UART_RXIE;
}

//< Add a character to the transmit buffer.
static inline void usci_uart_tx(uint8_t txc) {
	usci_uart_tx_buffer[usci_uart_tx_buffer_write_ptr] = txc;
	usci_uart_tx_buffer_write_ptr = (usci_uart_tx_buffer_write_ptr + 1) & (USCI_UART_TX_BUFFER_SIZE-1);
}

//< Get a character from the receive buffer.
static inline uint8_t usci_uart_rx() {
	uint8_t tmp;
	tmp = usci_uart_rx_buffer[usci_uart_rx_buffer_read_ptr];
	usci_uart_rx_buffer_read_ptr = (usci_uart_rx_buffer_read_ptr + 1) & (USCI_UART_RX_BUFFER_SIZE-1);
	return tmp;
}

//< Get number of characters pending receive.
static inline uint8_t usci_uart_rx_fifo_count() {
	return (usci_uart_rx_buffer_write_ptr - usci_uart_rx_buffer_read_ptr) & (USCI_UART_RX_BUFFER_SIZE-1);
}

void usci_uart_tx_process();

#ifdef USCI_UART_PRINTF
void usci_uart_puts(const char *string);
void usci_uart_printf(char *format, ...);
#endif

#endif /* ARAFE_SLAVE_TESTER_TYLER_USCI_UART_H_ */
