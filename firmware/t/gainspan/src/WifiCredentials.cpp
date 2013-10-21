#include "WifiCredentials.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"
#include "CRC8.h"

//
// DESCRIPTION:
// Save wifi credentials in EEPROM
//

WifiCredentials::WifiCredentials()
{
    setup();
}

void WifiCredentials::setup()
{
    eeprom_read_block((void*)&data, (void*)0, sizeof(data));
}

// crc8 is ok && version is ok
bool WifiCredentials::isValid()
{
    if (! data.isSet) {
        return false;
    }
    uint8_t crc = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    return (crc == data.crc8) && (WIFICREDENTIALS_VERSION == data.version);
}

void WifiCredentials::set(uint8_t key, const uint8_t *value, uint8_t length)
{
    switch (key) {
    case WIFICREDENTIALS_KEY_SECURITY:
        data.security = *value;
        break;
    case WIFICREDENTIALS_KEY_SSID:
        memcpy(data.ssid, value, length);
        data.ssid[length] = '\0';
        break;
    case WIFICREDENTIALS_KEY_PASSWORD:
        memcpy(data.password, value, length);
        data.password[length] = '\0';
        break;
    default:
        break;
    }
}

void WifiCredentials::save(void)
{
    data.version = WIFICREDENTIALS_VERSION;
    data.crc8    = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    eeprom_write_block((const void*)&data, (void*)0, sizeof(data));
}

void WifiCredentials::clear(void)
{
    data.isSet = 0;
    save();
}

void WifiCredentials::dump(void)
{
    Serial.print(P("version: "));
    Serial.println(data.version);

    if (data.isSet) {
        Serial.print(P("security: "));
        switch (data.security) {
        case WIFICREDENTIALS_SECURITY_OPEN:
            Serial.println(P("open"));
            break;
        case WIFICREDENTIALS_SECURITY_WEP:
            Serial.println(P("wep"));
            break;
        case WIFICREDENTIALS_SECURITY_WPAPSK:
            Serial.println(P("wpapsk"));
            break;
        case WIFICREDENTIALS_SECURITY_WPA2PSK:
            Serial.println(P("wpa2psk"));
            break;
        default:
            break;
        }

        Serial.print(P("ssid: "));
        Serial.write((const char*)data.ssid);

        Serial.print(P("password: "));
        Serial.write((const char*)data.password);
    }
    else {
        Serial.println(P("cleared"));
    }
}

uint8_t WifiCredentials::crc(void)
{
    return data.crc8;
}
