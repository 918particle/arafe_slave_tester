/*
 * Test program using an MSP430 LaunchPad for testing an ARAFE quad
 * through a serial port.
 * Updates
 * 			9/6/2016: Patrick fixed overflowed TX buffer
 *
 * main.c
 */
#include <msp430.h> 
#include "usci_uart.h"
#include "cli.h"
#include "arafe.h"
#include "timer_uart.h"

event_t *const arafe_tx_event = &timer_uart_tx_event;

// Pin usage for the slave tester.
// P1.0: capture+ (CA0)
// P1.1: USCI_A0 RX
// P1.2: USCI_A0 TX
// P1.3: capture- (CA3)
// That's all we need.

void event_ignore() {}



void usci_rx_char() {
	uint8_t num_chars;
	event_clear(&usci_uart_rx_event);

	num_chars = usci_uart_rx_fifo_count();
	for (;num_chars;num_chars--) {
		cli_process_char(usci_uart_rx());
	}
}

unsigned char sent_already = 0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;
    // Divide DCO by 8 to get SMCLK (1 MHz).
    BCSCTL2 = DIVS_3;
    P1SEL |= (BIT4);

    // Transmit events are ignored and disabled.
    event_handler_init(&usci_uart_tx_event, event_ignore, false);
    // Rx events trigger the receive character handling.
    event_handler_init(&usci_uart_rx_event, usci_rx_char, true);
    // Timer UART transmit events loop through the TX state machine handler.
    event_handler_init(&timer_uart_tx_event, arafe_tx_handler, true);
    timer_uart_init();
    usci_uart_init();
    usci_uart_irq_control(true);
    __enable_interrupt();


	usci_uart_puts("ARAFE Slave Tester.\r\n");
	cli_print_prompt();
	while (1) {
		event_process();
		usci_uart_tx_process();
		event_sleep();
    }
}
