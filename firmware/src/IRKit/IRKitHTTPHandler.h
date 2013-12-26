#ifndef __IRKITHTTPHANDLER_H__
#define __IRKITHTTPHANDLER_H__

#include "GSwifi.h"

// we will start requesting GET /m again after
// recently_posted_timer timeouts, and handler.timer timeouts: 5-10sec
#define SUSPEND_GET_MESSAGES_INTERVAL 5 // [sec]

extern int8_t irkit_httpclient_post_door();
extern int8_t irkit_httpclient_get_messages();
extern int8_t irkit_httpclient_post_messages();
extern int8_t irkit_httpclient_post_keys();
extern void   irkit_httpclient_start_polling(uint8_t delay);
extern void   irkit_httpserver_register_handler();
extern void   irkit_http_init();
extern void   irkit_http_on_timer();
extern void   irkit_http_loop();

#endif
