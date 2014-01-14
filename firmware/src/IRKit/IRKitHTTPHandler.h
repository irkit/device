/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
