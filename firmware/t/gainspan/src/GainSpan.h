#ifndef __GAINSPAN_H__
#define __GAINSPAN_H__

#include "Arduino.h"

#define GS_RECONNECT 60 // [s]
#define GS_CMD_SIZE 100

class GainSpan {
    public:
        /**
         * Wi-Fi security
         */
        enum GSSECURITY {
            GSSEC_AUTO       = 0,
            GSSEC_NONE       = 0,
            GSSEC_OPEN       = 1,
            GSSEC_WEP        = 2,
            GSSEC_WPA_PSK    = 4,
            GSSEC_WPA2_PSK   = 8,
            GSSEC_WPA_ENT    = 16,
            GSSEC_WPA2_ENT   = 32,
            GSSEC_WPS_BUTTON = 64,
            GSSEC_WPS_PIN,
        };
        enum GSSTATUS {
            GSSTAT_READY,
            GSSTAT_STANDBY,
            GSSTAT_WAKEUP,
            GSSTAT_DEEPSLEEP,
        };

        GainSpan(HardwareSerial* serial);
        void setup();
        void sendCommand(char *command);
        int connect (GSSECURITY sec, const char *ssid, const char *pass, int dhcp, int reconnect);

    private:
        HardwareSerial* serial_;

        // todo: initialize
        uint16_t timeout_start_;
        bool busy_;
        bool did_timeout_;
        void (*onTimeout_)();
        bool connected_;
        GSSTATUS status_;

        void parseCmdResponse (char *buf);
        uint8_t checkActivity(uint16_t timeout_ms);
        bool setBusy(bool busy);
        uint8_t parse(uint8_t character);
};

#endif // __GAINSPAN_H__
