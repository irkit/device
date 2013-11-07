#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "CRC8.h"
#include "Keys.h"

int main() {
    Keys::KeysCRCed data;
    memset( (void*)&data, 0, sizeof(data) );
    data.security    = GSSECURITY_WPA2_PSK;
    data.wifi_is_set = true;
    strcpy( data.ssid,     "Rhodos" );
    strcpy( data.password, "aaaaaaaaaaaaa" );
    strcpy( data.temp_key, "abc" );

    // security
    printf("%d/", data.security);

    // ssid
    for (int i=0; i<strlen(data.ssid); i++) {
        char letter = data.ssid[i];
        char upper  = (letter & 0xF0) >> 4;
        char lower  = (letter & 0x0F);
        printf("%x%x", upper, lower);
    }
    printf("/");

    // password
    for (int i=0; i<strlen(data.password); i++) {
        char letter = data.password[i];
        char upper  = (letter & 0xF0) >> 4;
        char lower  = (letter & 0x0F);
        printf("%x%x", upper, lower);
    }
    printf("/");

    // key
    for (int i=0; i<strlen(data.temp_key); i++) {
        char letter = data.temp_key[i];
        char upper  = (letter & 0xF0) >> 4;
        char lower  = (letter & 0x0F);
        printf("%x%x", upper, lower);
    }
    printf("/");

    // crc8
    uint8_t crc = crc8( (uint8_t*)&data, sizeof(data) );
    {
        char upper  = (crc & 0xF0) >> 4;
        char lower  = (crc & 0x0F);
        printf("%x%x", upper, lower);
    }
    printf("\n");

    printf("size: %lu", sizeof(data));
}
