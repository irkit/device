#ifndef __GSWIFI_CONST_H__
#define __GSWIFI_CONST_H__

#define GS_WREGDOMAIN 2 // 0:FCC, 1:ETSI, 2:TELEC

#define GS_TIMEOUT             10000 // ms
#define GS_TIMEOUT2            60000 // ms
#define GS_LONGPOLL_TIMEOUT    40 // s
#define GS_LONGPOLL_TIMEOUT_MS 40000 // ms
#define GS_IGNORE_TIMEOUT      10 // ms

#define GS_CMD_SIZE 64

#define GS_MAX_ROUTES      3
#define GS_MAX_PATH_LENGTH 9    // max: "/abcdefgh"

enum GSSECURITY {
    GSSECURITY_AUTO     = 0,
    GSSECURITY_NONE     = 0,
    GSSECURITY_OPEN     = 1,
    GSSECURITY_WEP      = 2,
    GSSECURITY_WPA_PSK  = 4,
    GSSECURITY_WPA2_PSK = 8,
};

#endif
