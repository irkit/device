/* Copyright (C) 2013 gsfan, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/** @file
 * @brief Gainspan wi-fi module library for mbed
 * GS1011MIC, GS1011MIP, GainSpan WiFi Breakout, etc.
 */

#ifndef _GSWIFI_H_
#define _GSWIFI_H_

#define DEBUG

#include "Arduino.h"
#include "CBuffer.h"
// #include "GSFunctionPointer.h"
#include "host.h"
#include "ipaddr.h"
#include "GSwifi_conf.h"

/**
 * GSwifi class
 */
class GSwifi {
public:

/**
 * Wi-Fi security
 */
enum GSSECURITY {
    GSSEC_AUTO = 0,
    GSSEC_NONE = 0,
    GSSEC_OPEN = 1,
    GSSEC_WEP = 2,
    GSSEC_WPA_PSK = 4,
    GSSEC_WPA2_PSK = 8,
    GSSEC_WPA_ENT = 16,
    GSSEC_WPA2_ENT = 32,
};

/**
 * TCP/IP protocol
 */
enum GSPROTOCOL {
    GSPROT_UDP = 0,
    GSPROT_TCP = 1,
    GSPROT_HTTPGET,
    GSPROT_HTTPPOST,
    GSPROT_HTTPD,
};

/**
 * Client/Server
 */
enum GSTYPE {
    GSTYPE_CLIENT = 0,
    GSTYPE_SERVER = 1,
};

enum GSRESPONCE {
    GSRES_NONE,
    GSRES_NORMAL,
    GSRES_CONNECT,
    GSRES_MACADDRESS,
    GSRES_DHCP,
    GSRES_DNSLOOKUP,
    GSRES_HTTP,
    GSRES_RSSI,
    GSRES_TIME,
    GSRES_STATUS,
};

enum GSMODE {
    GSMODE_COMMAND,
    GSMODE_DATA_RX,
    GSMODE_DATA_RXUDP,
    GSMODE_DATA_RX_BULK,
    GSMODE_DATA_RXUDP_BULK,
    GSMODE_DATA_RXHTTP,
};

enum GSSTATUS {
    GSSTAT_READY,
    GSSTAT_STANDBY,
    GSSTAT_WAKEUP,
    GSSTAT_DEEPSLEEP,
};

/**
 * data receive callback function
 */
typedef void (*onGsReceiveFunc)(int cid, int len);

struct GS_Socket {
    GSTYPE type;
    GSPROTOCOL protocol;
    bool connect;
    Host host;
    CircBuffer<char> *data;
    int lcid;
    bool received;
//    onGsReceiveFunc onGsReceive;
//    GSFunctionPointer onGsReceive;
};

#ifdef GS_ENABLE_HTTPD
enum GSHTTPDMODE {
    GSHTTPDMODE_REQUEST,
    GSHTTPDMODE_HEAD,
    GSHTTPDMODE_SPACE,
    GSHTTPDMODE_BODY,
    GSHTTPDMODE_ERROR,
    GSHTTPDMODE_WEBSOCKET,
    GSHTTPDMODE_WEBSOCKET_MASK,
    GSHTTPDMODE_WEBSOCKET_BODY,
};

struct GS_httpd {
    GSHTTPDMODE mode;
    int type;
    char *buf;  // body
    int len;  // length of buf
    char *uri;
    char *file;
    char *query;
    int length;  // content-length
    int keepalive;
    Host host;
};

typedef void (*onHttpdCgiFunc)(int cid, GS_httpd *gshttpd);

struct GS_httpd_handler {
    char *uri;
    char *dir;
    onHttpdCgiFunc onHttpCgi;
};
#endif // GS_ENABLE_HTTPD

    // ----- GSwifi.cpp -----
    /**
     * default constructor
     * @param serial
     */
    GSwifi (HardwareSerial *serial);

    /**
     * setup call once after initialization
     */
    int8_t setup();

    /**
     * polling
     */
    void poll();
    /**
     * send command
     */
    void command (const char *cmd, GSRESPONCE res, uint32_t timeout = GS_TIMEOUT);
    /**
     * reset recv responce
     */
    void resetResponse (GSRESPONCE res);
    /**
     * wait recv responce
     */
    void waitResponse (uint32_t ms);
    /**
     * associate infrastructure
     * @param sec GSSEC_OPEN, GSSEC_WEP, GSSEC_WPA_PSK, GSSEC_WPA2_PSK
     * @param ssid SSID
     * @param pass pass phrase
     * @param dhcp 0:static ip, 1:dhcp
     * @param reconnect auto re-connect time
     * @param name my host name
     * @retval 0 success
     * @retval -1 failure
     */
    int connect (GSSECURITY sec, const char *ssid, const char *pass, int dhcp = 1, int reconnect = GS_RECONNECT, char *name = NULL);
    /**
     * unassociate
     */
    int disconnect ();
    /**
     * re-connect
     */
    int reconnect ();
    /**
     * main polling
     */
    int8_t setBaud (uint32_t baud);
    /**
     * change radio region
     */
    int setRegion (int reg = GS_WREGDOMAIN);

