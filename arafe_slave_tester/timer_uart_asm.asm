	.cdecls C, LIST, "msp430.h"
	.cdecls C, LIST, "timer_uart.h"
	.global rx_char
	.global	tx_char_pending
	.global rx_bits
	.global timer1_isr_next_step
	.global timer_uart_tx_event
timer_uart_tx_event	.tag	event_t

COMPA_ISR:
	; Clear comparator interrupt.
	bic.b	#2, &CACTL1
	; Initialize TA1CCR1 to middle of the first bit, or so.
	mov.w	#18, &TACCR1 ; Use to be 140
	; Start the timer. (740 = 0x2E4 = TASSEL_2 + ID_3 + MC_2 + TACLR)
	mov.w	#740, &TACTL
	; Initialize rx_bits. Bit 7 is used to detect when we're done receiving (when it shifts into carry)
	mov.b	#(0x80), COND_RX_BITS
	; Initialize the first step in the Timer1 ISR to be looking for bits 0 to 7.
	mov.w	#(timer1_isr_bits0_to_7), COND_TIMER1_ISR_NEXT_STEP
	; Enable the interrupt.
	bis.w	#16, &TACCTL1
	; Return.
	reti

	; Receive ISR.
TIMER0_RX_ISR:
	; Clear TA1V. (Cheapest way possible)
	bit		#(0x01), &TAIV
	; Snag the comparator result right away into carry.
	bit		#(0x01), &CACTL2
	; Debugging code
	;bis.b #(0x40), &P2OUT; raise DEBUG
	;bic.b #(0x40), &P2OUT; lower DEBUG
	; Branch to our current step.
	branch	COND_TIMER1_ISR_NEXT_STEP


timer1_isr_bits0_to_7:
	; Shift carry into rx_bits, and everything else down.
	rrc.b	COND_RX_BITS
	; Is the carry bit 0? If so, we're not done. (Bit 7 was set before we started).
	jnc		timer1_isr_bits0_to_7_continue
	; Switch to stop bit check.
	mov.w	#(timer1_isr_stop_bit), COND_TIMER1_ISR_NEXT_STEP
timer1_isr_bits0_to_7_continue:
	; Continue. (104 is the bit time = 104 microseconds)
	add.w	#13, &TACCR1 ; Use to be 104
	; and return
	reti
timer1_isr_stop_bit:
	; Clear TACCTL1
	clr.w	&TACCTL1
	; Clear TA1CTL
	clr.w	&TACTL
	; Carry is still the stop bit. Is it 1?
	jc		timer1_isr_stop_bit_found
	; No, so it wasn't found.
	mov.b	#14, &CACTL1
	reti
timer1_isr_stop_bit_found:
	; Store rx_bits, it's OK.
	mov.b	COND_RX_BITS, &rxc
	; Now wake up (clear CPUOFF).
	bic.w	#(0x0010), 0(SP)
	; set rx_ok
	mov.b	#(1), &rx_ok
	; and return
	reti


	; Transmit ISR.
TIMER0_TX_ISR:
	; Rotate the LSB into carry.
	rrc.w	&tx_char_pending
	; Is it set?
	jnc		timer0_tx_isr_clear
	; It was set. We need to clear P1DIR bit 4 (tristate transmitter)
	; Nop-nop for even set/clear timing. They have a jz before setting.
	nop
	nop
	bic.b	#(BIT4), &P1DIR
	; Continue on.
	reti
timer0_tx_isr_clear:
	; Carry bit not set. Was the *whole word* zero? (stop bit)
	jz 		timer0_tx_isr_done
	; No, so that wasn't the stop bit. Set P1DIR (drive transmitter)
	bis.b	#(BIT4), &P1DIR
	; Continue on.
	reti
timer0_tx_isr_done:
	; turn off the timer
	mov.w 	#(TASSEL_2 + ID_3 + TACLR + MC_0), &TA0CTL
	; disable the capture/control block
	clr.w	&TACCTL0
	; push tx character done...
	tst.w		&timer_uart_tx_event.next
	jnz			timer0_tx_isr_complete
	mov.w		r4, &timer_uart_tx_event.next
	mov.w		#timer_uart_tx_event, r4
	; and wake up
	bic.w	#(LPM0), 0(SP)
timer0_tx_isr_complete:
	reti

	.sect	TIMER0_A0_VECTOR
	.short	TIMER0_TX_ISR
	.sect 	TIMER0_A1_VECTOR
	.short	TIMER0_RX_ISR
	.sect	COMPARATORA_VECTOR
	.short	COMPA_ISR
