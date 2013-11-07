#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "CRC8.h"
#include "Keys.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );

    Serial.println("start");

    Keys::KeysCRCed data;
    memset( (void*)&data, 0, sizeof(data) );
    data.security    = GSSECURITY_WPA2_PSK;
    data.wifi_is_set = true;
    strcpy( data.ssid,     "Rhodos" );
    strcpy( data.password, "aaaaaaaaaaaaa" );
    strcpy( data.temp_key, "abc" );

    uint8_t crc = crc8( (uint8_t*)&data, sizeof(data) );
    Serial.print("crc: 0x"); Serial.println(crc, HEX);

    Serial.print("size: "); Serial.println(sizeof(data));
}

void loop() {
}
