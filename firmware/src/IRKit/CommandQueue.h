#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

#include "ringbuffer.h"

#define COMMAND_POST_KEYS  1
#define COMMAND_SETUP      2
#define COMMAND_CONNECT    3
#define COMMAND_CLOSE      4
#define COMMAND_START      5
#define COMMAND_QUEUE_SIZE 6

class CommandQueue {
 public:
    CommandQueue(char *buf, uint8_t size);
    void get(char *command);
    void put(char command);
    bool is_empty();
    bool is_full();

 private:
    struct RingBuffer buf_;

};

#endif
