#ifndef __GSWIFI_CONST_H__
#define __GSWIFI_CONST_H__

#define GS_WREGDOMAIN           2 // 0:FCC, 1:ETSI, 2:TELEC

#define GS_TIMEOUT             10 // [s]
#define GS_TIMEOUT2            50 // [s]

#define GS_CMD_SIZE            64

#define GS_MAX_ROUTES           3
#define GS_MAX_PATH_LENGTH      9    // max: "/abcdefgh"

enum GSSECURITY {
    GSSECURITY_AUTO     = 0,
    GSSECURITY_NONE     = 0,
    GSSECURITY_OPEN     = 1,
    GSSECURITY_WEP      = 2,
    GSSECURITY_WPA_PSK  = 4,
    GSSECURITY_WPA2_PSK = 8,
};

#endif
