	.cdecls C,LIST,"usci_uart.h"

	;; Pending character to transmit.
	.global		usci_uart_tx_pending
	;; TX write buffer.
	.global		usci_uart_tx_buffer
	.global		usci_uart_tx_buffer_write_ptr
	.global		usci_uart_tx_buffer_read_ptr

	;; Receive ISR.
usci_uart_RX_ISR: .asmfunc ;Gets here
	;; Store r15
	push		r15
	mov.b		&usci_uart_rx_buffer_write_ptr, r15
	mov.b		&USCI_UART_RXBUF, usci_uart_rx_buffer(r15)
	inc.b		r15
	bic.b		#USCI_UART_RX_BUFFER_SIZE, r15
	mov.b		r15, &usci_uart_rx_buffer_write_ptr
	tst.w		&usci_uart_rx_event.next
	jnz			usci_uart_RX_ISR_complete
	mov.w		r4, &usci_uart_rx_event.next
	mov.w		#usci_uart_rx_event, r4
	pop			r15
	bic.w		#(LPM3), 0(SP)
usci_uart_RX_ISR_complete:
	reti
	.endasmfunc

	;; Transmit ISR.
usci_uart_TX_ISR: .asmfunc
	;; Copy the pending character over.
	mov.b		&usci_uart_tx_pending, &USCI_UART_TXBUF
	;; Now fetch the new one from the buffer, if there is one.
	cmp.b		&usci_uart_tx_buffer_write_ptr, &usci_uart_tx_buffer_read_ptr
	;; If equal there isn't one. 2 cycles.
	jeq			usci_uart_TX_ISR_finish
	;; Not equal, so there is one. Fetch it. First store r15. 4 cycles.
	push		r15
	;; Now fetch the pointer. 4 more cycles.
	mov.b		&usci_uart_tx_buffer_read_ptr, r15
	;; Indexed store. 6 cycles.
	mov.b		usci_uart_tx_buffer(r15), &usci_uart_tx_pending
	;; Increment. 1 cycle. C and Z both get cleared here.
	inc.w		r15
	;; Clear bit. 2 cycles.
	bic.b		#USCI_UART_TX_BUFFER_SIZE, r15
	;; Save pointer.
	mov.b		r15, &usci_uart_tx_buffer_read_ptr
	;; Restore. 4 cycles
	pop			r15
	;; Test our event. 4 cycles. This sets only Z, and clears C. (tst.w sets Z and C).
	add.w		#0, &usci_uart_tx_event.next
	;; 2 cycles, 1 word.
	jnz			usci_uart_TX_ISR_post_event_complete
	;; Event not posted yet. Save the old head: 4 cycles, 2 words.
	mov.w		r4, &usci_uart_tx_event.next
	;; and post the new one. 2 cycles, 2 words.
	mov.w		#usci_uart_tx_event, r4
	;; Now wake up.
	bic			#(LPM3), 0(SP)
usci_uart_TX_ISR_post_event_complete:
	;; Done.
	reti
usci_uart_TX_ISR_finish:
	;; Clear TXIFG interrupt, there's no new data. No reason to post an event either, no one waits for the last byte.
	bic			#(USCI_UART_TXIE), &IE2
	;; Done. 5 cycles, 1 word.
	reti
	.endasmfunc

USCIA0_TX_USCIB0_TX_ISR:
	bit.b	#(USCI_UART_TXIFG), &IFG2
	jnz		usci_uart_TX_ISR
	;; other code here
	reti
USCIA0_RX_USCIB0_RX_ISR:
	bit.b	#(USCI_UART_RXIFG), &IFG2
	jnz		usci_uart_RX_ISR
	;; other code here
	reti

	.sect 	USCIAB0TX_VECTOR
	.short	USCIA0_TX_USCIB0_TX_ISR
	.sect	USCIAB0RX_VECTOR
	.short	USCIA0_RX_USCIB0_RX_ISR
