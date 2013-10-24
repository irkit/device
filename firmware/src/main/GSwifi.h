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
#include "GSFunctionPointer.h"
#include "host.h"
#include "ipaddr.h"
#include "GSwifi_conf.h"
#include "ringbuffer.h"

/**
 * GSwifi class
 */
class GSwifi {
public:

    enum GSSECURITY {
        GSSECURITY_AUTO     = 0,
        GSSECURITY_NONE     = 0,
        GSSECURITY_OPEN     = 1,
        GSSECURITY_WEP      = 2,
        GSSECURITY_WPA_PSK  = 4,
        GSSECURITY_WPA2_PSK = 8,
    };

    enum GSPROTOCOL {
        GSPROTOCOL_UDP = 0,
        GSPROTOCOL_TCP = 1,
    };

    enum GSMETHOD {
        GSMETHOD_GET     = 0,
        GSMETHOD_POST    = 1,
        GSMETHOD_UNKNOWN = 2,
    };

    enum GSMODE {
        GSMODE_COMMAND,
        GSMODE_DATA_RX,
        GSMODE_DATA_RXUDP,
        GSMODE_DATA_RX_BULK,
        GSMODE_DATA_RXUDP_BULK,
        GSMODE_DATA_RXHTTP,
    };

    enum GSCOMMANDMODE {
        GSCOMMANDMODE_NONE,
        GSCOMMANDMODE_NORMAL,
        GSCOMMANDMODE_CONNECT,
        GSCOMMANDMODE_MACADDRESS,
        GSCOMMANDMODE_DHCP,
        GSCOMMANDMODE_DNSLOOKUP,
        GSCOMMANDMODE_HTTP,
        GSCOMMANDMODE_RSSI,
        GSCOMMANDMODE_TIME,
        GSCOMMANDMODE_STATUS,
    };

    enum GSPOWERSTATUS {
        GSPOWERSTATUS_READY,
        GSPOWERSTATUS_STANDBY,
        GSPOWERSTATUS_WAKEUP,
        GSPOWERSTATUS_DEEPSLEEP,
    };

    /**
     * data receive callback function
     */
    typedef void (*onGsReceiveFunc)(int8_t cid, int len);

    enum GSHTTPSTATE {
        GSHTTPSTATE_PREPARE, // line before http request ex: "CONNECT 0 1 192.168.2.1 63632"
        GSHTTPSTATE_HEAD1, // 1st line ex: "GET / HTTP/1.1"
        GSHTTPSTATE_HEAD2, // 2nd line and after
        GSHTTPSTATE_BODY,
        GSHTTPSTATE_RECEIVED, // received whole HTTP request successfully
        GSHTTPSTATE_ERROR,
        GSHTTPSTATE_RECEIVED_ERROR, // received error HTTP request successfully
    };

    struct GSServerRequest {
        int8_t      cid; // can be 1 <= cid, because cid == 0 is our http server
        int8_t      routeid;
        GSHTTPSTATE state;
        uint16_t    length;
    };

    struct GSRoute {
        GSMETHOD method;
        char path[GS_MAX_PATH_LENGTH + 1];
    };

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

    void loop();

    /**
     * send command
     */
    void command (const char *cmd, GSCOMMANDMODE res, uint32_t timeout = GS_TIMEOUT);
    /**
     * reset recv responce
     */
    void resetResponse (GSCOMMANDMODE res);
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
     * @param name my host name
     * @retval 0 success
     * @retval -1 failure
     */
    int join (GSSECURITY sec, const char *ssid, const char *pass, int dhcp = 1, char *name = NULL);
    bool isJoined ();

    int listen (GSPROTOCOL protocol,uint16_t port);
    bool isListening ();

    /**
     * unassociate
     */
    int disconnect ();
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
     * status
     * @return GSPOWERSTATUS
     */
    GSPOWERSTATUS getPowerStatus ();
    /**
     * RSSI
     * @return RSSI (dBm)
     */
    int getRssi ();

// ----- GSwifi_sock.cpp -----
    /**
     * tcp/udp client
     * @return CID, -1:failure
     */
    int open (Host &host, GSPROTOCOL pro, int port = 0);

    int open (Host &host, GSPROTOCOL pro, onGsReceiveFunc ponGsReceive, int port = 0) {
        int8_t cid = open(host, pro, port);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(ponGsReceive);
        return cid;
    }
    template<typename T>
    int8_t open (Host &host, GSPROTOCOL pro, T *object, void (T::*member)(int, int), int port = 0) {
        int8_t cid = open(host, pro, port);
        // if (cid >= 0) _gs_sock[cid].onGsReceive.attach(object, member);
        return cid;
    }
    /**
     * send data tcp(s/c), udp(c)
     */
    int send (int8_t cid, const char *buf, int len);
    /**
     * send data udp(s)
     */
    int send (int8_t cid, const char *buf, int len, Host &host);
    /**
     * recv data tcp(s/c), udp(c)
     * @return length
     */
    int recv (int8_t cid, char *buf, int len);
    /**
     * recv data udp(s)
     * @return length
     */
    int recv (int8_t cid, char *buf, int len, Host &host);
    /**
     * tcp/udp connected
     */
    bool isConnected (int8_t cid);

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

// ----- GSwifi_httpd.cpp -----
    /**
     * start http server
     * @param port
     */
    int httpd (int port = 80);

    void sendErrorResponse (int8_t cid, int err);
    /**
     * attach uri, http method pair to function
     */
    typedef int8_t (*GSRequestHandler)();
    int8_t registerRoute (GSMETHOD method, const char *path);
    void setRequestHandler (GSRequestHandler handler);
    void writeHead (uint16_t status_code);
    void write (const char *data);
    void end (const char *data);

    // TODO make accessor or rename
    struct RingBuffer *_buf_cmd;
    struct GSServerRequest _request;

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

    int8_t parseRequestLine (char *token, uint8_t token_size);
    void parseLine ();
    void parseCmdResponse (char *buf);
    int8_t router (GSMETHOD method, const char *path);

    int8_t close(int8_t cid);

    int getHandler (GSPROTOCOL method, char *uri);
    int strnicmp (const char *p1, const char *p2, int n);
    GSMETHOD x2method(const char *method);

private:
    HardwareSerial*    _serial;
    bool               _joined, _listening, _dhcp;
    GSPOWERSTATUS      _power_status;
    bool               _gs_ok, _gs_failure;
    int                _gs_response_lines;
    GSMODE             _gs_mode;
    GSCOMMANDMODE      _gs_commandmode;
    bool               _escape;
    int                _rssi;
    IpAddr             _ipaddr, _netmask, _gateway, _nameserver, _resolv;
    Host               _from, _to;
    char               _mac[6];

    struct GSRoute     _routes[GS_MAX_ROUTES];
    uint8_t            _route_count;
    GSRequestHandler   _handler;

    uint32_t           timeout_start_;
    bool               busy_;
    bool               did_timeout_;
    void               (*onTimeout_)();
    uint8_t            checkActivity(uint32_t timeout_ms);
    bool               setBusy(bool busy);
    void               parseByte(uint8_t dat);
    int8_t             dispatchRequestHandler();
};

#endif // __GSWIFI_H__
