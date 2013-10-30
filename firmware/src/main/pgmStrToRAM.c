#include <avr/pgmspace.h>

// largest used in our program
#define PROGMEM_CACHE_SIZE 48

// rule
// index 0 is for directly using Serial.println(P("hoge"))
// index 1,.. is for passing char* into next function

// choose different index to use simultaneously
char to_print[3][PROGMEM_CACHE_SIZE];

char *pgmStrToRAM(PROGMEM char *theString, uint8_t index) {
    strcpy_P( to_print[ index ], theString );
    return to_print[index];
}
