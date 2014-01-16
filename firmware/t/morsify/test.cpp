#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "CRC8.h"
#include "Keys.h"

int main() {
    Keys::KeysCRCed data;
    memset( (void*)&data, 0, sizeof(data) );
    data.security       = GSSECURITY_WEP;
    data.wifi_is_set    = true;
    data.wifi_was_valid = false;
    strcpy( data.ssid,     "001D739F0CB0" );
    strcpy( data.password, "abcde" );
    strcpy( data.temp_key, "1CB51E381C6A4576A54D6D37279F258C" );

    // security
    printf("%d/", data.security);

    // ssid
    for (int i=0; i<strlen(data.ssid); i++) {
        char letter = data.ssid[i];
        // char upper  = (letter & 0xF0) >> 4;
        // char lower  = (letter & 0x0F);
        // printf("%x%x", upper, lower);
        printf("%02x", letter & 0xFF);
    }
    printf("/");

    // password
    for (int i=0; i<strlen(data.password); i++) {
        char letter = data.password[i];
        char letters[3];
        sprintf( letters, "%02x", letter & 0xFF );

        char upper = letters[ 0 ];
        printf("%02x", upper);
        char lower = letters[ 1 ];
        printf("%02x", lower);
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
