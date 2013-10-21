#ifndef __WIFICREDENTIALS_H__
#define __WIFICREDENTIALS_H__

#include <Arduino.h>

// SSID is max 32 bytes
// see 7.3.1.2 of IEEE 802.11
#define WIFICREDENTIALS_MAX_SSID     32

// password is max 63 characters
// see H.4.1 of IEEE 802.11
#define WIFICREDENTIALS_MAX_PASSWORD 63

#define WIFICREDENTIALS_VERSION      1

#define WIFICREDENTIALS_SECURITY_OPEN    1
#define WIFICREDENTIALS_SECURITY_WEP     2
#define WIFICREDENTIALS_SECURITY_WPAPSK  3
#define WIFICREDENTIALS_SECURITY_WPA2PSK 4

#define WIFICREDENTIALS_KEY_SECURITY     1
#define WIFICREDENTIALS_KEY_SSID         2
#define WIFICREDENTIALS_KEY_PASSWORD     3

struct SavedData
{
    uint8_t ssid[WIFICREDENTIALS_MAX_SSID + 1];
    uint8_t password[WIFICREDENTIALS_MAX_PASSWORD + 1];
    uint8_t isSet;
    uint8_t security;
    uint8_t version;
    uint8_t crc8;
};

struct CRCedData
{
    uint8_t ssid[WIFICREDENTIALS_MAX_SSID + 1];
    uint8_t password[WIFICREDENTIALS_MAX_PASSWORD + 1];
    uint8_t isSet;
    uint8_t security;
    uint8_t version;
};

class WifiCredentials {
    public:
        WifiCredentials();

        bool isValid();
        uint8_t* get(uint8_t key);
        void set(uint8_t key, const uint8_t *value, uint8_t length);
        void save();
        void clear();
        void dump();

    private:
        SavedData data;
        void setup();
        uint8_t crc();
};

#endif
