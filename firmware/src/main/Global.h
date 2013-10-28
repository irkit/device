#ifndef __GLOBAL_H__
#define __GLOBAL_H__

class Global {
 public:
    Global();
    void loop();
    unsigned long now;
};

extern Global global;

#endif // __GLOBAL_H__
