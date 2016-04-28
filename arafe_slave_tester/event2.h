/*
 * event2.h
 *
 * This is a lightweight event-flagging library.
 *
 * Event handlers are prototyped as void handler(): however the handlers, once called,
 * must clear the event if they ever want it to be called again (via event_clear()). This should be the FIRST THING
 * DONE. If the event handlers want to disable future events, call event_disable();
 *
 * An example of the library's usage is:
 * \code
 * #pragma NOINIT(my_event);
 * event_t my_event;
 *
 * void my_event_handler() {
 * 		event_clear(&my_event);
 *      // do something
 * }
 * int main() {
 *      event_init();
 *      event_handler_init(&my_event, my_event_handler, true);
 *      while (1) {
 *			event_process();
 *			event_sleep();
 *      }
 * }
 * \endcode
 *
 * Obviously event_queue() needs to be called somewhere, or the assembly equivalent macro event_queue_interruptible_asm. There are two event queues
 * maintained - one for higher priority events (and whose status is volatile) and another for lower priority events, whose status is non-volatile:
 * that is, they are always posted from non-ISR code.
 *
 *  Created on: Dec 5, 2014
 *      Author: barawn
 */

#ifndef ARAFE_SLAVE_TESTER_TYLER_EVENT2_H_
#define ARAFE_SLAVE_TESTER_TYLER_EVENT2_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*event_handler_t)();

typedef struct event_t {
	event_handler_t handler;
	struct event_t *next;
} event_t;

#define EVENT_TAIL 0xFFFF
#define EVENT_WAITING 0x0000
#define EVENT_DISABLED 0x0001

#ifndef EVENT_C
extern event_t *event_head;
#else
#pragma NOINIT(event_head)
event_t *event_head;
#endif

//< I hate you, C compiler. So, so much.
static inline void event_init() {
	asm("	mov.w	#0xFFFF, r4");
	event_head = (event_t *) EVENT_TAIL;
}

static inline void event_handler_init(event_t *p, event_handler_t handler, bool enabled) {
	if (enabled) {
		p->handler = handler;
		p->next = (event_t *) EVENT_WAITING;
	} else {
		// Disabled event. Set the next pointer to the disabled marker. That way when the
		// event is posted, it will be skipped because p->next is already set.
		p->handler = handler;
		p->next = (event_t *) EVENT_DISABLED;
	}
}

//< Clear an event. Do this first inside the handler.
static inline void event_clear(event_t *p) {
	p->next = (event_t *) EVENT_WAITING;
}

/** \brief Disable any future event posts. Do this first inside the handler, if desired. Do NOT do this anywhere else.
 *
 * Disabling active events can \em only be done inside the handler. Disabling them outside the handler isn't
 * ISR-safe, and I can't think of a reason why you would need to. Outside the handler, you could always set the event's
 * handler to event_handler_ignore_and_disable().
 */
static inline void event_disable(event_t *p) {
	p->next = (event_t *) EVENT_DISABLED;
}

//< Enable a disabled event. This is ISR-safe: ISRs won't touch an event with p->next != 0.
static inline void event_enable(event_t *p) {
	if (p->next == (event_t *) EVENT_DISABLED) p->next = (event_t *) EVENT_WAITING;
}

//< Queue up an event.
static inline void event_queue(event_t *p) {
	if (p->next) return;
	p->next = event_head;
	event_head = p;
}

//< Replace an event's handler.
static inline void event_set_handler(event_t *p, event_handler_t handler) {
	p->handler = handler;
}

//< This code checks to see if there are events pending, and then sleeps.
static inline void event_sleep() {
	if ((uint16_t) event_head != (event_t *) EVENT_TAIL) goto event_sleep_wakeup;
	__disable_interrupt();
	asm("\t cmp.w	#0xFFFF, r4\n"
		"\t jne 	event_sleep_wakeup?\n"
		"\t bis.w	#0x18, SR\n"
		"event_sleep_wakeup?:\n");
	event_sleep_wakeup:
	__enable_interrupt();
}

//< Process pending events.
static inline void event_process() {
	event_t *pt, *ptnext;
	__disable_interrupt();
	pt = (event_t *) __get_R4_register();
	asm("\t mov.w #(0xFFFF), r4");
	__enable_interrupt();
	while (pt != (event_t *) EVENT_TAIL) {
		ptnext = pt->next;
		pt->next = (event_t *) EVENT_WAITING;
		pt->handler();
		pt = ptnext;
	}
	// Now process the non-ISR event queue.
	pt = event_head;
	event_head = (event_t *) EVENT_TAIL;
	while (pt != (event_t *) EVENT_TAIL) {
		ptnext = pt->next;
		pt->next = (event_t *) EVENT_TAIL;
		pt->handler();
		pt = ptnext;
	}
}

// This is for ISRs that AREN'T already in assembly.
// Hopefully there are none, because this code does dumb things.
static inline void event_queue_interruptible_atomic(event_t *p) {
	if (p->next) return;
	p->next = (event_t *) __get_R4_register();
	__set_R4_register((uint16_t) p);
}

// This is for queueing up an interruptible event outside of an ISR.
static inline void event_queue_interruptible(event_t *p) {
	__disable_interrupt();
	event_queue_interruptible_atomic(p);
	__enable_interrupt();
}

#endif /* ARAFE_SLAVE_TESTER_TYLER_EVENT2_H_ */
