#ifndef __PGMSTRTORAM_H__
#define __PGMSTRTORAM_H__

#include <avr/pgmspace.h>

#define getPSTR(s) pgmStrToRAM(PSTR(s))
#define P(s)       pgmStrToRAM(PSTR(s))

#ifdef __cplusplus
extern "C" {
#endif

char *pgmStrToRAM(PROGMEM char *theString);

#ifdef __cplusplus
}
#endif

#endif