    /**
     * use DHCP
     */
    int setAddress (char *name = NULL);
    /**
     * use static ip address
     */
    int setAddress (IpAddr ipaddr, IpAddr netmask, IpAddr gateway, IpAddr nameserver);
    /**
     * get ip address
     */
    int getAddress (IpAddr &ipaddr, IpAddr &netmask, IpAddr &gateway, IpAddr &nameserver);
    /**
     * get mac address
     */
    int getMacAddress (char *mac);
    /**
     * resolv hostname
     * @param name hostname
     * @param addr resolved ip address
     */
    int getHostByName (const char* name, IpAddr &addr);
    /**
     * resolv hostname
     * @param host.name hostname
     * @param host.ipaddr resolved ip address
     */
    int getHostByName (Host &host);
    /**
     * wifi connected
     */
    bool isConnected ();
    /**
     * status
     * @return GSSTATUS
     */
    GSSTATUS getStatus ();
    /**
     * RSSI
     * @return RSSI (dBm)
     */
    int getRssi ();

    char *getSsid () {
        return _ssid;
    }
    char *getPass () {
        return _pass;
    }
#ifndef GS_LIB_TINY
    /**
     * RF power
     * @param power 0(high)-7(low)
     */
    int setRFPower (int power);
    /**
     * power save mode
     * @param active rx radio 0:switched off, 1:always on
     * @param save power save 0:disable, 1:enable
     */
    int powerSave (int active, int save);
    /**
     * standby mode
     * @param msec wakeup after
     * wakeup after msec or alarm1/2
     * core off, save to RTC ram
     */
    int standby (int msec);
    /**
     * restore standby
     */
    int wakeup ();
    /**
     * deep sleep mode
     */
    int deepSleep ();

    /**
     * set NTP server
     * @param host SNTP server
     * @param sec time sync interval, 0:one time
     */
    int ntpdate (Host host, int sec = 0);
    /**
     * set system time
     * @param time date time (UTC)
     */
    // int setTime (time_t time);
    /**
     * get RTC time
     * @return date time (UTC)
     */
    // time_t getTime ();
    /**
     * GPIO output
     * @param port 10,11,30,31
     * @param out 0:set(high), 1:reset(low)
     */
    int gpioOut (int port, int out);
    /**
     * Web server
     */
    int provisioning (char *user, char *pass);
    /**
     * certificate
     */
    int certAdd (const char *name, const char *cert, int len);
#endif

// ----- GSwifi_sock.cpp -----
    /**
     * tcp/udp client
     * @return CID, -1:failure
     */
    int open (Host &host, GSPROTOCOL pro, int port = 0);

    int open (Host &host, GSPROTOCOL pro, onGsReceiveFunc ponGsReceive, int port = 0) {
        int cid = open(host, pro, port);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(ponGsReceive);
        return cid;
    }
    template<typename T>
    int open (Host &host, GSPROTOCOL pro, T *object, void (T::*member)(int, int), int port = 0) {
        int cid = open(host, pro, port);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(object, member);
        return cid;
    }
    /**
     * tcp/udp server
     * @return CID, -1:failure
     */
    int listen (int port, GSPROTOCOL pro);

    int listen (int port, GSPROTOCOL pro, onGsReceiveFunc ponGsReceive) {
        int cid = listen(port, pro);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(ponGsReceive);
        return cid;
    }
    template<typename T>
    int listen (int port, GSPROTOCOL pro, T *object, void (T::*member)(int, int)) {
        int cid = listen(port, pro);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(object, member);
        return cid;
    }
    /**
     * close client/server
     */
    int close (int cid);
    /**
     * send data tcp(s/c), udp(c)
     */
    int send (int cid, const char *buf, int len);
    /**
     * send data udp(s)
     */
    int send (int cid, const char *buf, int len, Host &host);
    /**
     * recv data tcp(s/c), udp(c)
     * @return length
     */
    int recv (int cid, char *buf, int len);
    /**
     * recv data udp(s)
     * @return length
     */
    int recv (int cid, char *buf, int len, Host &host);
    /**
     * tcp/udp connected
     */
    bool isConnected (int cid);

#ifdef GS_ENABLE_HTTP
// ----- GSwifi_http.cpp -----
    /**
     * http request (GET method)
     */
    int httpGet (Host &host, const char *uri, const char *user, const char *pwd, int ssl = 0, onGsReceiveFunc ponGsReceive = NULL);
    int httpGet (Host &host, const char *uri, int ssl = 0, onGsReceiveFunc ponGsReceive = NULL) {
        return httpGet (host, uri, NULL, NULL, ssl, ponGsReceive);
    }
    /**
     * http request (POST method)
     */
    int httpPost (Host &host, const char *uri, const char *body, const char *user, const char *pwd, int ssl = 0, onGsReceiveFunc ponGsReceive = NULL);
    int httpPost (Host &host, const char *uri, const char *body, int ssl = 0, onGsReceiveFunc ponGsReceive = NULL) {
        return httpPost (host, uri, body, NULL, NULL, ssl, ponGsReceive);
    }
    /**
     * websocket request (Upgrade method)
     */
    int wsOpen (Host &host, const char *uri, const char *user, const char *pwd, onGsReceiveFunc ponGsReceive = NULL);
    int wsOpen (Host &host, const char *uri, onGsReceiveFunc ponGsReceive = NULL) {
        return wsOpen (host, uri, NULL, NULL, ponGsReceive);
    }
    /**
     * send data websocket
     */
    int wsSend (int cid, const char *buf, int len, const char *mask = NULL);

