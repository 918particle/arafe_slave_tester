/*
 * usci_uart.c
 *
 *  Created on: Dec 9, 2014
 *      Author: barawn
 */

#define USCI_UART_C
#include "usci_uart.h"
#include <stdarg.h>

#pragma NOINIT(usci_uart_tx_pending)
uint8_t usci_uart_tx_pending;

void usci_uart_tx_process() {
	// Is the TX interrupt enabled?
	if (!(IE2 & USCI_UART_TXIE)) {
		// No, it isn't. If no data pending, return.
		if (usci_uart_tx_buffer_write_ptr == usci_uart_tx_buffer_read_ptr) return;
		// OK, data is pending, and no interrupt enabled. So prep things by pulling off a byte
		// and sticking it in usci_uart_tx_pending.
		usci_uart_tx_pending = usci_uart_tx_buffer[usci_uart_tx_buffer_read_ptr];
		usci_uart_tx_buffer_read_ptr = (usci_uart_tx_buffer_read_ptr + 1) & (USCI_UART_TX_BUFFER_SIZE-1);
		// Now enable the TXIE interrupt, which should immediately interrupt, grab the pending byte and start things off.
		IE2 |= USCI_UART_TXIE;
	}
}

#ifdef USCI_UART_PRINTF
void usci_uart_puts(const char *string) {
        while (*string) usci_uart_tx(*string++);
}

static const unsigned long usci_uart_dv[] = {
//  4294967296      // 32 bit unsigned max
    1000000000,     // +0
     100000000,     // +1
      10000000,     // +2
       1000000,     // +3
        100000,     // +4
//       65535      // 16 bit unsigned max
         10000,     // +5
          1000,     // +6
           100,     // +7
            10,     // +8
             1,     // +9
};

static void usci_uart_xtoa(unsigned long x, const unsigned long *dp)
{
    char c;
    unsigned long d;
    if(x) {
        while(x < *dp) ++dp;
        do {
            d = *dp++;
            c = '0';
            while(x >= d) ++c, x -= d;
            usci_uart_tx(c);
        } while(!(d & 1));
    } else
        usci_uart_tx('0');
}

static void usci_uart_puth(unsigned n)
{
    static const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    usci_uart_tx(hex[n & 15]);
}

void usci_uart_printf(char *format, ...)
{
    char c;
    int i; //blah;
    long n;

    va_list a;
    va_start(a, format);
    while(c = *format++) {
        if(c == '%') {
        	//blah = *format++;
            switch(c = *format++) {//I don't think it is getting here, so it is never getting to this if statement
                case 's':                       // String
                    usci_uart_puts(va_arg(a, char*));
                    break;
                case 'c':                       // Char
                    usci_uart_tx(va_arg(a, char));
                    break;
                case 'i':                       // 16 bit Integer
                case 'u':                       // 16 bit Unsigned
                    i = va_arg(a, int);
                    if(c == 'i' && i < 0) {
                    	i = -i;
                    	usci_uart_tx('-');
                    }
                    usci_uart_xtoa((unsigned)i, usci_uart_dv + 5);
                    break;
                case 'l':                       // 32 bit Long
                case 'n':                       // 32 bit uNsigned loNg
                    n = va_arg(a, long);
                    if(c == 'l' &&  n < 0) {
                    	n = -n;
                    	usci_uart_tx('-');
                    }
                    usci_uart_xtoa((unsigned long)n, usci_uart_dv);
                    break;
                case 'x':                       // 16 bit heXadecimal
                    i = va_arg(a, int);
                    usci_uart_puth(i >> 12);
                    usci_uart_puth(i >> 8);
                    usci_uart_puth(i >> 4);
                    usci_uart_puth(i);
                    break;
                case 0: return;
                default: goto bad_fmt;
            }
        } else
bad_fmt:    usci_uart_tx(c);
    }
    va_end(a);
}
#endif

