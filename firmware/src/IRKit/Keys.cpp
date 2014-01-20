/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Keys.h"
#include "Arduino.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"
#include "CRC8.h"
#include "convert.h" // x2i
#include "log.h"

#define MORSE_CREDENTIALS_SEPARATOR '/'

extern volatile char sharedbuffer[];

//
// DESCRIPTION:
// Fill wifi credentials
//

KeysFiller::KeysFiller ()
    : state(KeysFillerStateSecurity)
{
}

//
// DESCRIPTION:
// Save wifi credentials in EEPROM
//

Keys::Keys()
{
    data = (KeysShared*)sharedbuffer;
}

void Keys::load()
{
    eeprom_read_block((void*)data,   (void*)0,                  sizeof(KeysShared));
    eeprom_read_block((void*)&data2, (void*)sizeof(KeysShared), sizeof(KeysIndependent));
    if (! isCRCOK()) {
        clear();
    }
}

bool Keys::isCRCOK()
{
    uint8_t crc = crc8( (uint8_t*)data, sizeof(KeysCRCed) );
    return (crc == data->crc8);
}

// crc8 is ok and wifi credentials are valid
bool Keys::isWifiCredentialsSet()
{
    return data->wifi_is_set;
}

bool Keys::isAPIKeySet()
{
    return data2.key_is_set;
}

bool Keys::isValid()
{
    return data2.key_is_set && data2.key_is_valid;
}

bool Keys::wasWifiValid()
{
    return data->wifi_was_valid;
}

GSSECURITY Keys::getSecurity()
{
    return (GSSECURITY)data->security;
}

const char* Keys::getSSID()
{
    return data->ssid;
}

const char* Keys::getPassword()
{
    return data->password;
}

const char* Keys::getKey()
{
    return data2.key;
}

void Keys::set(GSSECURITY security, const char *ssid, const char *pass)
{
    data->security = security;
    strcpy(data->ssid,     ssid);
    strcpy(data->password, pass);
    data->wifi_is_set = true;
}

// for debug only
void Keys::setKey(const char *key)
{
    strcpy(data2.key, key);
    data2.key_is_set   = true;
    data2.key_is_valid = false;
}

void Keys::setWifiWasValid(bool valid)
{
    data->wifi_was_valid = valid;
}

void Keys::setKeyValid(bool valid)
{
    data2.key_is_valid = valid;
}

void Keys::save(void)
{
    data->crc8    = crc8( (uint8_t*)data, sizeof(KeysCRCed) );
    eeprom_write_block((const void*)data,   (void*)0,                  sizeof(KeysShared));
    save2();
}

void Keys::save2(void)
{
    eeprom_write_block((const void*)&data2, (void*)sizeof(KeysShared), sizeof(KeysIndependent));
}

void Keys::clear(void)
{
    memset( data, 0, sizeof(KeysShared) );

    clearKey();

    filler.state = KeysFillerStateSecurity;
    filler.index = 0;
}

void Keys::clearKey(void)
{
    memset( &data2, 0, sizeof(KeysIndependent) );
}

