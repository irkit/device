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
#include "MemoryFree.h"
#include "convert.h"
#include "ringbuffer.h"

#define RESPONSE_LINES_ENDED -1

#define NEXT_TOKEN_CID    0
#define NEXT_TOKEN_IP     1
#define NEXT_TOKEN_PORT   2
#define NEXT_TOKEN_LENGTH 3
#define NEXT_TOKEN_DATA   4

#define CID_UNDEFINED    -1

#define ESCAPE           0x1B

GSwifi::GSwifi( HardwareSerial *serial ) :
    _serial(serial)
{
    _buf_cmd       = ring_new(GS_CMD_SIZE);
    _request.cid   = CID_UNDEFINED;
    _route_count   = 0;
}

int8_t GSwifi::setup() {
    reset();

    _serial->begin(9600);

    command(PB("AT",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // disable echo
    command(PB("ATE0",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // faster baud rate
    setBaud(115200);

    return 0;
}

int8_t GSwifi::close (int8_t cid) {
    char cmd[GS_CMD_SIZE];

    sprintf(cmd, P("AT+NCLOSE=%X"), cid);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

void GSwifi::reset () {

    // memset(&_gs_sock, 0, sizeof(_gs_sock));
    memset(&_mac, 0, sizeof(_mac));
    _joined         = false;
    _listening      = false;
    _power_status   = GSPOWERSTATUS_READY;
    _escape         = false;
    resetResponse(GSCOMMANDMODE_NONE);
    _gs_mode        = GSMODE_COMMAND;
    _dhcp           = false;
    ring_clear(_buf_cmd);
}

void GSwifi::loop() {
    checkActivity( 0 );
}

// received a character from UART
void GSwifi::parseByte(uint8_t dat) {
    // Serial.print(dat, HEX);
    // if (dat > 0x0D) {
    //     Serial.print(" ");
    //     Serial.write(dat);
    // }
    // Serial.println();

    static int len;
    static int next_token; // split each byte into tokens (cid,ip,port,length,data)
    static char tmp[20];
    static uint8_t continous_newlines = 0;

    switch (_gs_mode) {
    case GSMODE_COMMAND: // command responce
        if (_escape) {
            // esc
            switch (dat) {
            case 'O':
                Serial.println("ok");
                _gs_ok = true;
                break;
            case 'F':
                Serial.println("failure");
                _gs_failure = true;
                break;
            case 'S':
                Serial.println("GSMODE_DATA_RX");
                _gs_mode   = GSMODE_DATA_RX;
                next_token = NEXT_TOKEN_CID;
                break;
            case 'u':
                Serial.println("GSMODE_DATA_RXUDP");
                _gs_mode   = GSMODE_DATA_RXUDP;
                next_token = NEXT_TOKEN_CID;
                break;
            case 'Z':
            case 'H':
                Serial.println("GSMODE_DATA_RX_BULK");
                _gs_mode   = GSMODE_DATA_RX_BULK;
                next_token = NEXT_TOKEN_CID;
                break;
            case 'y':
                Serial.println("GSMODE_DATA_RXUDP_BULK");
                _gs_mode   = GSMODE_DATA_RXUDP_BULK;
                next_token = NEXT_TOKEN_CID;
                break;
            default:
                Serial.print("unknown [ESC] 0x"); Serial.println(dat,HEX);
                break;
            }
            _escape = false;
        }
        else {
            if (dat == 0x1b) {
                _escape = true;
            }
            else if (dat == '\n') {
                // end of line
                parseLine();
            }
            else if (dat != '\r') {
                // command
                if ( ! ring_isfull(_buf_cmd) ) {
                    ring_put(_buf_cmd, dat);
                }
                else {
                    Serial.println(P("!!! line buffer overflowed !!!"));
                }
            }
        }
        break;

    case GSMODE_DATA_RX:
    case GSMODE_DATA_RXUDP:
        if (next_token == NEXT_TOKEN_CID) {
            // cid
            // _cid = x2i(dat);
            // _gs_sock[_cid].received = false;
            next_token = NEXT_TOKEN_IP;
            if (_gs_mode == GSMODE_DATA_RX) {
                next_token = NEXT_TOKEN_LENGTH;
            }
            len = 0;
        }
        else if (next_token == NEXT_TOKEN_IP) {
            // ip
            if ((dat < '0' || dat > '9') && dat != '.') {
                int ip1, ip2, ip3, ip4;
                tmp[len]   = 0;
                sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                _from.setIp(IpAddr(ip1, ip2, ip3, ip4));
                next_token = NEXT_TOKEN_PORT;
                len = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        }
        else if (next_token == NEXT_TOKEN_PORT) {
            // port
            if (dat < '0' || dat > '9') {
                tmp[len]   = 0;
                _from.setPort(atoi(tmp));
                next_token = NEXT_TOKEN_LENGTH;
                len        = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        }
        else if (_escape) {
            // esc
            switch (dat) {
            case 'E':
                // Serial.println("recv ascii %d", _cid);
                // _gs_sock[_cid].received = true;
                _gs_mode = GSMODE_COMMAND;

                // if (_gs_sock[_cid].protocol == GSPROT_HTTPGET) {
                //     // recv interrupt
                //     if (_gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available()) == 0) {
                //         _gs_sock[_cid].received = false;
                //     }
                // }
                break;
            default:
                Serial.print(P("unknown <ESC> ")); Serial.println(dat, HEX);
                break;
            }
            _escape = false;
        }
        else {
            if (dat == 0x1b) {
                _escape = true;
            }
            else {
                // data
                // if (_gs_sock[_cid].data != NULL) {
                //     _gs_sock[_cid].data->queue(dat);
                //     len ++;

                //     if (len < GS_DATA_SIZE && _gs_sock[_cid].data->isFull()) {
                //         // buffer full
                //         // recv interrupt
                //         _gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available());
                //     }
                // }
            }
        }
        break;

    case GSMODE_DATA_RX_BULK:
    case GSMODE_DATA_RXUDP_BULK:
        if (next_token == NEXT_TOKEN_CID) {
            // cid
            if (_gs_mode == GSMODE_DATA_RX_BULK) {
                next_token = NEXT_TOKEN_LENGTH;
            }
            else {
                next_token = NEXT_TOKEN_IP;
            }
            len = 0;
        }
        else if (next_token == NEXT_TOKEN_IP) {
            // ip
            if ((dat < '0' || dat > '9') && dat != '.') {
                int ip1, ip2, ip3, ip4;
                tmp[len]   = 0;
                sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                _from.setIp(IpAddr(ip1, ip2, ip3, ip4));
                next_token = NEXT_TOKEN_PORT;
                len        = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        }
        else if (next_token == NEXT_TOKEN_PORT) {
            // port
            if (dat < '0' || dat > '9') {
                tmp[len]   = 0;
                _from.setPort(atoi(tmp));
                next_token = NEXT_TOKEN_LENGTH;
                len        = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        }
        else if (next_token == NEXT_TOKEN_LENGTH) {
            // Data Length is 4 ascii char represents decimal value i.e. 1400 byte (0x31 0x34 0x30 0x30)
            tmp[ len ++ ] = dat;
            if (len >= 4) {
                tmp[ len ] = 0;
                len        = atoi(tmp); // length of data
                next_token = NEXT_TOKEN_DATA;

                _request.state = GSHTTPSTATE_HEAD1;
                ring_clear( _buf_cmd ); // reuse _buf_cmd to store HTTP request

                Serial.print(P("bulk length:")); Serial.println(len, DEC);
                break;
            }
        }
        else if (next_token == NEXT_TOKEN_DATA) {
            len --;

            switch (_request.state) {
            case GSHTTPSTATE_HEAD1:
                if (dat != '\n') {
                    ring_put( _buf_cmd, dat );
                }
                else {
                    // end of request line
                    char method[8];
                    uint8_t method_size = 8;
                    char path[8];
                    uint8_t path_size = 8;
                    int8_t result = parseRequestLine((char*)method, method_size);
                    Serial.print(P("method:")); Serial.write(method); Serial.println();

                    if ( result == 0 ) {
                        result = parseRequestLine((char*)path, path_size);
                        Serial.print(P("path:")); Serial.write(path); Serial.println();
                    }
                    if ( result != 0 ) {
                        // couldn't detect method or path
                        // TODO error response 401
                        _request.state = GSHTTPSTATE_ERROR;
                        Serial.println(P("error1"));
                        break;
                    }
                    GSMETHOD gsmethod = x2method(method);

                    int8_t routeid = router(gsmethod, &path[0]);
                    if ( routeid < 0 ) {
                        // TODO error response 404
                        _request.state = GSHTTPSTATE_ERROR;
                        Serial.println(P("error2"));
                        break;
                    }
                    _request.routeid   = routeid;
                    _request.state     = GSHTTPSTATE_HEAD2;
                    continous_newlines = 0;
                    Serial.println(P("next: head2"));
                }
                break;
            case GSHTTPSTATE_HEAD2:
                if (dat == '\n') {
                    continous_newlines ++;
                }
                else if (dat == '\r') {
                    // preserve
                }
                else {
                    continous_newlines = 0;
                }
                if (continous_newlines == 2) {
                    // if detected double \n, switch to body mode
                    _request.state = GSHTTPSTATE_BODY;
                    ring_clear(_buf_cmd);
                    Serial.println(P("next: body"));
                }
                break;
            case GSHTTPSTATE_BODY:
                if (ring_isfull(_buf_cmd)) {
                    Serial.println(P("full"));
                    dispatchRequestHandler();
                }
                ring_put(_buf_cmd, dat);
                break;
            case GSHTTPSTATE_ERROR:
                // skip
                break;
            case GSHTTPSTATE_RECEIVED:
            case GSHTTPSTATE_RECEIVED_ERROR:
            default:
                break;
            }

            if (len == 0) {
                Serial.println(P("len==0"));
                if (ring_isfull(_buf_cmd)) {
                    dispatchRequestHandler();
                }

                _escape = false;
                _gs_mode = GSMODE_COMMAND;
                if ( _request.state == GSHTTPSTATE_ERROR ) {
                    _request.state = GSHTTPSTATE_RECEIVED_ERROR;
                }
                else {
                    _request.state = GSHTTPSTATE_RECEIVED;
                }
                // last
                dispatchRequestHandler();
            }
        }
        break;
    }
}

int8_t GSwifi::parseRequestLine (char *token, uint8_t token_size) {
    uint8_t i;
    for ( i = 0; i < token_size; i++ ) {
        if (ring_isempty( _buf_cmd )) {
            return -1; // ' ' didn't appear
        }
        ring_get( _buf_cmd, token+i, 1 );
        if (token[i] == ' ') {
            token[i] = '\0';
            break;
        }
    }
    if ( i == token_size ) {
        return -1; // couldnt detect token
    }
    return 0;
}

int8_t GSwifi::router (GSMETHOD method, const char *path) {
    if (method == GSMETHOD_UNKNOWN) {
        return -1;
    }

    uint8_t i;
    for (i = 0; i < _route_count; i ++) {
        if ((method == _routes[i].method) &&
            (strncmp(path, _routes[i].path, GS_MAX_PATH_LENGTH) == 0)) {
            Serial.print(P("router matched: ")); Serial.println(i);
            return i;
        }
    }
    return -1;
}

int8_t GSwifi::registerRoute (GSwifi::GSMETHOD method, const char *path) {
    if ( _route_count >= GS_MAX_ROUTES ) {
        return -1;
    }
    _routes[ _route_count ].method = method;
    strncpy(_routes[ _route_count ].path, path, sizeof(_routes[_route_count].path));
    _route_count ++;
}

void GSwifi::setRequestHandler (GSRequestHandler handler) {
    _handler = handler;
}

int8_t GSwifi::dispatchRequestHandler () {
    return _handler();
}

void GSwifi::writeHead (uint16_t status_code) {
    Serial.print(P("writeHead>")); Serial.println(_request.cid);

    _serial->write(ESCAPE);
    _serial->write('S');
    _serial->print(_request.cid);

    _serial->print(P("HTTP/1.0 "));
    _serial->print(status_code);
    _serial->println(P(" OK"));

    _serial->println(P("Content-Type: text/plain"));

    _serial->println();
}

void GSwifi::write (const char *data) {
    _serial->print(data);
}

void GSwifi::end (const char *data) {
    _serial->println(data);
    _serial->write(ESCAPE);
    _serial->write('E');

    // close connection
    _serial->print(P("AT+NCLOSE=")); _serial->println(_request.cid);
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
                (i < sizeof(buf)) ) {
            ring_get( _buf_cmd, &buf[i], 1 );
            if (buf[i] == '\n') {
                break;
            }
            i ++;
        }
        if (i == 0) continue;
        buf[i] = 0;

        if ( (_gs_mode == GSMODE_COMMAND) &&
             (_gs_commandmode != GSCOMMANDMODE_NONE) ) {
            parseCmdResponse(buf);
        }

        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[8] >= '0' && buf[8] <= 'F' && buf[9] != 0) {
            // connect from client
            // CONNECT 0 1 192.168.2.1 63632
            // 1st cid is our http server's, should be 0
            // 2nd cid is for client
            // next line will be "[ESC]Z10140GET / ..."

            if (_request.cid != CID_UNDEFINED) {
                close( _request.cid );
            }

            _request.cid    = x2i(buf[10]); // 2nd cid
            _request.state  = GSHTTPSTATE_PREPARE;
            _request.length = 0;

            // ignore client's IP and port

            Serial.println(buf);
        }
        else if (strncmp(buf, "DISCONNECT ", 11) == 0) {
            int8_t cid = x2i(buf[11]);
            Serial.println(P("disconnect ")); Serial.println(cid);
            // _gs_sock[cid].connect = false;
            // _gs_sock[cid].onGsReceive.call(cid, -1); // event disconnected
        }
        else if (strncmp(buf, "DISASSOCIATED", 13) == 0 ||
                 strncmp(buf, "Disassociated", 13) == 0 ||
                 strncmp(buf, "Disassociation Event", 20) == 0 ) {
            _joined    = false;
            _listening = false;
            // for (i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
        }
        else if (strncmp(buf, "UnExpected Warm Boot", 20) == 0 ||
          strncmp(buf, "APP Reset-APP SW Reset", 22) == 0 ||
          strncmp(buf, "APP Reset-Wlan Except", 21) == 0 ) {
            Serial.println("disassociate");
            _joined       = false;
            _listening    = false;
            _power_status = GSPOWERSTATUS_READY;
            _escape       = false;
            resetResponse(GSCOMMANDMODE_NONE);
            _gs_mode      = GSMODE_COMMAND;
            // for (i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
        }
        else if (strncmp(buf, "Out of StandBy-Timer", 20) == 0 ||
          strncmp(buf, "Out of StandBy-Alarm", 20) == 0) {
            if (_power_status == GSPOWERSTATUS_STANDBY) {
                _power_status = GSPOWERSTATUS_WAKEUP;
            }
        }
        else if (strncmp(buf, "Out of Deep Sleep", 17) == 0 ) {
            if (_power_status == GSPOWERSTATUS_DEEPSLEEP) {
                _power_status = GSPOWERSTATUS_READY;
            }
        }
        // Serial.print(P("status: ")); Serial.println(_power_status, HEX);
    }
}

void GSwifi::parseCmdResponse (char *buf) {
    Serial.print(P("parseCmd: ")); Serial.println(buf);

    if (strcmp(buf, "OK") == 0) {
        _gs_ok = true;
    }
    else if (strncmp(buf, "ERROR", 5) == 0) {
        _gs_failure = true;
    }

    switch(_gs_commandmode) {
    case GSCOMMANDMODE_NORMAL:
        _gs_response_lines = RESPONSE_LINES_ENDED;
        break;
    case GSCOMMANDMODE_CONNECT:
        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[9] == 0) {
            // server started listening
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_DHCP:
        if (_gs_response_lines == 0 && strstr(buf, "SubNet") && strstr(buf, "Gateway")) {
            _gs_response_lines ++;
        } else
        if (_gs_response_lines == 1) {
            int ip1, ip2, ip3, ip4;
            char *tmp = buf + 1;
            sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _ipaddr = IpAddr(ip1, ip2, ip3, ip4);
            tmp = strstr(tmp, ":") + 2;
            sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _netmask = IpAddr(ip1, ip2, ip3, ip4);
            tmp = strstr(tmp, ":") + 2;
            sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _gateway = IpAddr(ip1, ip2, ip3, ip4);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_MACADDRESS:
        if (buf[2] == ':' && buf[5] == ':') {
            int mac1, mac2, mac3, mac4, mac5, mac6;
            sscanf(buf, "%x:%x:%x:%x:%x:%x", &mac1, &mac2, &mac3, &mac4, &mac5, &mac6);
            _mac[0] = mac1;
            _mac[1] = mac2;
            _mac[2] = mac3;
            _mac[3] = mac4;
            _mac[4] = mac5;
            _mac[5] = mac6;
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_DNSLOOKUP:
        if (strncmp(buf, "IP:", 3) == 0) {
            int ip1, ip2, ip3, ip4;
            sscanf(&buf[3], "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _resolv = IpAddr(ip1, ip2, ip3, ip4);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSCOMMANDMODE_HTTP:
        if (buf[0] >= '0' && buf[0] <= 'F' && buf[1] == 0) {
            // _cid = x2i(buf[0]);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    // case GSCOMMANDMODE_RSSI:
    //     if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
    //         _rssi = atoi(buf);
    //         _gs_response_lines = RESPONSE_LINES_ENDED;
    //     }
    //     break;
    case GSCOMMANDMODE_STATUS:
        if (_gs_response_lines == 0 && strncmp(buf, "NOT ASSOCIATED", 14) == 0) {
            _joined    = false;
            _listening = false;
            // for (int i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        if (_gs_response_lines == 0 && strncmp(buf, "MODE:", 5) == 0) {
            _gs_response_lines ++;
        }
        else if (_gs_response_lines == 1 && strncmp(buf, "BSSID:", 6) == 0) {
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    }

    return;
}

void GSwifi::command (const char *cmd, GSCOMMANDMODE res, uint32_t timeout) {
    Serial.print(P("c> "));

    resetResponse(res);

    _serial->println(cmd);
    Serial.println(cmd);

    setBusy(true);
    waitResponse(timeout);
}

void GSwifi::resetResponse (GSCOMMANDMODE res) {
    _gs_ok                = false;
    _gs_failure           = false;
    _gs_response_lines    = 0;
    _gs_commandmode = res;
}

bool GSwifi::setBusy(bool busy) {
    if (busy) {
        timeout_start_ = millis();
        did_timeout_   = false;
        // if (onBusy) onBusy();
    } else {
        // lastError = false;
        // if (onIdle) onIdle();
    }
    return busy_ = busy;
}

uint8_t GSwifi::checkActivity(uint32_t timeout_ms) {
    while ( _serial->available() &&
            ( (timeout_ms == 0) ||
              millis() - timeout_start_ < timeout_ms ) ) {

        parseByte( _serial->read() );

        if ( (_gs_ok || _gs_failure) &&
             (_gs_response_lines == RESPONSE_LINES_ENDED || _gs_commandmode == GSCOMMANDMODE_NONE) ) {
            _gs_commandmode = GSCOMMANDMODE_NONE;
            setBusy(false);
            break;
        }
    }

    if ( (timeout_ms > 0) &&
         busy_ &&
         (millis() - timeout_start_ >= timeout_ms) ) {
        Serial.println(P("!!! did timeout !!!"));
        did_timeout_ = true;
        if (onTimeout_ != 0) {
            onTimeout_();
        }
        setBusy(false);
    }

    return busy_;
}

void GSwifi::waitResponse (uint32_t ms) {
    while ( checkActivity(ms) ) {
    }
}

int GSwifi::join (GSSECURITY sec, const char *ssid, const char *pass, int dhcp, char *name) {
    char cmd[GS_CMD_SIZE];

    if (_joined || _power_status != GSPOWERSTATUS_READY) return -1;

    if (getMacAddress(_mac)) {
        return -1;
    }

#ifdef GS_BULK
    command(PB("AT+BDATA=1",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
#endif

    disconnect();

    // infrastructure mode
    command(PB("AT+WM=0",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // dhcp
    if (dhcp) {
        command(PB("AT+NDHCP=1",1), GSCOMMANDMODE_NORMAL);
    } else {
        command(PB("AT+NDHCP=0",1), GSCOMMANDMODE_NORMAL);
    }
    if (did_timeout_) {
        return -1;
    }

    switch (sec) {
    case GSSECURITY_NONE:
    case GSSECURITY_OPEN:
    case GSSECURITY_WEP:
        sprintf(cmd, P("AT+WAUTH=%d"), sec);
        command(cmd, GSCOMMANDMODE_NORMAL);
        if (sec != GSSECURITY_NONE) {
            sprintf(cmd, P("AT+WWEP1=%s"), pass);
            command(cmd, GSCOMMANDMODE_NORMAL);
            // wait_ms(100);
        }
        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT2);
        break;
    case GSSECURITY_WPA_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);

        sprintf(cmd, P("AT+WWPA=%s"), pass);
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT2);
        break;
    case GSSECURITY_WPA2_PSK:
        command(PB("AT+WAUTH=0",1), GSCOMMANDMODE_NORMAL);
        sprintf(cmd, P("AT+WPAPSK=%s,%s"), ssid, pass);
        command(cmd, GSCOMMANDMODE_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSCOMMANDMODE_DHCP, GS_TIMEOUT2);
        break;
    default:
        Serial.println(P("Can't use security"));
        return -1;
    }

    if (did_timeout_) {
        return -1;
    }

    if (!dhcp) {
        sprintf(cmd, P("AT+DNSSET=%d.%d.%d.%d"),
            _gateway[0], _gateway[1], _gateway[2], _gateway[3]);
        command(cmd, GSCOMMANDMODE_NORMAL);
    }

    _joined = true;
    _dhcp   = dhcp;
    return 0;
}

int GSwifi::listen(GSPROTOCOL protocol, uint16_t port) {
    char cmd[GS_CMD_SIZE];

    if ( (! _joined) ||
         (_power_status != GSPOWERSTATUS_READY) ) {
        return -1;
    }
    if (port == 0) {
        return -1;
    }

    if (protocol == GSPROTOCOL_UDP) {
        sprintf(cmd, P("AT+NSUDP=%d"), port);
    } else {
        sprintf(cmd, P("AT+NSTCP=%d"), port);
    }
    command(cmd, GSCOMMANDMODE_CONNECT);
    if (did_timeout_) {
        return -1;
    }

    _listening = true;

    // assume CID is 0 for server (only listen on 1 port)

    return 0;
}

int GSwifi::disconnect () {
    int i;

    _joined    = false;
    _listening = false;
    // for (i = 0; i < 16; i ++) {
    //     _gs_sock[i].connect = false;
    // }
    command(PB("AT+NCLOSEALL",1), GSCOMMANDMODE_NORMAL);
    command(PB("AT+WD",1),        GSCOMMANDMODE_NORMAL);
    command(PB("AT+NDHCP=0",1),   GSCOMMANDMODE_NORMAL);
    return 0;
}

int GSwifi::setAddress (char *name) {
    command(PB("AT+NDHCP=1",1), GSCOMMANDMODE_DHCP, GS_TIMEOUT2);
    if (did_timeout_) {
        return -1;
    }
    if (_ipaddr.isNull()) return -1;
    return 0;
}

int GSwifi::setAddress (IpAddr ipaddr, IpAddr netmask, IpAddr gateway, IpAddr nameserver) {
    int r;
    char cmd[GS_CMD_SIZE];

    command(PB("AT+NDHCP=0",1), GSCOMMANDMODE_NORMAL);
    // wait_ms(100);

    sprintf(cmd, P("AT+NSET=%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d"),
        ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3],
        netmask[0], netmask[1], netmask[2], netmask[3],
        gateway[0], gateway[1], gateway[2], gateway[3]);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    _ipaddr = ipaddr;
    _netmask = netmask;
    _gateway = gateway;

    if (ipaddr != nameserver) {
        sprintf(cmd, P("AT+DNSSET=%d.%d.%d.%d"),
            nameserver[0], nameserver[1], nameserver[2], nameserver[3]);
        command(cmd, GSCOMMANDMODE_NORMAL);
    }
    return did_timeout_;
}

int GSwifi::getAddress (IpAddr &ipaddr, IpAddr &netmask, IpAddr &gateway, IpAddr &nameserver) {
    ipaddr = _ipaddr;
    netmask = _netmask;
    gateway = _gateway;
    nameserver = _nameserver;
    return 0;
}

int GSwifi::getMacAddress (char *mac) {
    Serial.println(P("getMacAddress"));

    command(PB("AT+NMAC=?",1), GSCOMMANDMODE_MACADDRESS);
    if (_mac[0] || _mac[1] || _mac[2] || _mac[3] || _mac[4] || _mac[5]) {
        memcpy(mac, _mac, 6);
        return 0;
    }
    return -1;
}

int GSwifi::getHostByName (const char* name, IpAddr &addr) {
    char cmd[GS_CMD_SIZE];

    if (! _joined || _power_status != GSPOWERSTATUS_READY) return -1;

    sprintf(cmd, P("AT+DNSLOOKUP=%s"), name);
    command(cmd, GSCOMMANDMODE_DNSLOOKUP);
    if (did_timeout_) {
        return -1;
    }

    addr = _resolv;
    return 0;
}

int GSwifi::getHostByName (Host &host) {
    char cmd[GS_CMD_SIZE];

    if (! _joined || _power_status != GSPOWERSTATUS_READY) return -1;

    sprintf(cmd, P("AT+DNSLOOKUP=%s"), host.getName());
    command(cmd, GSCOMMANDMODE_DNSLOOKUP);
    if (did_timeout_) {
        return -1;
    }

    host.setIp(_resolv);
    return 0;
}

bool GSwifi::isJoined () {
    return _joined;
}

bool GSwifi::isListening () {
    return _listening;
}

GSwifi::GSPOWERSTATUS GSwifi::getPowerStatus () {
    return _power_status;
}

int GSwifi::getRssi () {
    command(PB("AT+WRSSI=?",1), GSCOMMANDMODE_RSSI);
    if (did_timeout_) {
        return 0;
    }
    return _rssi;
}

// 4.2.1 UART Parameters
// Allowed baud rates include: 9600, 19200, 38400, 57600, 115200, 230400,460800 and 921600.
// The new UART parameters take effect immediately. However, they are stored in RAM and will be lost when power is lost unless they are saved to a profile using AT&W (section 4.6.1). The profile used in that command must also be set as the power-on profile using AT&Y (section 4.6.3).
// This command returns the standard command response (section 4) to the serial interface with the new UART configuration.
int8_t GSwifi::setBaud (uint32_t baud) {
    char cmd[GS_CMD_SIZE];

    if (_power_status != GSPOWERSTATUS_READY) {
        return -1;
    }

    sprintf(cmd, P("ATB=%ld"), baud);
    _serial->println(cmd);
    Serial.print(P("c> ")); Serial.println(cmd);

    delay(1000);

    _serial->end();
    _serial->begin(baud);

    delay(1000);

    // Skip 1st "ERROR: INVALID INPUT" after baud rate change
    command("", GSCOMMANDMODE_NORMAL);

    return 0;
}

int GSwifi::setRegion (int reg) {
    char cmd[GS_CMD_SIZE];

    if (_power_status != GSPOWERSTATUS_READY) return -1;

    sprintf(cmd, P("AT+WREGDOMAIN=%d"), reg);
    command(cmd, GSCOMMANDMODE_NORMAL);
    return did_timeout_;
}

#ifdef GS_ENABLE_MDNS
/**
 * mDNS
 */
int8_t GSwifi::mDNSStart() {
    command(PB("AT+MDNSSTART",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSRegisterHostname(const char *hostname) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSHNREG=%s,local"), hostname);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDeregisterHostname(const char *hostname) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSHNDEREG=%s,local"), hostname);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

// AT+MDNSSRVREG=<ServiceInstanceName>,[<ServiceSubType>],<ServiceType>, <Protocol>,<Domain>,<port>,<Default Key=Val>,<key 1=val 1>, <key 2=val 2>.....
// Example: if the factory default host name is “GAINSPAN” and the mac address of the node is “00-1d-c9- 00-22-97”, then AT+MDNSHNREG=,local
// Will take the host name as “GAINSPAN_002297”
// TODO change factory default host name
int8_t GSwifi::mDNSRegisterService(const char *name, const char *subtype, const char *type, const char *protocol, uint16_t port) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSSRVREG=%s,%s,%s,%s,local,%d"), name, subtype, type, protocol, port );
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDeregisterService(const char *name, const char *subtype, const char *type, const char *protocol) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSSRVDEREG=%s,%s,%s,%s,local"), name, subtype, type, protocol );
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSAnnounceService() {
    command(PB("AT+MDNSANNOUNCE",1), GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDiscoverService(const char *subtype, const char *type, const char *protocol) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSSD=%s,%s,%s,local"), subtype, type, protocol);
    command(cmd, GSCOMMANDMODE_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}
#endif // GS_ENABLE_MDNS

// for test
void GSwifi::dump () {
    Serial.print(P("_joined:"));            Serial.println(_joined);
    Serial.print(P("_power_status:"));      Serial.println(_power_status);
    Serial.print(P("did_timeout_:"));       Serial.println(did_timeout_);
    Serial.print(P("_gs_response_lines:")); Serial.println(_gs_response_lines);

    int i;

//     DBG("GS mode=%d, escape=%d, cid=%d\r\n", _gs_mode, _escape, _cid);
//     for (i = 0; i < 16; i ++) {
//         DBG("[%d] ", i);
//         DBG("connect=%d, type=%d, protocol=%d, len=%d\r\n", _gs_sock[i].connect, _gs_sock[i].type, _gs_sock[i].protocol, _gs_sock[i].data->available());
//         DBG("  %x, %x\r\n", &_gs_sock[i], _gs_sock[i].data);
// #ifdef GS_ENABLE_HTTPD
//         if (_gs_sock[i].protocol == GSPROT_HTTPD) {
//             DBG("  mode=%d, type=%d, len=%d\r\n", i, _httpd[i].mode, _httpd[i].type, _httpd[i].len);
//             DBG("  %x, %x\r\n", &_httpd[i], _httpd[i].buf);
//         }
// #endif
//     }
}
