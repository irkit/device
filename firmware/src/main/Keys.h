#ifndef __KEYS_H__
#define __KEYS_H__

#include "GSwifi_const.h"
#include <inttypes.h>

// SSID is max 32 bytes
// see 7.3.1.2 of IEEE 802.11
#define MAX_WIFI_SSID_LENGTH     32

// password is max 63 characters
// see H.4.1 of IEEE 802.11
#define MAX_WIFI_PASSWORD_LENGTH 63

// it's an UUID
#define MAX_KEY_LENGTH           36

enum KeysFillerState {
    KeysFillerStateSecurity = 0,
    KeysFillerStateSSID     = 1,
    KeysFillerStatePassword = 2,
    KeysFillerStateKey      = 3,
    KeysFillerStateCRC      = 4,
};

class KeysFiller {
 public:
    KeysFiller();

    KeysFillerState state;
    uint8_t index;
};

class Keys {
 public:
    Keys();

    void load();
    bool isWifiCredentialsSet();
    bool isAPIKeySet();
    bool isValid();
    const char* getSSID();
    const char* getPassword();
    const char* getKey();
    GSSECURITY getSecurity();
    void set(GSSECURITY security, const char *ssid, const char *pass);
    void setKey(const char *key);
    void setKeyValid(bool valid);
    void save();
    void save2();
    void clear();
    void clearKey();
    int8_t put(char dat);
    int8_t putDone();

    void dump();

    // Both KeysShared and KeysIndependent are saved in EEPROM
    // KeysShared includes Wifi credentials, which is only needed
    // when we lost Wifi connection or haven't established it,
    // and no other classes will use gBuffer without Wifi connection.
    // So KeysShared area is shared with gBuffer.
    // KeysIndependent is used to store key,
    // which is needed to communicate with server,
    // and that's going to happen when other classes (ex: IR) uses gBuffer.
    // CRC is used to detect EEPROM corruption and corruption during Morse communication

    struct KeysShared
    {
        uint8_t    security;
        char       ssid    [MAX_WIFI_SSID_LENGTH     + 1];
        char       password[MAX_WIFI_PASSWORD_LENGTH + 1];
        uint8_t    wifi_is_set;
        // temp_key is only used when
        // receiving key through morse communication.
        // when morse communication is done, we copy key to KeysIndependent area
        // and key is accessed through KeysIndependent afterwards
        char       temp_key[MAX_KEY_LENGTH           + 1];

        uint8_t    crc8;
    } __attribute__ ((packed));

    struct KeysCRCed
    {
        uint8_t    security;
        char       ssid    [MAX_WIFI_SSID_LENGTH     + 1];
        char       password[MAX_WIFI_PASSWORD_LENGTH + 1];
        uint8_t    wifi_is_set;
        char       temp_key[MAX_KEY_LENGTH           + 1];
    } __attribute__ ((packed));

    struct KeysIndependent
    {
        char       key     [MAX_KEY_LENGTH           + 1];
        bool       key_is_set;
        bool       key_is_valid; // POST /door succeeded
    };

 private:
    bool isCRCOK();

    KeysShared      *data;
    KeysIndependent  data2;
    KeysFiller       filler;
};

#endif
