#ifndef __GSWIFI_CONST_H__
#define __GSWIFI_CONST_H__

#define DOMAIN "api.getirkit.com"

#define GS_WREGDOMAIN           2 // 0:FCC, 1:ETSI, 2:TELEC

#define GS_TIMEOUT             20 // [s]
#define GS_TIMEOUT_LONG        50 // [s]
#define GS_TIMEOUT_NOWAIT       0

#define GS_CMD_SIZE            64

#define GS_MAX_ROUTES           4
#define GS_MAX_PATH_LENGTH      9    // max: "/messages"

enum GSSECURITY {
    GSSECURITY_AUTO     = 0,
    GSSECURITY_NONE     = 0,
    GSSECURITY_OPEN     = 1,
    GSSECURITY_WEP      = 2,
    GSSECURITY_WPA_PSK  = 4,
    GSSECURITY_WPA2_PSK = 8,
};

#endif
