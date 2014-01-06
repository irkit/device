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

#ifndef __GSWIFI_H__
#define __GSWIFI_H__

#define DEBUG

#include "Arduino.h"
#include "env.h"
#include "GSwifi_const.h"
#include "ringbuffer.h"
#include "HardwareSerialX.h"

#define CID_UNDEFINED     0xFF

// nginx took 499
#define HTTP_STATUSCODE_CLIENT_TIMEOUT 498
#define HTTP_STATUSCODE_DISCONNECT     497

/**
 * GSwifi class
 */
class GSwifi {
public:

    enum GSMETHOD {
        GSMETHOD_GET     = 0,
        GSMETHOD_POST    = 1,
        GSMETHOD_UNKNOWN = 2,
    };

    enum GSMODE {
        GSMODE_COMMAND,
        GSMODE_DATA_RX,
        GSMODE_DATA_RX_BULK,
        GSMODE_DATA_RXHTTP,
    };

    enum GSCOMMANDMODE {
        GSCOMMANDMODE_NONE,
        GSCOMMANDMODE_NORMAL,
        GSCOMMANDMODE_CONNECT,
        GSCOMMANDMODE_DHCP,
        GSCOMMANDMODE_DNSLOOKUP,
        GSCOMMANDMODE_RSSI,
        GSCOMMANDMODE_TIME,
        GSCOMMANDMODE_STATUS,
        GSCOMMANDMODE_MDNS,
        GSCOMMANDMODE_MAC,
#ifdef FACTORY_CHECKER
        GSCOMMANDMODE_VERSION,
#endif
    };

    enum GSREQUESTSTATE {
        // if request:
        // 1st line ex: "GET / HTTP/1.1"
        // if respones:
        // ex: "200 OK", "401 UNAUTHORIZED", ..
        GSREQUESTSTATE_HEAD1      = 0,
        GSREQUESTSTATE_HEAD2      = 1, // 2nd line and after
        GSREQUESTSTATE_BODY_START = 2,
        GSREQUESTSTATE_BODY       = 3,
        GSREQUESTSTATE_RECEIVED   = 4, // received whole HTTP request successfully
        GSREQUESTSTATE_ERROR      = 5,
    };

    typedef int8_t (*GSEventHandler)();
    typedef int8_t (*GSRequestHandler)(int8_t cid, int8_t routeid, GSREQUESTSTATE state);
    typedef int8_t (*GSResponseHandler)(int8_t cid, uint16_t status_code, GSREQUESTSTATE state);

    struct GSRoute {
        GSMETHOD method;
        char path[GS_MAX_PATH_LENGTH + 1];
    };

    // ----- GSwifi.cpp -----
    /**
     * default constructor
     * @param serial
     */
    GSwifi (HardwareSerialX *serial);

    /**
     * setup call once after initialization
     */
    int8_t setup( GSEventHandler on_disconnect, GSEventHandler on_reset );
    int8_t setupMDNS();

    void loop();
    void reset();

    /**
     * send command
     */
    void command (const char *cmd, GSCOMMANDMODE res, uint8_t timeout = GS_TIMEOUT);
    void escape (const char *sequence, uint8_t timeout = GS_TIMEOUT);
    /**
     * reset recv responce
     */
    void resetResponse (GSCOMMANDMODE res);
    /**
     * wait recv responce
     */
    void waitResponse (uint8_t timeout_second);
    /**
     * associate infrastructure
     * @param sec GSSEC_OPEN, GSSEC_WEP, GSSEC_WPA_PSK, GSSEC_WPA2_PSK
     * @param ssid SSID
     * @param pass pass phrase
     * @param dhcp 0:static ip, 1:dhcp
     * @param name my host name
     * @retval 0 success
     * @retval -1 failure
     */
    int8_t join (GSSECURITY sec, const char *ssid, const char *pass, int dhcp = 1, char *name = NULL);
    bool isJoined ();

    int listen (uint16_t port);
    bool isListening ();

    int8_t startLimitedAP ();
    bool isLimitedAP ();

    /**
     * unassociate
     */
    int disconnect ();
    /**
     * main polling
     */
    int8_t setBaud (uint32_t baud);

