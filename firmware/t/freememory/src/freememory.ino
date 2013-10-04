#include "Arduino.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"

void setup() {
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );

    /* // 2385 */
    /* Serial.print("aaa"); Serial.println(freeMemory()); */
    /* // 2351 */
    /* Serial.print("ffffffffffffffffffffffffffffffff"); Serial.println(freeMemory()); */
    /* // 2319 */
    /* Serial.print("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"); Serial.println(freeMemory()); */

    // 2376
    Serial.print(P("aaa")); Serial.println(freeMemory());
    Serial.print(P("ffffffffffffffffffffffffffffffff")); Serial.println(freeMemory());
    Serial.print(P("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee")); Serial.println(freeMemory());

    char cmd1[100];
    sprintf(cmd1, P("1:%s"),PB("aaa",1));
    Serial.println(cmd1);

    sprintf(cmd1, P("2:%s"),PB("bbb",1));
    Serial.println(cmd1);

}

void loop() {
}
