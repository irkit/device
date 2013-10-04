#include <avr/pgmspace.h>
#include <stdlib.h>

// rule
// index 0 is for directly using Serial.println(P("hoge"))
// index 1,.. is for passing char* into next function

// choose different index to use simultaneously
char *to_print[3];

char *pgmStrToRAM(PROGMEM char *theString, uint8_t index) {
	free(to_print[index]);
	to_print[index]=(char *) malloc(strlen_P(theString));
	strcpy_P(to_print[index], theString);
	return (to_print[index]);
}
