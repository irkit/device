#ifndef _GSWIFI_CONF_H_
#define _GSWIFI_CONF_H_

#define GS_BAUD 9600  // default module baud

//#define GS_UART_DIRECT
#define GS_BULK
//#define GS_LIB_TINY

#define GS_DNSNAME "setup.local"
#define GS_WREGDOMAIN 2 // 0:FCC, 1:ETSI, 2:TELEC

#define GS_TIMEOUT 10000 // ms
#define GS_TIMEOUT2 130000 // ms
#define GS_RECONNECT 60 // s

#define GS_CMD_SIZE 100

#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368)
//#define GS_DATA_SIZE 1500
#define GS_DATA_SIZE 1024
#elif defined(TARGET_LPC11U24)
#define GS_DATA_SIZE 256
#elif defined(TARGET_KL25Z)
#define GS_DATA_SIZE 512
#endif

#ifndef GS_LIB_TINY

#define GS_SYSLOG // log for stdout

// ----- GSwifi_smtp.cpp -----

#define GS_ENABLE_SMTP  // use smtp client

#define SMTP_TIMEOUT 15000

// ----- GSwifi_http.cpp -----

#define GS_ENABLE_HTTP  // use http client

// ----- GSwifi_httpd.cpp -----

//#define GS_ENABLE_HTTPD  // use http server
//#define GS_ENABLE_WEBSOCKET  // use websocket server (need httpd)

#define HTTPD_TIMEOUT 15000
#define HTTPD_HANDLE 10

#define HTTPD_BUF_SIZE 200
#define HTTPD_URI_SIZE 100

#define HTTPD_KEEPALIVE 10 // request count

#endif // GS_LIB_TINY

#endif
