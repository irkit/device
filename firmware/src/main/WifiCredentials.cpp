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

GSwifi::GSSECURITY WifiCredentials::getSecurity()
{
    return data.security;
}

char* WifiCredentials::getSSID()
{
    return data.ssid;
}

char* WifiCredentials::getPassword()
{
    return data.password;
}

void WifiCredentials::set(GSwifi::GSSECURITY security, const char *ssid, const char *pass)
{
    data.security = security;
    strcpy(data.ssid, ssid);
    strcpy(data.password, pass);
    data.isSet = true;
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
        case GSwifi::GSSECURITY_AUTO:
            Serial.println(P("auto/none"));
            break;
        case GSwifi::GSSECURITY_OPEN:
            Serial.println(P("open"));
            break;
        case GSwifi::GSSECURITY_WEP:
            Serial.println(P("wep"));
            break;
        case GSwifi::GSSECURITY_WPA_PSK:
            Serial.println(P("wpa-psk"));
            break;
        case GSwifi::GSSECURITY_WPA2_PSK:
            Serial.println(P("wpa2-psk"));
            break;
        default:
            break;
        }

        Serial.print(P("ssid: "));
        Serial.write((const char*)data.ssid);
        Serial.println();

        Serial.print(P("password: "));
        Serial.write((const char*)data.password);
        Serial.println();
    }
    else {
        Serial.println(P("cleared"));
    }
}

uint8_t WifiCredentials::crc(void)
{
    return data.crc8;
}
