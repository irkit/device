#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define GLOBAL_BUFFER_SIZE 400

class Global {
 public:
    Global();
    void loop();
    unsigned long now;

    volatile char buffer[ GLOBAL_BUFFER_SIZE ];
};

extern Global global;

#endif // __GLOBAL_H__
