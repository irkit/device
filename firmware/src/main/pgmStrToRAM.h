#ifndef __PGMSTRTORAM_H__
#define __PGMSTRTORAM_H__

#include <avr/pgmspace.h>

#define P(s)       pgmStrToRAM(PSTR(s),0)
#define PB(s,i)    pgmStrToRAM(PSTR(s),i)

#ifdef __cplusplus
extern "C" {
#endif

char *pgmStrToRAM(PROGMEM char *theString, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
