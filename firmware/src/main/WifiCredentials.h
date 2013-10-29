#ifndef __WIFICREDENTIALS_H__
#define __WIFICREDENTIALS_H__

#include <Arduino.h>
#include "GSwifi.h"

// SSID is max 32 bytes
// see 7.3.1.2 of IEEE 802.11
#define WIFICREDENTIALS_MAX_SSID     32

// password is max 63 characters
// see H.4.1 of IEEE 802.11
#define WIFICREDENTIALS_MAX_PASSWORD 63

#define CREDENTIALS_MAX_TOKEN        40

#define WIFICREDENTIALS_VERSION      1

struct SavedData
{
    char ssid    [WIFICREDENTIALS_MAX_SSID + 1];
    char password[WIFICREDENTIALS_MAX_PASSWORD + 1];
    char token   [CREDENTIALS_MAX_TOKEN + 1];
    uint8_t isSet;
    GSwifi::GSSECURITY security;
    uint8_t version;
    uint8_t crc8;
};

struct CRCedData
{
    char ssid    [WIFICREDENTIALS_MAX_SSID + 1];
    char password[WIFICREDENTIALS_MAX_PASSWORD + 1];
    char token   [CREDENTIALS_MAX_TOKEN + 1];
    uint8_t isSet;
    GSwifi::GSSECURITY security;
    uint8_t version;
};

enum WifiCredentialsFillerState {
    WifiCredentialsFillerStateSecurity = 0,
    WifiCredentialsFillerStateSSID     = 1,
    WifiCredentialsFillerStatePassword = 2,
    WifiCredentialsFillerStateToken    = 3,
    WifiCredentialsFillerStateCRC      = 4,
};

class WifiCredentialsFiller {
 public:
    WifiCredentialsFiller();

    WifiCredentialsFillerState state;
    uint8_t index;
};

class WifiCredentials {
 public:
    WifiCredentials();

    void load();
    bool isValid();
    char* getSSID();
    char* getPassword();
    char* getToken();
    GSwifi::GSSECURITY getSecurity();
    void set(GSwifi::GSSECURITY security, const char *ssid, const char *pass);
    void save();
    void clear();

    int8_t put(char dat);
    int8_t putDone();

    void dump();

 private:
    SavedData *data;
    WifiCredentialsFiller filler;
    uint8_t crc();
};

#endif
