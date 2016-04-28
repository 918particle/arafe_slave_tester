/*
 * usci_uart_config.h
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 */

#ifndef ARAFE_SLAVE_TESTER_TYLER_USCI_UART_CONFIG_H_
#define ARAFE_SLAVE_TESTER_TYLER_USCI_UART_CONFIG_H_

#include <msp430.h>

// Look these up in Table 15-4 of the MSP430x2xx guide.
#define USCI_UART_BR (104)
#define USCI_UART_BRS (1)
#define USCI_UART_BRF (0)
#define USCI_UART_UCOS16 (0)

// Clock source selection
#define USCI_UART_CLOCK_SOURCE (UCSSEL_2)

// Register defines.
#define USCI_UART_BR0 UCA0BR0
#define USCI_UART_BR1 UCA0BR1
#define USCI_UART_MCTL UCA0MCTL
#define USCI_UART_CTL1 UCA0CTL1
#define USCI_UART_RXBUF UCA0RXBUF
#define USCI_UART_RXIE UCA0RXIE
#define USCI_UART_RXIFG UCA0RXIFG
#define USCI_UART_TXBUF UCA0TXBUF
#define USCI_UART_TXIE UCA0TXIE
#define USCI_UART_TXIFG UCA0TXIFG

#define USCI_UART_PORT_INIT 	\
		P1SEL |= (BIT1 + BIT2); \
		P1SEL2 |= (BIT1 + BIT2)

// must be a power of 2
#define USCI_UART_TX_BUFFER_SIZE 64
// must be a power of 2
#define USCI_UART_RX_BUFFER_SIZE 8

#define USCI_UART_PRINTF

#endif /* ARAFE_SLAVE_TESTER_TYLER_USCI_UART_CONFIG_H_ */
