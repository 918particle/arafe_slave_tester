/*
 * arafe.c
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 *
 * Updates:
 *  		1: 06/17/2015 - Increased delay for state = LISTEN to 2500 cycles, to avoid 12EN ringing on comms line.
 */
#include "timer_uart.h"
#include "usci_uart.h"
#define ARAFE_C
#include "arafe.h"
#include <stdbool.h>


// Global variables
int dump_status = 0;
int m = 0; // For printing out outcome of dump

typedef enum arafe_state_t {
	STATE_PREAMBLE_0 = 1,
	STATE_PREAMBLE_1 = 2,
	STATE_PREAMBLE_2 = 3,
	STATE_COMMAND = 4,
	STATE_ARGUMENT = 5,
	STATE_ETX = 6,
	STATE_LISTEN = 7,
	STATE_IDLE = 0
} arafe_state_t;

static arafe_state_t state = STATE_IDLE;

bool arafe_busy() {
	return (state != STATE_IDLE);
}


void arafe_send_command() {	
	state = STATE_PREAMBLE_0;
	arafe_tx_handler();
}

void arafe_tx_handler() {
	switch (state) {
		case STATE_PREAMBLE_0:
		case STATE_PREAMBLE_2:
			tx_char('!');
			break;
		case STATE_PREAMBLE_1:
			tx_char('M');
			break;
		case STATE_COMMAND:
			tx_char(arafe_command);
			break;
		case STATE_ARGUMENT:
			tx_char(arafe_argument);
			break;
		case STATE_ETX:
			tx_char(0xFF);
			break;
		case STATE_LISTEN:
			__delay_cycles(2500); // When turning on control, there is crap on comms. Be sure to wait after 2.2 ms
			{
				// Receive preamble (0-2), ack bit (3), and end of transmission (4)
				uint8_t received_char[6];
				if ( dump_status == 1) {  //Receive all characters transmitted by slave and put into array
					received_char[0] = rx_char();
					if (timeout_FLAG == 1) break; // Yes, redundant, but we don't want anything getting through
					received_char[1] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[2] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[3] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[4] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[5] = rx_char();

					// If counter = 0, then print header to screen
					if ( m == 0) {
					usci_uart_printf("Dumping device_info_buffer[]: Beep boop"); //Remove the sass before final revision. Maybe.
					usci_uart_printf("\r\n");
					}

					usci_uart_printf("%x ", received_char[4]);
					if ( ( m == 3 || m == 7 || m == 11 || m == 15)) { // Make 4x4 matrix
						usci_uart_printf("\r\n");
					}

					m++; // Increment for times gone through dump recursion, used to keep track of placing of values in 4x4 matrix


				} else { // For all other commands. Ack back the command received from slave to ensure the user can double check what he sent in is what the board actually did
					received_char[0] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[1] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[2] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[3] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[4] = rx_char();
					if (timeout_FLAG == 1) break;
					received_char[5] = rx_char();
					if (timeout_FLAG == 1) break;

					usci_uart_printf("Acknowledged Command of Type: ");

					if (received_char[3] & 0x80) usci_uart_printf("write %i\r\n\r\n", (received_char[3] & 0xF));
					else if (received_char[3] & 0x40) usci_uart_printf("read %i \r\nReturned: %x \r\n\r\n", (received_char[3] & 0xF), received_char[4]);
					else if (received_char[3] & 0x20) usci_uart_printf("flash\r\n\r\n");
					else if (received_char[3] & 0x10) usci_uart_printf("sensor %i \r\nReturned: %x \r\n\r\n", (received_char[3] & 0x7), received_char[4]);
					else if ( !(received_char[3] & 0x08) ) {
						if (received_char[3] & 0x04) usci_uart_printf("trig %i\r\n\r\n", (received_char[3] - 0x04));
						else {usci_uart_printf("sig %i\r\n\r\n", (received_char[3]));}
					}
					else if ( !(received_char[3] & 0x04) ) usci_uart_printf("control %i\r\n\r\n", (received_char[3] - 0x08 ) );
					else if ( ((received_char[3] & 0x03) & 0x1) ) usci_uart_printf("5v\r\n\r\n");
					else if ( ((received_char[3] & 0x03) & 0x2) ) usci_uart_printf("12v\r\n\r\n"); //*******************
				}// **Note: We could technically use arafe_argument in the output, but we don't get that back from slave, so it really doesn't ensure that slave "got and executed" that argument

				// If we are dumping, and haven't reached end of device_info, re-loop and transmit again with another device_info entity after
				// it has listened and printed out the transmission from the slave board
				if(dump_status == 1) {
					if (arafe_command < 0x4F) { // 16 values in device_info, array ordering 0-15. Corresponds to commands 64-79 (0x40-0x4F)
						arafe_command++; // Increment device_info position(embeded in actual arafe_command)
						__delay_cycles(128000); //  Add delay, the tester sends a command so quickly that the slave can't keep up
						arafe_send_command(); // Recall the switch loop and reset state to STATE_PREAMBLE_0
						return; // Want to return here so it exits the tx_handler and not just return once it finishes the above function
					} else {
						dump_status = 0;// Reset dump_status when exiting to avoid always being in dump mode once arafe_argument is too large
						m = 0; // Reset matrix formatting counter
						usci_uart_printf("\r\n");
					}
				}
			}

			state = STATE_IDLE;
			return;

		}
		 if (!(state == STATE_IDLE)) { // This needs to stay. If it doesn't everything breaks.
			 state++; // If state has nto been set to idle, carry on as usual
		 }
		//state++;
	}
