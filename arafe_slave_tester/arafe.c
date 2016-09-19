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

					//going to set up a switch logic table to figure out what ack to deliver
					if(received_char[3] == 0xFF) usci_uart_printf("flash"); //check flash, which is special
					else {
						switch (received_char[3] & 0xC0) {
							case 0x00: // attenuator
								// check if it's sig or trig
								//remember 0x00 -> 0x03 -> are the sig commands while 0x04->0x07 are the trig commands
								//so if we "and" 0x04 with 0x04 and above, we get a true, and realize it's a trig command
								//so if we "and" 0x04 with 0x03 and below, we get a true, and we relegate that to the else statement to ack the sig
								if (received_char[3] & 0x4) // if true, this is trig
									//so, 0x07 - 0x04 returns 0x03, which is channel 3, which is the channel we intended to program
									usci_uart_printf("trig %i", (received_char[3] - 0x04)); //subtract 4 from it, because want to print out the channel number
								else // otherwise, it's a sig
									//because the sig commands are 0x00 and 0x03, we don't need to adjust anything before returning the channel we issued the command to
									usci_uart_printf("sig %i", (received_char[3]));
								break;
							case 0x40: // sensor
								//the sensor commands are 0x40, 0x41, 0x42, and 0x43 for the first three sensors
								//0x44 through 0x47 give the last two bits of the ADC
								//put the logic table in here
								usci_uart_printf("sensor %i \r\nReturned: %x", (received_char[3] & 0xF), received_char[4]);
								//cout, I need to put something in here to handle the retreiving of the last two bits of the ADC, which are commands 44->47
								break;
							case 0x80: // info
								// check if it's a read or a write
								//remember, 0x85 would say read device info 5, while 0x95 would say write device info five
								//the logic is easiest to see in binary
								//for example, 0x85 would translate to 1000 0101 and 0x95 would be 1001 0101 and, 0x10 is 0001 0000
								//so, if we "and" 0x10 into 0x85, we get a false, and conclude it is a read
								//if we "and" 0x10 into 0x95 and get a true, we conclude it is a write
								if (received_char[3] & 0x10) // if true, this is a write
									//"and" against 0XFF (0000 1111) will clear the front numbers, but will preserve the device information
									usci_uart_printf("write %i", (received_char[3] & 0xF));
								else //otherise, it's false, and this is a read
									usci_uart_printf("read %i \r\nReturned: %x", (received_char[3] & 0xF), received_char[4]);
									//"and" against 0XFF (0000 1111) will clear the front numbers, but will preserve the device information
								break;
							case 0xC0: // misc
								//remember, C0, C1, C2, and C3 are the 12V enable for the four channels
								//while C4 and C5 are the global on and off for the 5V and 12 regulator
								//so if we "and" with 0x4, and return true, then we are dealing with the global on and off
								//if false, then we should be up in C0, C1, C2, and C3
								/*
								 //logic table
								C0: 1100 0000
								C1: 1100 0001
								C2: 1100 0010
								C3: 1100 0011

								C4: 1100 0100
								C5: 1100 0101

								01: 0000 0001
								02: 0000 0010
								03: 0000 0011
								04: 0000 0100

								C4: 1100 0100 //5v case
							 &	01: 0000 0001
								false

								C5: 1100 0101 //12v case
							 &  01: 0000 0001
							 	true
							 	*/

								if (received_char[3] & 0x4){ // if true, this is a global 5 or 12 V on or off
									if ((received_char[3] & 0x4) & 0x01) //this is a 12v global enable
										usci_uart_printf("12v regulator enable");
									else //this is a 5v global enable
										usci_uart_printf("5v regulator enable");
									break; //we're done
								}
								else //it's one of the individual line enables, and we just want to read out the channel you were trying to control
									usci_uart_printf("control %i", (received_char[3] & 0xF ));
								break;
						}
					}
					usci_uart_printf("\r\n\r\n"); //conclude with this to create a newline after the previous print out

					//The old routine by T. Erjavec
					/*
					if (received_char[3] & 0x80) usci_uart_printf("write %i\r\n\r\n", (received_char[3] & 0xF)); //done
					else if (received_char[3] & 0x40) usci_uart_printf("read %i \r\nReturned: %x \r\n\r\n", (received_char[3] & 0xF), received_char[4]); //done
					else if (received_char[3] & 0x20) usci_uart_printf("flash\r\n\r\n"); //done
					else if (received_char[3] & 0x10) usci_uart_printf("sensor %i \r\nReturned: %x \r\n\r\n", (received_char[3] & 0x7), received_char[4]);
					else if ( !(received_char[3] & 0x08) ) { //done
						if (received_char[3] & 0x04) usci_uart_printf("trig %i\r\n\r\n", (received_char[3] - 0x04));
						else {usci_uart_printf("sig %i\r\n\r\n", (received_char[3]));}
					}
					else if ( !(received_char[3] & 0x04) ) usci_uart_printf("control %i\r\n\r\n", (received_char[3] - 0x08 ) ); //done
					else if ( ((received_char[3] & 0x03) & 0x1) ) usci_uart_printf("5v\r\n\r\n"); //done
					else if ( ((received_char[3] & 0x03) & 0x2) ) usci_uart_printf("12v\r\n\r\n"); //done
					*/

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