// we use morse code to transfer Security, SSID, Password, Key, CRC8 to IRKit device
// SSID can be multi byte, so we transfer HEX 4bit as 1 ASCII character (0-9A-F),
// so we need 2 morse letters to transfer a single character.
// we might want to transfer more in the future (like static IP), so prepare reserved state
// future iOS and firmware can support more parameters, while still supporting old firmware
// [0248]/#{SSID}/#{Password}/#{Key}///////#{CRC}
int8_t Keys::put(char code)
{
    static uint8_t  character;
    static bool     is_first_byte;
    static char    *container = 0;
    static uint8_t  max_length;

    if (code == MORSE_CREDENTIALS_SEPARATOR) {
        // null terminate
        switch (filler.state) {
        case KeysFillerStateSSID:
        case KeysFillerStatePassword:
        case KeysFillerStateKey:
            container[ filler.index ] = 0;
            break;
        default:
            break;
        }
        if (filler.state != KeysFillerStateCRC) {
            // wait for putDone() on CRC state

            filler.state = (KeysFillerState)( filler.state + 1 );

            switch (filler.state) {
            case KeysFillerStateSSID:
                container  = data->ssid;
                max_length = MAX_WIFI_SSID_LENGTH;
                break;
            case KeysFillerStatePassword:
                container  = data->password;
                max_length = MAX_WIFI_PASSWORD_LENGTH;
                break;
            case KeysFillerStateKey:
                container  = data->temp_key;
                max_length = MAX_KEY_LENGTH;
                break;
            default:
                break;
            }
        }
        is_first_byte = true;
        filler.index  = 0;
        return 0;
    }
    if ( ! (('0' <= code) && (code <= '9')) &&
         ! (('A' <= code) && (code <= 'F')) ) {
        // we only use letters which match: [0-9A-F]
        KEYLOG_PRINT("!E23:"); KEYLOG_PRINTLN2( code, HEX );
        return -1;
    }
    if (filler.state == KeysFillerStateSecurity) {
        switch (code) {
        case '3':
            // start of the 1st sine wave includes envelope = fadein effect
            // so 1st short sound can be mistaken as long sound,
            // fix it (3: _ _ -> 2: . _)
            code = '2';
            // continues vv

        // we try OPEN when WEP failed, to present less options to user
        case '0': // GSwifi::GSSECURITY_NONE:
        // case '1': // GSwifi::GSSECURITY_OPEN:
        case '2': // GSwifi::GSSECURITY_WEP:
        case '4': // GSwifi::GSSECURITY_WPA_PSK:
        case '8': // GSwifi::GSSECURITY_WPA2_PSK:
            data->security = (GSSECURITY)x2i(code);
            return 0;
        default:
            KEYLOG_PRINT("!E22:"); KEYLOG_PRINTLN2( code, HEX );
            return -1;
        }
    }

    if (filler.state == KeysFillerStateKey) {
        character = code;
    }
    else {
        // ssid, password might be Japanese character,
        // so we transfer utf8 bytes 0x00-0xFF as pair of [0-F] ASCII letters
        // so 2 bytes construct 1 character, network *bit* order
        // also CRC is transfered in HEX (pair of [0-F] ASCII letters)
        if (is_first_byte) {
            character       = x2i(code);
            character     <<= 4;        // F0h
            is_first_byte   = false;
            return 0;
        }
        else {
            character     += x2i(code); // 0Fh
            is_first_byte  = true;
        }
    }

    if (filler.state == KeysFillerStateCRC) {
        if (filler.index > 0) {
            KEYLOG_PRINTLN("!E21");
            return -1;
        }
        data->crc8 = character;
        filler.index ++;
        return 0;
    }

    if ( filler.index == max_length ) {
        KEYLOG_PRINTLN("!E18");
        return -1;
    }
    container[ filler.index ++ ] = character;
    return 0;
}

int8_t Keys::putDone()
{
    if (filler.state != KeysFillerStateCRC) {
        KEYLOG_PRINTLN("!E17");
        return -1;
    }

    data->wifi_is_set = true;

    if (isCRCOK()) {
        // copy to data2 area
        strcpy(data2.key, data->temp_key);
        data2.key_is_set   = true;
        data2.key_is_valid = false;
        return 0;
    }
    else {
        dump();
        KEYLOG_PRINTLN("!E16");
        return -1;
    }
}

void Keys::dump(void)
{
    // KEYLOG_PRINT("F:");
    // KEYLOG_PRINTLN(data->wifi_is_set);
    // KEYLOG_PRINTLN(data->wifi_was_valid);
    // KEYLOG_PRINTLN(data2.key_is_set);
    // KEYLOG_PRINTLN(data2.key_is_valid);

    // KEYLOG_PRINT("E:");
    // KEYLOG_PRINTLN(data->security);

    // KEYLOG_PRINT("S:");
    // KEYLOG_PRINTLN((const char*)data->ssid);
    // KEYLOG_PRINTLN((const char*)data->password);
    // KEYLOG_PRINTLN((const char*)data2.key);
    // KEYLOG_PRINTLN(data->crc8, HEX);
}
