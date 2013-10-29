#ifndef __GLOBAL_H__
#define __GLOBAL_H__

class Global {
 public:
    Global();
    void loop();
    unsigned long now;
};

enum GBufferMode {
    GBufferModeIR,
    GBufferModeWifiCredentials,
    GBufferModeUnused,
};

extern Global global;
extern volatile char gBuffer[ 1024 ];
extern GBufferMode gBufferMode;

#endif // __GLOBAL_H__
