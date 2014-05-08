/*
 * Copyright (C) 2013 gsfan, MIT License
 * Copyright (C) 2013-2014 Masakazu Ohtsuka
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
 * @brief Gainspan wi-fi module library for Arduino
 * GS1011MIC, GS1011MIP, GainSpan WiFi Breakout, etc.
 */

#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "GSwifi.h"
#include "GSwifi_const.h"
#include "MemoryFree.h"
#include "convert.h"
#include "ringbuffer.h"
#include "version.h"
#include "timer.h"
#include "base64encoder.h"
#include "log.h"
#include "cert.h"
#include <avr/eeprom.h>
#include "const.h"
#include "env.h"

extern void software_reset();
extern void wifi_hardware_reset();

#define RESPONSE_LINES_ENDED -1

#define NEXT_TOKEN_CID    0
#define NEXT_TOKEN_IP     1
#define NEXT_TOKEN_PORT   2
#define NEXT_TOKEN_LENGTH 3
#define NEXT_TOKEN_DATA   4

// memory is corruputed or something terrible that only AVR reset can rescue us
#define ASSERT(a) if (!(a)) { software_reset(); }

static char __buf_cmd[GS_CMD_SIZE + 1];

static void base64encoded( char encoded ) {
    Serial1X.print(encoded);
    GSLOG_PRINT(encoded);
}

GSwifi::GSwifi( HardwareSerialX *serial ) :
    serial_(serial),
    server_cid_(-1)
{
    _buf_cmd          = &ring_buffer_;
    ring_init( _buf_cmd, __buf_cmd, GS_CMD_SIZE + 1 );
}

