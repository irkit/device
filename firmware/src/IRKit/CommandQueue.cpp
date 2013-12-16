#include "Arduino.h"
#include "CommandQueue.h"

CommandQueue::CommandQueue(char *data, uint8_t size)
{
    ring_init( &buf_, data, size );
}

void CommandQueue::get(char *command) {
    ring_get( &buf_, command, 1 );
}

void CommandQueue::put(char command) {
    ring_put( &buf_, command );
}

bool CommandQueue::is_empty() {
    return ring_isempty( &buf_ );
}

bool CommandQueue::is_full() {
    return ring_isfull( &buf_ );
}
