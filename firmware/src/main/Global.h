#ifndef __GLOBAL_H__
#define __GLOBAL_H__

enum GBufferMode {
    GBufferModeIR              = 0,
    GBufferModeWifiCredentials = 1,
    GBufferModeUnused          = 10,
};

class Global {
 public:
    Global();
    void loop();
    unsigned long now;

    volatile char buffer[ 1024 ];
    GBufferMode buffer_mode;

};

extern Global global;

#endif // __GLOBAL_H__
