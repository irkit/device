#ifndef FlexiTimer2_h
#define FlexiTimer2_h

#ifdef __AVR__
#include <avr/interrupt.h>
#else
#error FlexiTimer2 library only works on AVR architecture
#endif


namespace FlexiTimer2 {
	extern uint16_t time_units;
	extern void (*func)();

	void set(uint16_t ms, void (*f)());
	void start();
	void stop();
	void _overflow();
}

#endif
