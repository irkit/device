#ifndef __GLOBAL_H__
#define __GLOBAL_H__

enum GBufferMode {
    GBufferModeIR              = 0,
    GBufferModeWifiCredentials = 1,
    GBufferModeUnused          = 10,
};

#define GLOBAL_BUFFER_SIZE 400

class Global {
 public:
    Global();
    void loop();
    unsigned long now;

    volatile char buffer[ GLOBAL_BUFFER_SIZE ];
    GBufferMode buffer_mode;
};

extern Global global;

#endif // __GLOBAL_H__
