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

#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "GSwifi.h"
#include "GSwifi_const.h"
#include "MemoryFree.h"
#include "convert.h"
#include "ringbuffer.h"
#include "version.h"
#include "timer.h"

#define DOMAIN "api.getirkit.com"

#define RESPONSE_LINES_ENDED -1

#define NEXT_TOKEN_CID    0
#define NEXT_TOKEN_IP     1
#define NEXT_TOKEN_PORT   2
#define NEXT_TOKEN_LENGTH 3
#define NEXT_TOKEN_DATA   4

#define ESCAPE            0x1B

static char __buf_cmd[GS_CMD_SIZE + 1];

GSwifi::GSwifi( HardwareSerialX *serial ) :
    serial_(serial)
{
    _buf_cmd          = &ring_buffer_;
    ring_init( _buf_cmd, __buf_cmd, GS_CMD_SIZE + 1 );
    route_count_      = 0;
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

    serial_->begin(57600);

    // how to change baud rate
    // setBaud(57600);
    // command(PB("AT&W0",1), GSCOMMANDMODE_NORMAL);
    // command(PB("AT&Y0",1), GSCOMMANDMODE_NORMAL);

    // need this to ignore initial response
    command(PB("AT",1), GSCOMMANDMODE_NORMAL);

    // enable bulk data mode
    command(PB("AT+BDATA=1",1), GSCOMMANDMODE_NORMAL);

    // enable software flow control
    command(PB("AT&K1",1), GSCOMMANDMODE_NORMAL);

    // get my mac address
    command(PB("AT+NMAC=?",1), GSCOMMANDMODE_MAC);

    // disable association keep alive timer
    command(PB("AT+PSPOLLINTRL=0",1), GSCOMMANDMODE_NORMAL);
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
    cmd = PB("AT+MDNSHNREG=IRKit%%%%,local",1);
    cmd[18] = mac_[12];
    cmd[19] = mac_[13];
    cmd[20] = mac_[15];
    cmd[21] = mac_[16];
    command(cmd, GSCOMMANDMODE_MDNS);

    cmd = PB("AT+MDNSSRVREG=IRKit%%%%,,_irkit,_tcp,local,80",1);
    cmd[19] = mac_[12];
    cmd[20] = mac_[13];
    cmd[21] = mac_[15];
    cmd[22] = mac_[16];
    command(cmd, GSCOMMANDMODE_MDNS);

    command(PB("AT+MDNSANNOUNCE",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::close (uint8_t cid) {
    char *cmd = PB("AT+NCLOSE=0", 1);
    cmd[ 10 ]  = cid + '0';

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
    resetResponse(GSCOMMANDMODE_NONE);
    gs_mode_        = GSMODE_COMMAND;
    ring_clear(_buf_cmd);

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

            Serial.print(("!E4 ")); Serial.println(i);

            dispatchResponseHandler(i, HTTP_STATUSCODE_CLIENT_TIMEOUT, GSREQUESTSTATE_ERROR);
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

    if ((dat > 0x10) && (dat < 0x20)) {
        // 0x1B : Escape
        // 0x11 : XON
        // 0x13 : XOFF
        Serial.print(dat,HEX); Serial.print('|');
    }
    else { // if (next_token != NEXT_TOKEN_DATA) {
        Serial.write(dat);
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
                Serial.print(("!E1 ")); Serial.println(dat,HEX);
                break;
            }
            escape = false;
        }
        else {
            if (dat == 0x1b) {
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
                    Serial.println(("!E2"));
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
    static uint8_t        current_cid;
    static GSREQUESTSTATE request_state;

    if (next_token == NEXT_TOKEN_CID) {
        // dat is cid
        current_cid = x2i(dat);
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
        static uint8_t continuous_newlines = 0;

        if (cidIsRequest(current_cid)) {
            static uint16_t error_code;
            static int8_t   routeid;

            switch (request_state) {
            case GSREQUESTSTATE_HEAD1:
                if (dat != '\n') {
                    // TODO: document max request line length
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
                        Serial.print("M:"); Serial.println(method_chars);
                        result = parseRequestLine((char*)path, path_size);
                    }
                    if ( result != 0 ) {
                        // couldn't detect method or path
                        request_state = GSREQUESTSTATE_ERROR;
                        error_code    = 400;
                        ring_clear(_buf_cmd);
                        break;
                    }
                    Serial.print("P:"); Serial.println(path);
                    GSMETHOD method = x2method(method_chars);

                    routeid = router(method, path);
                    if ( routeid < 0 ) {
                        request_state = GSREQUESTSTATE_ERROR;
                        error_code    = 404;
                        ring_clear(_buf_cmd);
                        break;
                    }
                    request_state                   = GSREQUESTSTATE_HEAD2;
                    continuous_newlines             = 0;
                    content_lengths_[ current_cid ] = 0;
                    ring_clear(_buf_cmd);
                }
                break;
            case GSREQUESTSTATE_HEAD2:
                if (dat == '\n') {
                    continuous_newlines ++;
                }
                else if (dat == '\r') {
                    // preserve
                }
                else {
                    continuous_newlines = 0;
                    if ( ! ring_isfull(_buf_cmd) ) {
                        // ignore if overflowed
                        ring_put( _buf_cmd, dat );
                    }
                }
                if (continuous_newlines == 1) {
                    // check "Content-Length: x" header
                    // if it's non 0, wait for more body data
                    // otherwise disconnect after handling response
                    // GS splits HTTP requests/responses into multiple bulk messages,
                    // 1st ends just after headers, and 2nd contains only response body
                    // "Content-Length: "    .length = 16
                    // "Content-Length: 9999".length = 20
                    char content_length_chars[21];
                    uint8_t copied = ring_get( _buf_cmd, &content_length_chars[0], 20 );
                    if ((copied >= 16) &&
                        (strncmp(content_length_chars, "Content-Length: ", 16) == 0)) {
                        content_length_chars[20] = 0;
                        content_lengths_[ current_cid ] = atoi(&content_length_chars[16]);
                        Serial.print("C: "); Serial.println(content_lengths_[current_cid]);
                    }
                    ring_clear(_buf_cmd);
                }
                if (continuous_newlines == 2) {
                    // if detected double (\r)\n, switch to body mode
                    request_state = GSREQUESTSTATE_BODY;
                    ring_clear(_buf_cmd);
                    Serial.println("rq1");
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
                Serial.println("rq2");

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
                    // user callback should write(), end() and close()
                    dispatchRequestHandler(current_cid, routeid, request_state);
                }
                ring_clear(_buf_cmd);
            }
        } // is request
        else {
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
                        // TODO handle this
                        status_code   = 999;
                        request_state = GSREQUESTSTATE_ERROR;
                        break;
                    }
                    ring_clear(_buf_cmd);
                    status_code                     = atoi(status_code_chars);
                    request_state                   = GSREQUESTSTATE_HEAD2;
                    continuous_newlines             = 0;
                    content_lengths_[ current_cid ] = 0;
                    Serial.print("S:"); Serial.println(status_code);
                }
                break;
            case GSREQUESTSTATE_HEAD2:
                // we don't read *any* headers except 1st request line.
                // "Expect: 100-continue" doesn't work
                if (dat == '\n') {
                    continuous_newlines ++;
                }
                else if (dat == '\r') {
                    // preserve
                }
                else {
                    continuous_newlines = 0;
                    if ( ! ring_isfull(_buf_cmd) ) {
                        // ignore if overflowed
                        ring_put( _buf_cmd, dat );
                    }
                }
                if (continuous_newlines == 1) {
                    // check "Content-Length: x" header
                    // if it's non 0, wait for more body data
                    // otherwise disconnect after handling response
                    // GS splits HTTP requests/responses into multiple bulk messages,
                    // 1st ends just after headers, and 2nd contains only response body
                    // "Content-Length: "    .length = 16
                    // "Content-Length: 9999".length = 20
                    char content_length_chars[21];
                    uint8_t copied = ring_get( _buf_cmd, content_length_chars, 20 );
                    if ((copied >= 16) &&
                        (strncmp(content_length_chars, "Content-Length: ", 16) == 0)) {
                        content_length_chars[20] = 0;
                        content_lengths_[ current_cid ] = atoi(&content_length_chars[16]);
                        Serial.print("C: "); Serial.println(content_lengths_[current_cid]);
                    }
                    ring_clear(_buf_cmd);
                }
                if (continuous_newlines == 2) {
                    // if detected double (\r)\n, switch to body mode
                    request_state = GSREQUESTSTATE_BODY;
                    ring_clear(_buf_cmd);
                    Serial.println("rs2");
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
            Serial.print("R:"); Serial.println(i);
            return i;
        }
    }
    return -1;
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

int8_t GSwifi::dispatchRequestHandler (uint8_t cid, int8_t routeid, GSREQUESTSTATE state) {
    return request_handler_(cid, routeid, state);
}

int8_t GSwifi::dispatchResponseHandler (uint8_t cid, uint16_t status_code, GSREQUESTSTATE state) {
    return handlers_[ cid ](cid, status_code, state);
}

inline void GSwifi::setCidIsRequest(uint8_t cid, bool is_request) {
    if (is_request) {
        cid_bitmap_ |= _BV(cid);
    }
    else {
        cid_bitmap_ &= ~_BV(cid);
    }
}

inline bool GSwifi::cidIsRequest(uint8_t cid) {
    return cid_bitmap_ & _BV(cid);
}

int8_t GSwifi::writeHead (uint8_t cid, uint16_t status_code) {
    char *cmd = PB("S0",1);
    cmd[ 1 ]  = cid + '0';

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
    serial_->println(P("Access-Control-Allow-Origin: *"));
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

GSwifi::GSMETHOD GSwifi::x2method(const char *method) {
    if (strncmp(method, P("GET"), 3) == 0) {
        return GSMETHOD_GET;
    }
    else if (strncmp(method, P("POST"), 4) == 0) {
        return GSMETHOD_POST;
    }
    return GSMETHOD_UNKNOWN;
}

void GSwifi::parseLine () {
    uint8_t i;
    char buf[GS_CMD_SIZE];

    while (! ring_isempty(_buf_cmd)) {
        // received "\n"
        i = 0;
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
            // 1st cid is our http server's, should be 0
            // 2nd cid is for client
            // next line will be "[ESC]Z10140GET / ..."

            uint8_t cid = x2i(buf[10]); // 2nd cid = HTTP client cid
            setCidIsRequest(cid, true);
            content_lengths_[ cid ] = 0;

            // don't close other connections,
            // other connections close theirselves on their turn
            // ignore client's IP and port
        }
        // else if (strncmp(buf, P("DISCONNECT "), 11) == 0) {
        // }
        else if (strncmp(buf, P("DISASSOCIATED"), 13) == 0 ||
                 strncmp(buf, P("Disassociat"), 11) == 0) {
            // Disassociated
            // Disassociation Event
            Serial.println(("!E12"));
            clear();
            on_disconnect_();
        }
        else if (strncmp(buf, P("UnExpected Warm Boot"), 20) == 0 ||
                 strncmp(buf, P("APP Reset"), 9) == 0) {
            // APP Reset-APP SW Reset
            // APP Reset-Wlan Except
            Serial.println(("!E13"));
            clear();
            on_reset_();
        }
        // else if (strncmp(buf, P("Out of StandBy-Timer"), 20) == 0 ||
        //          strncmp(buf, P("Out of StandBy-Alarm"), 20) == 0) {
        // }
        // else if (strncmp(buf, P("Out of Deep Sleep"), 17) == 0 ) {
        // }
    }
}

void GSwifi::parseCmdResponse (char *buf) {
    Serial.print(P("c< ")); Serial.println(buf);

    if (strncmp(buf, P("OK"), 3) == 0) {
        gs_ok_ = true;
    }
    else if (strncmp(buf, P("ERROR"), 5) == 0) {
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

            gs_response_lines_ = RESPONSE_LINES_ENDED;

            if (buf[8] == '0') {
                // it's server successfully started listening
            }
            else {
                int8_t cid = x2i(buf[8]);
                setCidIsRequest(cid, false);
                content_lengths_[ cid ] = 0;

                // don't close other connections,
                // other connections close theirselves on their turn

                TIMER_STOP(timers_[cid]);
                connected_cid_ = cid;
            }
        }
        break;
    case GSCOMMANDMODE_DHCP:
        if (gs_response_lines_ == 0 && strstr(buf, P("SubNet")) && strstr(buf, P("Gateway"))) {
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
    case GSCOMMANDMODE_STATUS:
        if (gs_response_lines_ == 0 && strncmp(buf, P("NOT ASSOCIATED"), 14) == 0) {
            joined_            = false;
            listening_         = false;
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        if (gs_response_lines_ == 0 && strncmp(buf, P("MODE:"), 5) == 0) {
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1 && strncmp(buf, P("BSSID:"), 6) == 0) {
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_MDNS:
        if (gs_response_lines_ == 0) {
            // 1st line is just OK
            gs_response_lines_ ++;
        }
        else if ((gs_response_lines_ == 1) && (buf[1] == 'R')) {
            // 2nd line is something like:
            // " Registration Success!! for RR: IRKitXXXX"
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_MAC:
        if ((gs_response_lines_ == 0) && (buf[2] == ':')) {
            // 1st line is something like:
            // "00:1d:c9:01:99:99"
            sprintf( mac_, "%s", buf );
            gs_response_lines_ ++;
        }
        else if (gs_response_lines_ == 1) {
            // 2nd line is just "OK"
            gs_response_lines_ = RESPONSE_LINES_ENDED;
        }
        break;
    }

    return;
}

void GSwifi::command (const char *cmd, GSCOMMANDMODE res, uint8_t timeout_second) {
    Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );
    Serial.print(P("c> "));

    resetResponse(res);

    serial_->println(cmd);

    Serial.println(cmd);

    if (timeout_second == GS_TIMEOUT_NOWAIT) {
        return;
    }

    setBusy(true);
    waitResponse(timeout_second);
}

void GSwifi::escape (const char *cmd, uint8_t timeout_second) {
    Serial.print(P("e> "));

    resetResponse(GSCOMMANDMODE_NONE);

    serial_->write( 0x1B );
    serial_->print(cmd); // without ln

    Serial.println(cmd);
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
    while ( serial_->available() &&
            ! TIMER_FIRED(timeout_timer_) ) {

        parseByte( serial_->read() );

        if ( gs_failure_ ||
             (gs_ok_ &&
              (gs_response_lines_ == RESPONSE_LINES_ENDED ||
               gs_commandmode_    == GSCOMMANDMODE_NONE)) ) {
            gs_commandmode_ = GSCOMMANDMODE_NONE;
            setBusy(false);
            break;
        }
    }

    if ( busy_ &&
         TIMER_FIRED(timeout_timer_) ) {
        TIMER_STOP(timeout_timer_);
        Serial.println(("!E24"));
        did_timeout_ = true;
        setBusy(false);
    }

    return busy_;
}

void GSwifi::waitResponse (uint8_t timeout_second) {
    TIMER_START( timeout_timer_, timeout_second );

    while ( checkActivity() ) {
    }
}

int8_t GSwifi::join (GSSECURITY sec, const char *ssid, const char *pass, int dhcp, char *name) {
    char cmd[GS_CMD_SIZE];

    if (joined_) {
        return -1;
    }

    disconnect();

    // infrastructure mode
    command(PB("AT+WM=0",1), GSCOMMANDMODE_NORMAL);

    // set DHCP name with mac address
    sprintf(cmd, P("AT+NDHCP=1,IRKit%c%c%c%c"), mac_[12], mac_[13],  mac_[15], mac_[16]);
    command(cmd, GSCOMMANDMODE_NORMAL);

    switch (sec) {
    case GSSECURITY_NONE:
    case GSSECURITY_OPEN:
    case GSSECURITY_WEP:
        sprintf(cmd, P("AT+WAUTH=%d"), sec);
        command(cmd, GSCOMMANDMODE_NORMAL);
        if (sec != GSSECURITY_NONE) {
            // key are either 10 or 26 hexadecimal digits corresponding to a 40- bit or 104-bit key
            sprintf(cmd, P("AT+WWEP1=%s"), pass);
            command(cmd, GSCOMMANDMODE_NORMAL);
        }
        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);

        // normal people don't understand difference between wep open/shared.
        // so we try shared first, and when it failed, try open
        if (gs_failure_ && (sec == GSSECURITY_WEP)) {
            return join(GSSECURITY_OPEN, ssid, pass, dhcp, name);
        }
        break;
    case GSSECURITY_WPA_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);

        sprintf(cmd, P("AT+WWPA=%s"), pass);
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);
        break;
    case GSSECURITY_WPA2_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);
        sprintf(cmd, P("AT+WPAPSK=%s,%s"), ssid, pass);
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT_LONG);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT_LONG);

        if (gs_failure_) {
            return join(GSSECURITY_WPA_PSK, ssid, pass, dhcp, name);
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

    if ( ! joined_ ) {
        return -1;
    }

    // longest: "AT+NSTCP=65535"
    sprintf(cmd, P("AT+NSTCP=%d"), port);
    command(cmd, GSCOMMANDMODE_CONNECT);
    if (did_timeout_) {
        return -1;
    }

    listening_   = true;

    // assume CID is 0 for server (only listen on 1 port)

    return 0;
}

int GSwifi::disconnect () {
    joined_    = false;
    listening_ = false;
    command(PB("AT+NCLOSEALL",1), GSCOMMANDMODE_NORMAL);
    command(PB("AT+WD",1),        GSCOMMANDMODE_NORMAL);
    command(PB("AT+NDHCP=0",1),   GSCOMMANDMODE_NORMAL);
    return 0;
}

bool GSwifi::isJoined () {
    return joined_;
}

bool GSwifi::isListening () {
    return listening_;
}

// 4.2.1 UART Parameters
// Allowed baud rates include: 9600, 19200, 38400, 57600, 115200, 230400,460800 and 921600.
// The new UART parameters take effect immediately. However, they are stored in RAM and will be lost when power is lost unless they are saved to a profile using AT&W (section 4.6.1). The profile used in that command must also be set as the power-on profile using AT&Y (section 4.6.3).
// This command returns the standard command response (section 4) to the serial interface with the new UART configuration.
int8_t GSwifi::setBaud (uint32_t baud) {
    char cmd[11];

    // longest: "ATB=921600"
    sprintf(cmd, P("ATB=%ld"), baud);
    serial_->println(cmd);
    Serial.print(P("c> ")); Serial.println(cmd);

    delay(1000);

    serial_->end();
    serial_->begin(baud);

    delay(1000);

    // Skip 1st "ERROR: INVALID INPUT" after baud rate change
    command("", GSCOMMANDMODE_NORMAL);

    return 0;
}

int8_t GSwifi::request(GSwifi::GSMETHOD method, const char *path, const char *body, uint8_t length, GSwifi::GSResponseHandler handler, uint8_t timeout) {
    char cmd[ GS_CMD_SIZE ];

    sprintf(cmd, P("AT+DNSLOOKUP=%s"), DOMAIN);
    command(cmd, GSCOMMANDMODE_DNSLOOKUP);

    // don't fail fatally on dnslookup failure,
    // we cache our server's ipaddress anyway
    // it happens randomly

    sprintf(cmd, P("AT+NCTCP=%s,80"), ipaddr_);

    connected_cid_ = 0xFF;
    command(cmd, GSCOMMANDMODE_CONNECT);
    if (did_timeout_) {
        return -1;
    }
    if (connected_cid_ == 0xFF) {
        return -1;
    }

    uint8_t cid = connected_cid_; // this might change inside command()

    handlers_[ cid ] = handler;

    // TCP_MAXRT = 10
    // AT+SETSOCKOPT=0,6,10,10,4
    sprintf(cmd, P("AT+SETSOCKOPT=%d,6,10,10,4"), cid);
    command(cmd, GSCOMMANDMODE_NORMAL);

    // Enable TCP_KEEPALIVE on this socket
    // AT+SETSOCKOPT=0,65535,8,1,4
    sprintf(cmd, P("AT+SETSOCKOPT=%d,65535,8,1,4"), cid);
    command(cmd, GSCOMMANDMODE_NORMAL);

    // TCP_KEEPALIVE_PROBES = 2
    // AT+SETSOCKOPT=0,6,4005,2,4
    sprintf(cmd, P("AT+SETSOCKOPT=%d,6,4005,2,4"), cid);
    command(cmd, GSCOMMANDMODE_NORMAL);

    // TCP_KEEPALIVE_INTVL = 150
    // AT+SETSOCKOPT=0,6,4001,150,4
    // mysteriously, GS1011MIPS denies with "ERROR: INVALID INPUT" for seconds less than 150
    sprintf(cmd, P("AT+SETSOCKOPT=%d,6,4001,150,4"), cid);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    sprintf(cmd, "S%d", cid);
    escape( cmd );

    if (method == GSMETHOD_POST) {
        serial_->print(P("POST "));
    }
    else {
        serial_->print(P("GET "));
    }
    serial_->print(path);
    serial_->println(P(" HTTP/1.1"));

    sprintf(cmd, P("User-Agent: IRKit/%s"), version);
    serial_->println(cmd);

    sprintf(cmd, P("Host: %s"), DOMAIN);
    serial_->println(cmd);

    if (method == GSMETHOD_POST) {
        serial_->print(P("Content-Length: "));
        serial_->println(length);

        serial_->println(P("Content-Type: application/x-www-form-urlencoded"));
        serial_->println();
        serial_->print(body);
    }
    else {
        serial_->println();
    }

    // we're long polling here, to receive other events, we're going back to our main loop
    // ignore timeout, we always timeout here
    escape( "E" );

    TIMER_START(timers_[cid], timeout);

    return 0;
}

int8_t GSwifi::get(const char *path, GSResponseHandler handler, uint8_t timeout_second) {
    return request( GSMETHOD_GET, path, NULL, 0, handler, timeout_second );
}

int8_t GSwifi::post(const char *path, const char *body, uint16_t length, GSResponseHandler handler, uint8_t timeout_second) {
    return request( GSMETHOD_POST, path, body, length, handler, timeout_second );
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

// for test
// void GSwifi::dump () {
//     Serial.print(P("joined_:"));            Serial.println(joined_);
//     Serial.print(P("did_timeout_:"));       Serial.println(did_timeout_);
//     Serial.print(P("gs_response_lines_:")); Serial.println(gs_response_lines_);
//     Serial.print(P("gs_mode_:"));           Serial.println(gs_mode_);
//     Serial.print(P("timeout_timer_:"));     Serial.println(timeout_timer_);
// }
