#ifndef __IRKITHTTPCLIENT_H__
#define __IRKITHTTPCLIENT_H__

#include "GSwifi.h"

// we will start requesting GET /m again after
// recently_posted_timer timeouts, and client.timer timeouts: 5-10sec
#define SUSPEND_GET_MESSAGES_INTERVAL 5 // [sec]

class IRKitHTTPClient
{
public:
    IRKitHTTPClient(GSwifi* gs);
    int8_t postDoor();
    int8_t getMessages();
    int8_t postMessages();
    int8_t postKeys();
    void registerRequestHandler();

    void startTimer(uint8_t time);
    void onTimer();
    void loop();

private:
    GSwifi *gs_;
    volatile uint8_t timer_;
};

#endif
