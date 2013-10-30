#ifndef __KEYS_H__
#define __KEYS_H__

#include <Arduino.h>
#include "GSwifi.h"

// SSID is max 32 bytes
// see 7.3.1.2 of IEEE 802.11
#define MAX_WIFI_SSID_LENGTH     32

// password is max 63 characters
// see H.4.1 of IEEE 802.11
#define MAX_WIFI_PASSWORD_LENGTH 63

// it's an UUID
#define MAX_DEVICE_KEY_LENGTH    32

enum KeysFillerState {
    KeysFillerStateSecurity = 0,
    KeysFillerStateSSID     = 1,
    KeysFillerStatePassword = 2,
    KeysFillerStateToken    = 3,
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
    bool isSet();
    const char* getSSID();
    const char* getPassword();
    const char* getDeviceKey();
    GSwifi::GSSECURITY getSecurity();
    void set(GSwifi::GSSECURITY security, const char *ssid, const char *pass);
    void setDeviceKey(const char *key);
    void save();
    void clear();

    int8_t put(char dat);
    int8_t putDone();

    void dump();

 private:
    // Both SharedArea and IndependentArea are saved in EEPROM
    // SharedArea includes Wifi credentials, which is only needed
    // when we lost Wifi connection or haven't established it,
    // and no other classes will use gBuffer without Wifi connection.
    // IndependentArea is used to store device_key,
    // which is needed to communicate with server,
    // and that's going to happen when other classes (ex: IR) uses gBuffer
    // CRC is used to detect EEPROM corruption, so let's just use SharedArea for simplicity

    struct KeysShared
    {
        GSwifi::GSSECURITY security;
        char               ssid    [MAX_WIFI_SSID_LENGTH     + 1];
        char               password[MAX_WIFI_PASSWORD_LENGTH + 1];
        bool               is_set;
        uint8_t            crc8;
    };

    struct KeysCRCed
    {
        GSwifi::GSSECURITY security;
        char               ssid    [MAX_WIFI_SSID_LENGTH     + 1];
        char               password[MAX_WIFI_PASSWORD_LENGTH + 1];
        bool               is_set;
    };

    struct KeysIndependent
    {
        char device_key[MAX_DEVICE_KEY_LENGTH + 1];
        bool is_set;
    };

    bool isCRCOK();

    KeysShared      *data;
    KeysIndependent  data2;
    KeysFiller       filler;
};

#endif
