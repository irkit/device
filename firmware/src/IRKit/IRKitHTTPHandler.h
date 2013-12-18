#ifndef __IRKITHTTPHANDLER_H__
#define __IRKITHTTPHANDLER_H__

#include "GSwifi.h"

// we will start requesting GET /m again after
// recently_posted_timer timeouts, and handler.timer timeouts: 5-10sec
#define SUSPEND_GET_MESSAGES_INTERVAL 5 // [sec]

int8_t irkit_httpclient_post_door();
int8_t irkit_httpclient_get_messages();
int8_t irkit_httpclient_post_messages();
int8_t irkit_httpclient_post_keys();
void   irkit_httpclient_start_polling(uint8_t delay);
void   irkit_httpserver_register_handler();
void   irkit_http_on_timer();
void   irkit_http_loop();

#endif
