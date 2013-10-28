#include "WifiCredentials.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"
#include "CRC8.h"
#include "Global.h"
#include "convert.h"

#define MORSE_CREDENTIALS_SEPARATOR '/'

//
// DESCRIPTION:
// Fill wifi credentials
//

WifiCredentialsFiller::WifiCredentialsFiller ()
    : state(WifiCredentialsFillerStateSecurity)
{
}

//
// DESCRIPTION:
// Save wifi credentials in EEPROM
//

WifiCredentials::WifiCredentials()
{
    data = (SavedData*)gBuffer;
}

void WifiCredentials::load()
{
    eeprom_read_block((void*)&data, (void*)0, sizeof(data));
}

// crc8 is ok && version is ok
bool WifiCredentials::isValid()
{
    if (! data->isSet) {
        return false;
    }
    uint8_t crc = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    return (crc == data->crc8) && (WIFICREDENTIALS_VERSION == data->version);
}

GSwifi::GSSECURITY WifiCredentials::getSecurity()
{
    return data->security;
}

char* WifiCredentials::getSSID()
{
    return data->ssid;
}

char* WifiCredentials::getPassword()
{
    return data->password;
}

void WifiCredentials::set(GSwifi::GSSECURITY security, const char *ssid, const char *pass)
{
    data->security = security;
    strcpy(data->ssid, ssid);
    strcpy(data->password, pass);
    data->isSet = true;
}

void WifiCredentials::save(void)
{
    data->version = WIFICREDENTIALS_VERSION;
    data->crc8    = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    eeprom_write_block((const void*)&data, (void*)0, sizeof(data));
}

void WifiCredentials::clear(void)
{
    data->isSet = 0;
    save();

    filler.state = WifiCredentialsFillerStateSecurity;
    filler.index = 0;
}

// we use morse code to transfer Security, SSID, Password, Token, CRC8 to IRKit device
// SSID can be multi byte, so we transfer HEX 4bit as 1 ASCII character (0-9A-F),
// so we need 2 morse letters to transfer a single character.
int8_t WifiCredentials::put(char code)
{
    static uint8_t character;
    static bool is_first_byte;

    if (code == MORSE_CREDENTIALS_SEPARATOR) {
        // wait for putDone() on CRC state
        switch (filler.state) {
        case WifiCredentialsFillerStateSecurity:
            filler.state = WifiCredentialsFillerStateSSID;
            break;
        case WifiCredentialsFillerStateSSID:
            data->ssid[ filler.index ] = 0;
            filler.state = WifiCredentialsFillerStatePassword;
            break;
        case WifiCredentialsFillerStatePassword:
            data->password[ filler.index ] = 0;
            filler.state = WifiCredentialsFillerStateToken;
            break;
        case WifiCredentialsFillerStateToken:
            data->token[ filler.index ] = 0;
            filler.state = WifiCredentialsFillerStateCRC;
            break;
        case WifiCredentialsFillerStateCRC:
            // wait
            break;
        default:
            break;
        }
        is_first_byte = 1;
        filler.index  = 0;
        return 0;
    }
    if ( ! (('0' <= code) && (code <= '9')) &&
         ! (('A' <= code) && (code <= 'F')) ) {
        // we only use letters which match: [0-9A-F,]
        Serial.print(P("unexpected code: 0x")); Serial.println( code, HEX );
        return -1;
    }
    if (filler.state == WifiCredentialsFillerStateSecurity) {
        switch (code) {
        case '0': // GSwifi::GSSECURITY_NONE:
        case '1': // GSwifi::GSSECURITY_OPEN:
        case '2': // GSwifi::GSSECURITY_WEP:
        case '4': // GSwifi::GSSECURITY_WPA_PSK:
        case '8': // GSwifi::GSSECURITY_WPA2_PSK:
            data->security = (GSwifi::GSSECURITY)x2i(code);
            return 0;
        default:
            Serial.print(P("unexpected security: 0x")); Serial.println( code, HEX );
            return -1;
        }
    }
    // we transfer [0..9A..F] as ASCII
    // so 2 bytes construct 1 character, network *bit* order
    if (is_first_byte) {
        character          = x2i(code);
        character        <<= 4;         // F0h
        is_first_byte   = false;
        return 0;
    }
    else {
        character        += x2i(code); // 0Fh
        is_first_byte  = true;
    }

    switch (filler.state) {
    case WifiCredentialsFillerStateSSID:
        if ( filler.index == WIFICREDENTIALS_MAX_SSID ) {
            Serial.println(P("overflow 1"));
            return -1;
        }
        data->ssid[ filler.index ++ ] = character;
        break;
    case WifiCredentialsFillerStatePassword:
        if ( filler.index == WIFICREDENTIALS_MAX_PASSWORD ) {
            Serial.println(P("overflow 2"));
            return -1;
        }
        data->password[ filler.index ++ ] = character;
        break;
    case WifiCredentialsFillerStateToken:
        if (filler.index == CREDENTIALS_MAX_TOKEN) {
            Serial.println(P("overflow 3"));
            return -1;
        }
        data->token[ filler.index ++ ] = character;
        break;
    case WifiCredentialsFillerStateCRC:
        if (filler.index > 0) {
            Serial.println(P("overflow 4"));
            return -1;
        }
        data->crc8 = character;
        filler.index ++;
        break;
    default:
        Serial.println(P("??"));
        return -1;
    }
    return 0;
}

int8_t WifiCredentials::putDone()
{
    if (filler.state != WifiCredentialsFillerStateCRC) {
        Serial.println(P("state error"));
        return -1;
    }

    // check CRC
    data->isSet   = 1;
    data->version = WIFICREDENTIALS_VERSION;

    dump();

    if (isValid()) {
        return 0;
    }
    else {
        Serial.println(P("crc error"));
        return -1;
    }
}

void WifiCredentials::dump(void)
{
    Serial.print(P("version: "));
    Serial.println(data->version);

    Serial.print(P("isSet: "));
    Serial.println(data->isSet);

    Serial.print(P("security: "));
    switch (data->security) {
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
    Serial.println((const char*)data->ssid);

    Serial.print(P("password: "));
    Serial.println((const char*)data->password);

    Serial.print(P("token: "));
    Serial.println((const char*)data->token);

    Serial.print(P("crc8: 0x"));
    Serial.println(data->crc8, HEX);
}

uint8_t WifiCredentials::crc(void)
{
    return data->crc8;
}