    /**
     * base64 encode
     */
    int base64encode (char *input, int length, char *output, int len);
    /**
     * url encode
     */
    int urlencode (char *str, char *buf, int len);
    /**
     * url decode
     */
    int urldecode (char *str, char *buf, int len);
#endif

#ifdef GS_ENABLE_HTTPD
// ----- GSwifi_httpd.cpp -----
    /**
     * start http server
     * @param port
     */
    int httpd (int port = 80);
    /**
     * attach uri to dirctory handler
     */
    void send_httpd_error (int cid, int err);
    /**
     * attach uri to dirctory handler
     */
    int attach_httpd (const char *uri, const char *dir);
    /**
     * attach uri to cgi handler
     */
    int attach_httpd (const char *uri, onHttpdCgiFunc ponHttpCgi);
#endif

#ifdef GS_ENABLE_MDNS
    /**
     * mDNS
     */
    int8_t mDNSStart();
    int8_t mDNSRegisterHostname(const char *hostname = "");
    int8_t mDNSDeregisterHostname(const char *hostname);
    int8_t mDNSRegisterService(const char *name, const char *subtype, const char *type, const char *protocol, uint16_t port);
    int8_t mDNSDeregisterService(const char *name, const char *subtype, const char *type, const char *protocol);
    int8_t mDNSAnnounceService();
    int8_t mDNSDiscoverService(const char *subtype, const char *type, const char *protocol);
#endif // GS_ENABLE_MDNS

#ifdef DEBUG
    void dump ();
#endif

protected:
    void reset ();
    int acquireUart (int ms = GS_TIMEOUT);
    void releaseUart ();

    // inline void _gs_puts (char *s) {
    //     int i;
    //     for (i = 0; i < strlen(s); i++) {
    //         _gs_putc(s[i]);
    //     }
    // }

    void parseResponse ();
    void parseCmdResponse (char *buf);

    int x2i (char c);
    char i2x (int i);
    void isr_recv ();
    int from_hex (int ch);
    int to_hex (int code);

    void newSock (int cid, GSTYPE type, GSPROTOCOL pro);

    void newSock (int cid, GSTYPE type, GSPROTOCOL pro, onGsReceiveFunc ponGsReceive) {
        newSock(cid, type, pro);
        // _gs_sock[cid].onGsReceive.attach(ponGsReceive);
    }
    template<typename T>
    void newSock (int cid, GSTYPE type, GSPROTOCOL pro, T *object, void (T::*member)(int, int)) {
        newSock(cid, type, pro);
        // _gs_sock[cid].onGsReceive.attach(object, member);
    }

    int wait_ws (int cid, int code);

#ifdef GS_ENABLE_HTTPD
    int get_handler (char *uri);
    int httpd_request (int cid, GS_httpd *gshttpd, char *dir);
    char *mimetype (char *file);
    int strnicmp (const char *p1, const char *p2, int n);
#endif

private:
    HardwareSerial* _serial;
    bool _rts;
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368)
    LPC_UART1_TypeDef *_uart;
#elif defined(TARGET_LPC11U24)
    LPC_USART_Type *_uart;
#endif
    // DigitalInOut _reset;
    // DigitalInOut *_alarm;
    volatile bool _connect, _dhcp;
    volatile GSSTATUS _status;
    volatile bool _gs_ok, _gs_failure;
    volatile int _gs_flg;
    volatile GSRESPONCE _gs_res;
    volatile GSMODE _gs_mode;
    volatile bool _escape;
    volatile int _cid, _rssi;
    IpAddr _ipaddr, _netmask, _gateway, _nameserver, _resolv;
    Host _from, _to;
    char _mac[6];
    CircBuffer<char> _buf_cmd;
    struct GS_Socket _gs_sock[16];
    // time_t _time;
    unsigned long _time;
    GSSECURITY _sec;
    char *_ssid, *_pass;
    int _reconnect;
    // time_t _reconnect_time;
    unsigned long _reconnect_time;

#ifdef GS_ENABLE_HTTPD
    struct GS_httpd _httpd[16];
    struct GS_httpd_handler _handler[10];
    int _handler_count;

    void poll_httpd (int cid, int len);
#endif

    uint32_t timeout_start_;
    bool busy_;
    bool did_timeout_;
    void (*onTimeout_)();
    bool connected_;
    uint8_t checkActivity(uint32_t timeout_ms);
    bool setBusy(bool busy);
    void parse(uint8_t dat);
};

#endif