    /**
     * use DHCP
     */
    int setAddress (char *name = NULL);
    /**
     * use static ip address
     */
    // int setAddress (IpAddr ipaddr, IpAddr netmask, IpAddr gateway, IpAddr nameserver);
    /**
     * get ip address
     */
    // int getAddress (IpAddr &ipaddr, IpAddr &netmask, IpAddr &gateway, IpAddr &nameserver);
    /**
     * resolv hostname
     * @param name hostname
     * @param addr resolved ip address
     */
    // int getHostByName (const char* name, IpAddr &addr);
    /**
     * resolv hostname
     * @param host.name hostname
     * @param host.ipaddr resolved ip address
     */
    // int getHostByName (Host &host);

    /**
     * attach uri, http method pair to function
     */
    void clearRoutes ();
    int8_t registerRoute (GSMETHOD method, const char *path);
    void setRequestHandler (GSRequestHandler handler);
    int8_t writeHead (int8_t cid, uint16_t status_code);
    void write (const char *data);
    void write (const char data);
    void write (const uint8_t data);
    void write (const uint16_t data);
    int8_t writeEnd ();

    /**
     * HTTP request
     * @param timeout_second closes connection and dispatches callback with status_code 498 after this number of times caller calls onTimer()
     */
    int8_t request(GSMETHOD method, const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second, uint8_t is_binary);
    int8_t get (const char *path, GSResponseHandler handler, uint8_t timeout_second);
    int8_t postBinary (const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second);
    int8_t post (const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second);
    int8_t close(int8_t cid);

    char *hostname();

    // on timer ISR
    void onTimer();

    void bufferClear();
    bool bufferEmpty();
    char bufferGet();

#ifdef FACTORY_CHECKER
    int8_t factorySetup(uint32_t initial_baud);
    int8_t checkVersion();
    const char* appVersion();
    const char* gepsVersion();
    const char* wlanVersion();
#endif

#ifdef DEBUG
    void dump ();
#endif

private:
    HardwareSerialX*   serial_;
    bool               joined_, listening_;
    bool               limited_ap_;
    bool               gs_ok_, gs_failure_;
    int                gs_response_lines_;
    GSMODE             gs_mode_;
    GSCOMMANDMODE      gs_commandmode_;
    uint8_t            continuous_newlines_; // this should be per cid to handle multiple concurrent connections
    char               ipaddr_[16]; // xxx.xxx.xxx.xxx
    char               mac_[17];    // 00:1d:c9:01:99:99
#ifdef FACTORY_CHECKER
    char               versions_[3][8]; // APP,GEPS,WLAN
#endif

    struct GSRoute     routes_[GS_MAX_ROUTES];
    uint8_t            route_count_;

    volatile uint8_t   timeout_timer_;
    bool               busy_;
    bool               did_timeout_;
    GSEventHandler     on_disconnect_;
    GSEventHandler     on_reset_;
    struct RingBuffer  ring_buffer_;
    struct RingBuffer *_buf_cmd;

    uint16_t           cid_bitmap_; // cid:0/1
    GSRequestHandler   request_handler_;
    GSResponseHandler  handlers_[16]; // handler for each cid
    volatile uint8_t   timers_[16]; // timer for each cid
    uint16_t           content_lengths_[16]; // remaining content-length for each cid
    int8_t             connected_cid_; // this cid has just connected

    void               clear();
    uint8_t            checkActivity();
    bool               setBusy(bool busy);
    void               parseByte(uint8_t dat);
    int8_t             parseHead2(uint8_t dat, int8_t cid);
    int8_t             parseRequestLine (char *token, uint8_t token_size);
    void               parseLine ();
    void               parseCmdResponse (char *buf);
    int8_t             router (GSMETHOD method, const char *path);
    GSMETHOD           x2method(const char *method);

    int8_t             dispatchRequestHandler(int8_t cid, int8_t routeid, GSREQUESTSTATE state);
    int8_t             dispatchResponseHandler (int8_t cid, uint16_t status_code, GSREQUESTSTATE state);
    inline void        setCidIsRequest(int8_t cid, bool is_request);
    inline bool        cidIsRequest(int8_t cid);
};

#endif // __GSWIFI_H__