// factory should issue following commands:
// % AT
// % ATE0
// % ATB=38400
// % AT&W0
// % AT&Y0
int8_t GSwifi::setup(GSEventHandler on_disconnect, GSEventHandler on_reset) {
    on_disconnect_ = on_disconnect;
    on_reset_      = on_reset;

    clear();

    // baud rate is written into default profile in factory
    serial_->begin(57600);

    // how to change baud rate (2.5.1 has only one profile, don't need to send AT&Y0)
    // setBaud(57600);
    // command(PB("ATE0",1), GSCOMMANDMODE_NORMAL);
    // command(PB("AT&W0",1), GSCOMMANDMODE_NORMAL);
    // command(PB("AT&Y0",1), GSCOMMANDMODE_NORMAL);

    // need this to ignore initial response
    command(PB("AT",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        // return here to speed up after hardware reset
        return -1;
    }

    // write this before AT&K1, cert includes XON or XOFF hex
    writeCert();

    // enable software flow control
    command(PB("AT&K1",1), GSCOMMANDMODE_NORMAL);

    // get my mac address
    command(PB("AT+NMAC=?",1), GSCOMMANDMODE_MAC);

    // disable association keep alive timer
    command(PB("AT+PSPOLLINTRL=0",1), GSCOMMANDMODE_NORMAL);

    // set system time (must be within cert valid period)
    command(PB("AT+SETTIME=01/01/2015,00:00:00",1), GSCOMMANDMODE_NORMAL);

    if (did_timeout_) {
        return -1;
    }

    return 0;
}

// mDNS setup has to be done while joined to network
int8_t GSwifi::setupMDNS() {
    char *cmd;

    // no impact on discoverability
    // command(PB("AT+DHCPSRV",1), GSCOMMANDMODE_NORMAL);

    command(PB("AT+MDNSSTART",1), GSCOMMANDMODE_NORMAL);

    // ex: "00:1d:c9:01:99:99"
    cmd = PB("AT+MDNSHNREG=IRKitXXXX,local",1);
    strcpy( cmd+13, hostname() );
    cmd[22] = ',';
    command(cmd, GSCOMMANDMODE_MDNS);

    cmd = "AT+MDNSSRVREG=IRKitXXXX,,_irkit,_tcp,local,80";
    strcpy( cmd+14, hostname() );
    cmd[23] = ',';
    command(cmd, GSCOMMANDMODE_MDNS);

    command(PB("AT+MDNSANNOUNCE",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

void GSwifi::loadLimitedAPPassword( char *password ) {
    eeprom_read_block( (void*)password,
                       (void*)EEPROM_LIMITEDAPPASSWORD_OFFSET,
                       EEPROM_LIMITEDAPPASSWORD_LENGTH );
}

#ifdef FACTORY_CHECKER

void GSwifi::saveLimitedAPPassword( const char *password ) {
    eeprom_write_block( password,
                        (void*)EEPROM_LIMITEDAPPASSWORD_OFFSET,
                        EEPROM_LIMITEDAPPASSWORD_LENGTH );
}

int8_t GSwifi::factorySetup(uint32_t initial_baud) {
    clear();

    // version 2.5.1 firmware starts with 115200 baud rate
    // version 2.5.1 (Tue, Dec 10, 2013 at 2:14 PM) firmware starts with 9600 baud rate
    // version 2.4.3 firmware starts with 9600 baud rate
    serial_->begin(initial_baud);

    // need this to ignore invalid response
    command(PB("AT",1), GSCOMMANDMODE_NORMAL, GS_TIMEOUT_SHORT);
    if (did_timeout_) {
        return -1;
    }

    setBaud(57600);

    command(PB("ATE0",1), GSCOMMANDMODE_NORMAL);

    // enable bulk data mode
    command(PB("AT+BDATA=1",1), GSCOMMANDMODE_NORMAL);

    command(PB("AT&W0",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::checkVersion() {
    command(PB("AT+VER=?",1), GSCOMMANDMODE_VERSION);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

const char *GSwifi::appVersion() {
    return versions_[ 0 ];
}

const char *GSwifi::gepsVersion() {
    return versions_[ 1 ];
}

const char *GSwifi::wlanVersion() {
    return versions_[ 2 ];
}

#endif // FACTORY_CHECKER

int8_t GSwifi::close (int8_t cid) {
    char *cmd = PB("AT+NCLOSE=0", 1);
    cmd[ 10 ] = i2x(cid);

    TIMER_STOP( timers_[cid] );

    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

void GSwifi::clear () {
    joined_         = false;
    listening_      = false;
    limited_ap_     = false;
    resetResponse(GSCOMMANDMODE_NONE);
    gs_mode_        = GSMODE_COMMAND;
    ring_clear(_buf_cmd);
    memset(ipaddr_, 0, sizeof(ipaddr_));

    for (uint8_t i=0; i<16; i++) {
        TIMER_STOP( timers_[i] );
    }
}

void GSwifi::loop() {
    TIMER_STOP( timeout_timer_ );

    checkActivity();

    for (uint8_t i=0; i<16; i++) {
        if (! cidIsRequest(i) &&
            TIMER_FIRED(timers_[i])) {
            TIMER_STOP(timers_[i]);

            GSLOG_PRINT("!E4 "); GSLOG_PRINTLN(i);

            // request timeout means GS is in trouble,
            // and it's easy to just rescue ourselves to reset
            wifi_hardware_reset();
            return;
        }
    }
}

void GSwifi::reset () {
    command(PB("AT+RESET",1), GSCOMMANDMODE_NONE, GS_TIMEOUT_NOWAIT);
}

// received a character from UART
void GSwifi::parseByte(uint8_t dat) {
    static uint8_t  next_token; // split each byte into tokens (cid,ip,port,length,data)
    static bool     escape = false;

    if (dat == ESCAPE) {
        // 0x1B : Escape
        GSLOG_PRINT("e< ");
    }
    else { // if (next_token != NEXT_TOKEN_DATA) {
        GSLOG_WRITE(dat);
    }

    if (gs_mode_ == GSMODE_COMMAND) {
        if (escape) {
            // esc
            switch (dat) {
            case 'O':
            case 'F':
                // ignore
                break;
            case 'Z':
            case 'H':
                gs_mode_   = GSMODE_DATA_RX_BULK;
                next_token = NEXT_TOKEN_CID;
                break;
            default:
                // GSLOG_PRINT("!E1 "); GSLOG_PRINTLN2(dat,HEX);
                break;
            }
            escape = false;
        }
        else {
            if (dat == ESCAPE) {
                escape = true;
            }
            else if (dat == '\n') {
                // end of line
                parseLine();
            }
            else if (dat != '\r') {
                if ( ! ring_isfull(_buf_cmd) ) {
                    ring_put(_buf_cmd, dat);
                }
                else {
                    GSLOG_PRINTLN("!E2");
                }
            }
        }
        return;
    }
    else if (gs_mode_ != GSMODE_DATA_RX_BULK) {
        return;
    }

    static uint16_t       len;
    static char           len_chars[5];
    static int8_t         current_cid;
    static GSREQUESTSTATE request_state;

    if (next_token == NEXT_TOKEN_CID) {
        // dat is cid
        current_cid = x2i(dat);
        ASSERT((0 <= current_cid) && (current_cid <= 16));

        next_token  = NEXT_TOKEN_LENGTH;
        len         = 0;
    }
    else if (next_token == NEXT_TOKEN_LENGTH) {
        // Data Length is 4 ascii char represents decimal value i.e. 1400 byte (0x31 0x34 0x30 0x30)
        len_chars[ len ++ ] = dat;
        if (len >= 4) {
            len_chars[ len ] = 0;
            len        = atoi(len_chars); // length of data
            next_token = NEXT_TOKEN_DATA;

            if (content_lengths_[ current_cid ] > 0) {
                // this is our 2nd bulk message from GS for this response
                // we already swallowed HTTP response headers,
                // following should be body
                request_state = GSREQUESTSTATE_BODY;
            }
            else {
                request_state = GSREQUESTSTATE_HEAD1;
            }
            ring_clear( _buf_cmd ); // reuse _buf_cmd to store HTTP request
        }
    }
    else if (next_token == NEXT_TOKEN_DATA) {
        len --;

        if (cidIsRequest(current_cid)) { // request against us
            static uint16_t error_code;
            static int8_t   routeid;

            switch (request_state) {
            case GSREQUESTSTATE_HEAD1:
                if (dat != '\n') {
                    if ( ! ring_isfull(_buf_cmd) ) {
                        ring_put( _buf_cmd, dat );
                    }
                    // ignore overflowed
                }
                else {
                    // end of request line
                    char    method_chars[8];
                    uint8_t method_size = 7;
                    char    path[ GS_MAX_PATH_LENGTH + 1 ];
                    uint8_t path_size   = GS_MAX_PATH_LENGTH;
                    int8_t  result      = parseRequestLine((char*)method_chars, method_size);
                    if ( result == 0 ) {
                        result = parseRequestLine((char*)path, path_size);
                    }
                    if ( result != 0 ) {
                        // couldn't detect method or path
                        request_state = GSREQUESTSTATE_ERROR;
                        error_code    = 400;
                        ring_clear(_buf_cmd);
                        break;
                    }
                    GSMETHOD method = x2method(method_chars);

                    routeid = router(method, path);
                    if ( routeid < 0 ) {
                        request_state = GSREQUESTSTATE_ERROR;
                        error_code    = 404;
                        ring_clear(_buf_cmd);
                        break;
                    }
                    request_state                   = GSREQUESTSTATE_HEAD2;
                    continuous_newlines_            = 0;
                    content_lengths_[ current_cid ] = 0;
                    ring_clear(_buf_cmd);
                }
                break;
            case GSREQUESTSTATE_HEAD2:
                if(0 == parseHead2(dat, current_cid)) {
                    request_state = GSREQUESTSTATE_BODY;
                    // dispatched once, at start of body
                    dispatchRequestHandler(current_cid, routeid, GSREQUESTSTATE_BODY_START);
                }
                break;
            case GSREQUESTSTATE_BODY:
                if (content_lengths_[ current_cid ] > 0) {
                    content_lengths_[ current_cid ] --;
                }
                if (ring_isfull(_buf_cmd)) {
                    dispatchRequestHandler(current_cid, routeid, request_state); // POST, user callback should write()
                }
                ring_put(_buf_cmd, dat);
                break;
            case GSREQUESTSTATE_ERROR:
                // skip until received whole request
                break;
            case GSREQUESTSTATE_RECEIVED:
            default:
                break;
            }

            // end of bulk transfered data
            if (len == 0) {
                gs_mode_ = GSMODE_COMMAND;

                if ( request_state == GSREQUESTSTATE_ERROR ) {
                    writeHead( current_cid, error_code );
                    writeEnd();
                    close( current_cid );
                }
                else {
                    if (content_lengths_[ current_cid ] == 0) {
                        // if Content-Length header was longer than <ESC>Z length,
                        // we wait til next bulk transfer
                        request_state = GSREQUESTSTATE_RECEIVED;
                    }
                    // user callback should write(), writeEnd() and close()
                    dispatchRequestHandler(current_cid, routeid, request_state);
                }
                ring_clear(_buf_cmd);
            }
        }
        else {
            // is request from us
            static uint16_t status_code;

            switch (request_state) {
            case GSREQUESTSTATE_HEAD1:
                if (dat != '\n') {
                    if ( ! ring_isfull(_buf_cmd) ) {
                        // ignore if overflowed
                        ring_put( _buf_cmd, dat );
                    }
                }
                else {
                    uint8_t i=0;

                    // skip 9 characters "HTTP/1.1 "
                    char trash;
                    while (i++ < 9) {
                        ring_get( _buf_cmd, &trash, 1 );
                    }

                    char status_code_chars[4];
                    status_code_chars[ 3 ] = 0;
                    int8_t count = ring_get( _buf_cmd, status_code_chars, 3 );
                    if (count != 3) {
                        // protocol error
                        // we should receive something like: "200 OK", "401 Unauthorized"
                        status_code   = 999;
                        request_state = GSREQUESTSTATE_ERROR;
                        break;
                    }
                    status_code                     = atoi(status_code_chars);
                    request_state                   = GSREQUESTSTATE_HEAD2;
                    continuous_newlines_            = 0;
                    content_lengths_[ current_cid ] = 0;
                    ring_clear(_buf_cmd);
                }
                break;
            case GSREQUESTSTATE_HEAD2:
                if(0 == parseHead2(dat, current_cid)) {
                    request_state = GSREQUESTSTATE_BODY;
                    // dispatched once, at start of body
                    dispatchResponseHandler(current_cid, status_code, GSREQUESTSTATE_BODY_START);
                }
                break;
            case GSREQUESTSTATE_BODY:
                if (content_lengths_[ current_cid ] > 0) {
                    content_lengths_[ current_cid ] --;
                }
                if (ring_isfull(_buf_cmd)) {
                    dispatchResponseHandler(current_cid, status_code, GSREQUESTSTATE_BODY);
                }
                ring_put(_buf_cmd, dat);
                break;
            case GSREQUESTSTATE_ERROR:
            case GSREQUESTSTATE_RECEIVED:
            default:
                break;
            }

            if (len == 0) {
                gs_mode_ = GSMODE_COMMAND;

                if ( request_state == GSREQUESTSTATE_ERROR ) {
                    dispatchResponseHandler(current_cid, status_code, request_state);
                }
                else {
                    if (content_lengths_[ current_cid ] == 0) {
                        // if Content-Length header was longer than <ESC>Z length,
                        // we wait til all response body received.
                        // we need to close our clientRequest before handling it.
                        // GS often locks when closing 2 connections in a row
                        // ex: POST /keys from iPhone (cid:1) -> POST /keys to server (cid:2)
                        //     response from server arrives -> close(1) -> close(2) -> lock!!
                        // the other way around: close(2) -> close(1) doesn't lock :(
                        request_state = GSREQUESTSTATE_RECEIVED;
                    }
                    dispatchResponseHandler(current_cid, status_code, request_state);
                }
                ring_clear( _buf_cmd );
            }
        } // is response
    } // (next_token == NEXT_TOKEN_DATA)
}

int8_t GSwifi::parseHead2(uint8_t dat, int8_t cid) {
    if (dat == '\n') {
        continuous_newlines_ ++;
    }
    else if (dat == '\r') {
        // preserve
    }
    else {
        continuous_newlines_ = 0;
        if ( ! ring_isfull(_buf_cmd) ) {
            // ignore if overflowed
            ring_put( _buf_cmd, dat );
        }
    }
    if (continuous_newlines_ == 1) {
        // check "Content-Length: x" header
        // if it's non 0, wait for more body data
        // otherwise disconnect after handling response
        // GS splits HTTP requests/responses into multiple bulk messages,
        // 1st ends just after headers, and 2nd contains only response body
        // "Content-Length: "    .length = 16
        // "Content-Length: 9999".length = 20
        char content_length_chars[21];
        memset( content_length_chars, 0, 21 );

        uint8_t copied = ring_get( _buf_cmd, content_length_chars, 20 );
        if ((copied >= 16) &&
            (strncmp(content_length_chars, "Content-Length: ", 16) == 0)) {
            content_length_chars[20] = 0;
            content_lengths_[ cid ] = atoi(&content_length_chars[16]);
        }
        ring_clear(_buf_cmd);
    }
    if (continuous_newlines_ == 2) {
        // if detected double (\r)\n, switch to body mode
        ring_clear(_buf_cmd);
        return 0;
    }
    return -1;
}

int8_t GSwifi::parseRequestLine (char *token, uint8_t token_size) {
    uint8_t i;
    for ( i = 0; i <= token_size; i++ ) {
        if (ring_isempty( _buf_cmd )) {
            return -1; // space didn't appear
        }
        ring_get( _buf_cmd, token+i, 1 );
        if (token[i] == ' ') {
            token[i] = '\0';
            break;
        }
    }
    if ( i == token_size + 1 ) {
        return -1; // couldnt detect token
    }
    return 0;
}

int8_t GSwifi::router (GSMETHOD method, const char *path) {
    if (method == GSMETHOD_UNKNOWN) {
        return -1;
    }

    uint8_t i;
    for (i = 0; i < route_count_; i ++) {
        if ((method == routes_[i].method) &&
            (strncmp(path, routes_[i].path, GS_MAX_PATH_LENGTH) == 0)) {
            return i;
        }
    }
    return -1;
}

void GSwifi::clearRoutes () {
    route_count_ = 0;
}

int8_t GSwifi::registerRoute (GSwifi::GSMETHOD method, const char *path) {
    if ( route_count_ >= GS_MAX_ROUTES ) {
        return -1;
    }
    routes_[ route_count_ ].method = method;
    sprintf( routes_[ route_count_ ].path, "%s", path );
    route_count_ ++;
}

void GSwifi::setRequestHandler (GSRequestHandler handler) {
    request_handler_ = handler;
}

// request against us
int8_t GSwifi::dispatchRequestHandler (int8_t cid, int8_t routeid, GSREQUESTSTATE state) {
    return request_handler_(cid, routeid, state);
}

// response to our request
int8_t GSwifi::dispatchResponseHandler (int8_t cid, uint16_t status_code, GSREQUESTSTATE state) {
    return handlers_[ cid ](cid, status_code, state);
}

inline void GSwifi::setCidIsRequest(int8_t cid, bool is_request) {
    if (is_request) {
        cid_bitmap_ |= _BV(cid);
    }
    else {
        cid_bitmap_ &= ~_BV(cid);
    }
}

inline bool GSwifi::cidIsRequest(int8_t cid) {
    return cid_bitmap_ & _BV(cid);
}

int8_t GSwifi::writeHead (int8_t cid, uint16_t status_code) {
    char *cmd = PB("S0",1);
    cmd[ 1 ]  = i2x(cid);

    escape( cmd );

    serial_->print(P("HTTP/1.0 "));
    char *msg;
    switch (status_code) {
    case 200:
        msg = P("200 OK");
        break;
    case 400:
        msg = P("400 Bad Request");
        break;
    case 404:
        msg = P("404 Not Found");
        break;
    case 500:
    default:
        msg = P("500 Internal Server Error");
        break;
    }

    serial_->println(msg);
    serial_->print("Access-Control-Allow-Origin: *\r\nServer: IRKit/");
    serial_->println(version);
    serial_->println(P("Content-Type: text/plain\r\n"));
}

void GSwifi::write (const char *data) {
    serial_->print(data);
}

void GSwifi::write (const char data) {
    serial_->print(data);
}

void GSwifi::write (const uint8_t data) {
    serial_->print(data);
}

void GSwifi::write (const uint16_t data) {
    serial_->print(data);
}

int8_t GSwifi::writeEnd () {
    escape( "E" );
}

void GSwifi::writeCert() {
    // Binary format, store in memory
    command( "AT+TCERTADD=cacert,0,753,1", GSCOMMANDMODE_NORMAL );

    resetResponse(GSCOMMANDMODE_NORMAL);

    // ESCAPE 'W' is written in pgm too (thus +2)
    for (uint16_t i=0; i<753 + 2; i++) {
        uint8_t read = pgm_read_byte_near(der + i);
        serial_->write( read );
        Serial.print( read, HEX );
    }

    setBusy(true);
    waitResponse(GS_TIMEOUT);
}

GSwifi::GSMETHOD GSwifi::x2method(const char *method) {
    if (strncmp(method, "GET", 3) == 0) {
        return GSMETHOD_GET;
    }
    else if (strncmp(method, "POST", 4) == 0) {
        return GSMETHOD_POST;
    }
    return GSMETHOD_UNKNOWN;
}

void GSwifi::parseLine () {
    char buf[GS_CMD_SIZE];

    while (! ring_isempty(_buf_cmd)) {
        // received "\n"
        uint8_t i = 0;
        while ( (! ring_isempty(_buf_cmd)) &&
                (i < sizeof(buf) - 1) ) {
            ring_get( _buf_cmd, &buf[i], 1 );
            if (buf[i] == '\n') {
                break;
            }
            i ++;
        }
        if (i == 0) continue;
        buf[i] = 0;

        if ( (gs_mode_ == GSMODE_COMMAND) &&
             (gs_commandmode_ != GSCOMMANDMODE_NONE) ) {
            parseCmdResponse(buf);
        }

        if (strncmp(buf, P("CONNECT "), 8) == 0 && buf[8] >= '0' && buf[8] <= 'F' && buf[9] != 0) {
            // connect from client
            // CONNECT 0 1 192.168.2.1 63632
            // 1st cid is our http server's
            // 2nd cid is for client
            // next line will be "[ESC]Z10140GET / ..."

            int8_t cid = x2i(buf[10]); // 2nd cid = HTTP client cid
            ASSERT((0 <= cid) && (cid <= 16));

            setCidIsRequest(cid, true); // request against us
            content_lengths_[ cid ] = 0;

            // don't close other connections,
            // other connections close theirselves on their turn
            // ignore client's IP and port
        }
        else if (strncmp(buf, P("DISCONNECT "), 11) == 0) {
            int8_t cid = x2i(buf[11]);
            ASSERT((0 <= cid) && (cid <= 16));

            if (cid == server_cid_) {
                // if it's our server, this is fatal
                reset();
            }
            else if (! cidIsRequest(cid) ) {
                // request *from* us was disconnected (ex: polling)
                dispatchResponseHandler(cid, HTTP_STATUSCODE_DISCONNECT, GSREQUESTSTATE_ERROR);
            }
        }
        else if (strncmp(buf, P("DISASSOCIATED"), 13) == 0 ||
                 strncmp(buf, P("Disassociat"), 11) == 0) {
            // Disassociated
            // Disassociation Event
            clear();
            on_disconnect_();
            gs_failure_ = true;
        }
        else if (strncmp(buf, P("APP Reset"), 9) == 0 ||
                 strncmp(buf, P("Serial2WiFi APP"), 15) == 0) {
            // APP Reset-APP SW Reset
            // APP Reset-Wlan Except
            // Serial2Wifi APP
            on_reset_();
            gs_failure_ = true;
        }
        // else if (strncmp(buf, P("Out of StandBy-Timer"), 20) == 0 ||
        //          strncmp(buf, P("Out of StandBy-Alarm"), 20) == 0) {
        // }
        // else if (strncmp(buf, P("Out of Deep Sleep"), 17) == 0 ) {
        // }
    }
}

void GSwifi::parseCmdResponse (char *buf) {
    GSLOG_PRINT(P("c< ")); GSLOG_PRINTLN(buf);

    if (strncmp(buf, "OK", 3) == 0) {
        gs_ok_ = true;
    }
    else if (strncmp(buf, "ERROR", 5) == 0) {
        gs_failure_ = true;
    }

    switch(gs_commandmode_) {
    case GSCOMMANDMODE_NORMAL:
        gs_response_lines_ = RESPONSE_LINES_ENDED;
        break;
    case GSCOMMANDMODE_CONNECT:
        if (strncmp(buf, P("CONNECT "), 8) == 0 && buf[9] == 0) {
            // both "AT+NSTCP=port" and "AT+NCTCP=ip,port" responds with
            // CONNECT <cid>

            // following lines are needed for client cids,
            // but won't be a problem if we run it for a server cid

            gs_response_lines_ = RESPONSE_LINES_ENDED;

            int8_t cid = x2i(buf[8]);
            ASSERT((0 <= cid) && (cid <= 16));

            setCidIsRequest(cid, false);
            content_lengths_[ cid ] = 0;

            // don't close other connections,
            // other connections close theirselves on their turn
            
            TIMER_STOP(timers_[cid]);
            connected_cid_ = cid;
        }
        break;
    case GSCOMMANDMODE_DHCP:
        if (gs_response_lines_ == 0 && strstr(buf, P("SubNet"))) {
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1) {
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_DNSLOOKUP:
        if (strncmp(buf, P("IP:"), 3) == 0) {
            sprintf( ipaddr_, "%s", &buf[3] );
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_MDNS:
        if (gs_response_lines_ == 0) {
            // 1st line is just OK
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1) {
            // 2nd line is something like:
            // " Registration Success!! for RR: IRKitXXXX"
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_MAC:
        if ((gs_response_lines_ == 0) && (buf[2] == ':')) {
            // 1st line is something like:
            // "00:1d:c9:01:99:99"
            for (uint8_t i=0; i<17; i++) {
                if (buf[i] >= 'a') {
                    buf[i] -= 0x20; // a -> A
                }
            }
            sprintf( mac_, "%s", buf );
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1) {
            // 2nd line is just "OK"
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
#ifdef FACTORY_CHECKER
    case GSCOMMANDMODE_VERSION:
        // expects something like:
        // S2W APP VERSION=2.4.3
        // S2W GEPS VERSION=2.4.3
        // S2W WLAN VERSION=2.4.1
        if (gs_response_lines_ == 0) {
            memset(versions_, 0, sizeof(versions_));
            snprintf(versions_[0], 8, buf+16);
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1) {
            snprintf(versions_[1], 8, buf+17);
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 2) {
            snprintf(versions_[2], 8, buf+17);
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
#endif
    default:
        break;
    }

    return;
}

void GSwifi::command (const char *cmd, GSCOMMANDMODE res, uint8_t timeout_second) {
    // GSLOG_PRINT(P("F: 0x")); GSLOG_PRINTLN2( freeMemory(), HEX );
    GSLOG_PRINT(P("c> "));

    resetResponse(res);

    serial_->println(cmd);

    GSLOG_PRINTLN(cmd);

    if (timeout_second == GS_TIMEOUT_NOWAIT) {
        return;
    }

    setBusy(true);
    waitResponse(timeout_second);
}

void GSwifi::escape (const char *cmd, uint8_t timeout_second) {
    GSLOG_PRINT(P("e> "));

    resetResponse(GSCOMMANDMODE_NONE);

    serial_->write( ESCAPE );
    serial_->print(cmd); // without ln

    GSLOG_PRINTLN(cmd);
}

void GSwifi::resetResponse (GSCOMMANDMODE res) {
    gs_ok_             = false;
    gs_failure_        = false;
    gs_response_lines_ = 0;
    gs_commandmode_    = res;
}

bool GSwifi::setBusy(bool busy) {
    if (busy) {
        did_timeout_  = false;
    }
    return busy_ = busy;
}

uint8_t GSwifi::checkActivity() {
    static uint8_t continuous_timeouts_ = 0;

    while ( serial_->available() &&
            ! TIMER_FIRED(timeout_timer_) ) {

        parseByte( serial_->read() );

        if ( gs_failure_ ||
             (gs_ok_ &&
              (gs_response_lines_ == RESPONSE_LINES_ENDED ||
               gs_commandmode_    == GSCOMMANDMODE_NONE)) ) {
            gs_commandmode_      = GSCOMMANDMODE_NONE;
            continuous_timeouts_ = 0;
            setBusy(false);
            break;
        }
    }

    if ( busy_ &&
         TIMER_FIRED(timeout_timer_) ) {
        TIMER_STOP(timeout_timer_);
        GSLOG_PRINTLN("!E24");
        did_timeout_          = true;
        continuous_timeouts_ ++;
        setBusy(false);
    }

    // need a GS hardware reset, which we issue in setup()
    ASSERT( continuous_timeouts_ <= 5 );

    return busy_;
}

void GSwifi::waitResponse (uint8_t timeout_second) {
    TIMER_START( timeout_timer_, timeout_second );

    while ( checkActivity() ) {
    }
}

int8_t GSwifi::join (GSSECURITY sec, const char *ssid, const char *pass, int dhcp, char *name_) {
    char *cmd;
    uint8_t offset;

    if (joined_) {
        return -1;
    }

    disconnect();

    // infrastructure mode
    command(PB("AT+WM=0",1), GSCOMMANDMODE_NORMAL);

    // set DHCP name with mac address
    cmd = PB("AT+NDHCP=1,%",1);
    strcpy( cmd+11, hostname());
    command(cmd, GSCOMMANDMODE_NORMAL);

    switch (sec) {
    case GSSECURITY_NONE:
    case GSSECURITY_OPEN:
    case GSSECURITY_WEP:
        command(PB("AT+WSEC=0",1), GSCOMMANDMODE_NORMAL);

        cmd = PB("AT+WAUTH=%",1);
        cmd[ 9 ] = i2x(sec);
        command(cmd, GSCOMMANDMODE_NORMAL);
        if (sec != GSSECURITY_NONE) {
            // key are either 10 or 26 hexadecimal digits corresponding to a 40- bit or 104-bit key
            cmd = PB("AT+WWEP1=%",1);
            strcpy(cmd+9, pass);
            command(cmd, GSCOMMANDMODE_NORMAL);
        }
        cmd = PB("AT+WA=%",1);
        strcpy(cmd+6, ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);

        // normal people don't understand difference between wep open/shared.
        // so we try shared first, and when it failed, try open
        if (gs_failure_ && (sec == GSSECURITY_WEP)) {
            return join(GSSECURITY_OPEN, ssid, pass, dhcp, name_);
        }
        break;
    case GSSECURITY_WPA_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);

        cmd = PB("AT+WWPA=%",1);
        strcpy( cmd+8, pass);
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

        cmd = PB("AT+WA=%",1);
        strcpy( cmd+6, ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);
        break;
    case GSSECURITY_WPA2_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);

        cmd = PB("AT+WPAPSK=%,%",1);
        offset = 10;
        strcpy( cmd+offset, ssid );

        offset += strlen(ssid);
        cmd[ offset ] = ',';

        offset += 1;
        strcpy( cmd+offset, pass );
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

        cmd = PB("AT+WA=%",1);
        strcpy( cmd+6, ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);

        if (gs_failure_) {
            return join(GSSECURITY_WPA_PSK, ssid, pass, dhcp, name_);
        }
        break;
    default:
        return -1;
    }

    if (did_timeout_) {
        return -1;
    }
    if (gs_failure_) {
        return -1;
    }

    joined_ = true;
    return 0;
}

int GSwifi::listen(uint16_t port) {
    char cmd[15];

    // longest: "AT+NSTCP=65535"
    sprintf(cmd, P("AT+NSTCP=%d"), port);
    command(cmd, GSCOMMANDMODE_CONNECT);
    if (did_timeout_) {
        return -1;
    }

    server_cid_ = connected_cid_;
    listening_  = true;

    return 0;
}

int8_t GSwifi::startLimitedAP () {
    char *cmd;

    disconnect();

    command(PB("AT+NSET=192.168.1.1,255.255.255.0,192.168.1.1",1), GSCOMMANDMODE_NORMAL);

    // AT+WPAPSK=IRKitXXXX,0123456789
    cmd = PB("AT+WPAPSK=%,%",1);
    strcpy( cmd+10, hostname() );
    cmd[19] = ',';
    loadLimitedAPPassword( cmd + 20 );
    command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

    // WPA2
    command(PB("AT+WSEC=8",1),           GSCOMMANDMODE_NORMAL);
    command(PB("AT+WAUTH=0",1),          GSCOMMANDMODE_NORMAL);

    // limited AP
    command(PB("AT+WM=2",1),             GSCOMMANDMODE_NORMAL);

    command(PB("AT+DHCPSRVR=1",1),       GSCOMMANDMODE_NORMAL);

    // start
    cmd = PB("AT+WA=%",1);
    strcpy( cmd+6, hostname() );
    command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

    if (did_timeout_) {
        return -1;
    }

    limited_ap_ = true;

    return 0;
}

int GSwifi::disconnect () {
    joined_     = false;
    listening_  = false;
    limited_ap_ = false;
    command(PB("AT+NCLOSEALL",1),  GSCOMMANDMODE_NORMAL);
    command(PB("AT+WD",1),         GSCOMMANDMODE_NORMAL);
    command(PB("AT+DHCPSRVR=0",1), GSCOMMANDMODE_NORMAL);
    return 0;
}

bool GSwifi::isJoined () {
    return joined_;
}

bool GSwifi::isListening () {
    return listening_;
}

bool GSwifi::isLimitedAP () {
    return limited_ap_;
}

#ifdef FACTORY_CHECKER

// 4.2.1 UART Parameters
// Allowed baud rates include: 9600, 19200, 38400, 57600, 115200, 230400,460800 and 921600.
// The new UART parameters take effect immediately. However, they are stored in RAM and will be lost when power is lost unless they are saved to a profile using AT&W (section 4.6.1). The profile used in that command must also be set as the power-on profile using AT&Y (section 4.6.3).
// This command returns the standard command response (section 4) to the serial interface with the new UART configuration.
int8_t GSwifi::setBaud (uint32_t baud) {
    char cmd[11];

    // longest: "ATB=921600"
    sprintf(cmd, P("ATB=%ld"), baud);
    serial_->println(cmd);
    GSLOG_PRINT(P("c> ")); GSLOG_PRINTLN(cmd);

    delay(1000);

    serial_->end();
    serial_->begin(baud);

    delay(1000);

    // Skip 1st "ERROR: INVALID INPUT" after baud rate change
    command("", GSCOMMANDMODE_NORMAL);

    return 0;
}

#endif

int8_t GSwifi::setRegDomain (char regdomain) {
    char *cmd = PB("AT+WREGDOMAIN=%",1);
    cmd[ 14 ] = regdomain;
    command(cmd, GSCOMMANDMODE_NORMAL);
    return 0;
}

// returns -1 on error, >0 on success
int8_t GSwifi::request(GSwifi::GSMETHOD method, const char *path, const char *body, uint16_t length, GSwifi::GSResponseHandler handler, uint8_t timeout, uint8_t is_binary) {
    char cmd[ GS_CMD_SIZE ];
    char *cmd2;

#ifdef TESTER
    // Tester only requests against limited AP test target
    // DNSLOOKUP should fail, because it's in limitedAP mode, ignore it.
    sprintf( ipaddr_, "%s", "192.168.1.1" );
#else
    sprintf(cmd, P("AT+DNSLOOKUP=%s"), DOMAIN);
    command(cmd, GSCOMMANDMODE_DNSLOOKUP);
#endif

    // don't fail fatally on dnslookup failure,
    // we cache our server's ipaddress anyway
    // it happens randomly
    if (ipaddr_[ 0 ] == 0) {
        // but if it has not been resolved yet,
        // we must be not connected to internet,
        // let's show yellow blink on colorled
        clear();
        on_disconnect_();
        return -1;
    }

#ifdef TESTER
    sprintf(cmd, P("AT+NCTCP=%s,80"), ipaddr_);
#else
    sprintf(cmd, P("AT+NCTCP=%s,443"), ipaddr_);
#endif

    connected_cid_ = CID_UNDEFINED;
    command(cmd, GSCOMMANDMODE_CONNECT);
    int8_t cid = connected_cid_;
    char   xid = i2x(cid);

    if (did_timeout_) {
        return -1;
    }
    if ((0 <= cid) && (cid <= 16)) {
        // OK
    }
    else {
        return -1;
    }

    handlers_[ cid ] = handler;

#ifndef TESTER
    cmd2 = PB("AT+SSLOPEN=%,cacert",1);
    cmd2[ 11 ] = xid;
    command(cmd2, GSCOMMANDMODE_NORMAL);
#endif

    // disable TCP_MAXRT and TCP_KEEPALIVE_X, because these commands takes some time to issue,
    // and GS1011MIPS denies short TCP_KEEPALIVE_INTVL,
    // and our server (currently Heroku) disconnects connections after 30seconds,
    // and it's difficult to handle incoming requests while sending these.

    // TCP_MAXRT = 10
    // AT+SETSOCKOPT=0,6,10,10,4
    // cmd2 = PB("AT+SETSOCKOPT=%,6,10,10,4",1);
    // cmd2[ 14 ] = xid;
    // command(cmd2, GSCOMMANDMODE_NORMAL);

    // Enable TCP_KEEPALIVE on this socket
    // AT+SETSOCKOPT=0,65535,8,1,4
    // cmd2 = PB("AT+SETSOCKOPT=%,65535,8,1,4",1);
    // cmd2[ 14 ] = xid;
    // command(cmd2, GSCOMMANDMODE_NORMAL);

    // TCP_KEEPALIVE_PROBES = 2
    // AT+SETSOCKOPT=0,6,4005,2,4
    // cmd2 = PB("AT+SETSOCKOPT=%,6,4005,2,4",1);
    // cmd2[ 14 ] = xid;
    // command(cmd2, GSCOMMANDMODE_NORMAL);

    // TCP_KEEPALIVE_INTVL = 150
    // AT+SETSOCKOPT=0,6,4001,150,4
    // mysteriously, GS1011MIPS denies with "ERROR: INVALID INPUT" for seconds less than 150
    // cmd2 = PB("AT+SETSOCKOPT=%,6,4001,150,4",1);
    // cmd2[ 14 ] = xid;
    // command(cmd2, GSCOMMANDMODE_NORMAL);
    // if (did_timeout_) {
    //     return -1;
    // }

    cmd2 = PB("S0",1);
    cmd2[ 1 ]  = xid;
    escape( cmd2 );

    if (method == GSMETHOD_POST) {
        serial_->print(P("POST "));
    }
    else {
        serial_->print(P("GET "));
    }
    serial_->print(path);
    serial_->print(P(" HTTP/1.1\r\nUser-Agent: IRKit/"));
    serial_->println(version);

    serial_->println("Host: " DOMAIN);

    if (method == GSMETHOD_POST) {
        serial_->print(P("Content-Length: "));
        serial_->println( is_binary ? base64_length(length) : length );

        serial_->println("Content-Type: application/x-www-form-urlencoded\r\n");
        if (is_binary) {
            base64_encode((const uint8_t*)body, length, &base64encoded);
        }
        else {
            serial_->print(body);
        }
    }
    else {
        serial_->println();
    }

    // we're long polling here, to receive other events, we're going back to our main loop
    // ignore timeout, we always timeout here
    escape( "E" );

    TIMER_START(timers_[cid], timeout);

    return cid;
}

int8_t GSwifi::get(const char *path, GSResponseHandler handler, uint8_t timeout_second) {
    return request( GSMETHOD_GET, path, NULL, 0, handler, timeout_second, false );
}

int8_t GSwifi::post(const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second) {
    return request( GSMETHOD_POST, path, body, length, handler, timeout_second, false );
}

int8_t GSwifi::postBinary(const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second) {
    return request( GSMETHOD_POST, path, body, length, handler, timeout_second, true );
}

char* GSwifi::hostname() {
    char *ret = PB("IRKit%%%%", 2);
    ret[ 5 ] = mac_[12];
    ret[ 6 ] = mac_[13];
    ret[ 7 ] = mac_[15];
    ret[ 8 ] = mac_[16];
    return ret;
}

void GSwifi::bufferClear() {
    ring_clear(_buf_cmd);
}

bool GSwifi::bufferEmpty() {
    return ring_isempty(_buf_cmd);
}

char GSwifi::bufferGet() {
    char letter;
    ring_get(_buf_cmd, &letter, 1);
    return letter;
}

// careful, called from ISR
void GSwifi::onTimer() {
    for (int i=0; i<16; i++) {
        if ( ! cidIsRequest(i) &&
             TIMER_RUNNING(timers_[i]) ) {
            TIMER_COUNTDOWN(timers_[i]);
        }
    }

    TIMER_TICK( timeout_timer_ );
}

void GSwifi::dump () {
    // GSLOG_PRINT(P("joined_:"));            GSLOG_PRINTLN(joined_);
    // GSLOG_PRINT(P("did_timeout_:"));       GSLOG_PRINTLN(did_timeout_);
    // GSLOG_PRINT(P("gs_response_lines_:")); GSLOG_PRINTLN(gs_response_lines_);
    // GSLOG_PRINT(P("gs_mode_:"));           GSLOG_PRINTLN(gs_mode_);
    // GSLOG_PRINT(P("timeout_timer_:"));     GSLOG_PRINTLN(timeout_timer_);
    // for (uint8_t i=0; i<GS_MAX_ROUTES; i++) {
    //     GSLOG_PRINTLN(routes_[i].method);
    //     GSLOG_PRINTLN(routes_[i].path);
    // }
}
