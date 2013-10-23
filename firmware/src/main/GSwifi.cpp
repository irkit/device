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

GSwifi::GSwifi( HardwareSerial *serial ) :
    _serial(serial)
{
    _buf_cmd = ring_new(GS_CMD_SIZE);
}

int8_t GSwifi::setup() {
    reset();

    _serial->begin(9600);

    command(PB("AT",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // disable echo
    command(PB("ATE0",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // faster baud rate
    setBaud(115200);

    return 0;
}

// void GSwifi::newSock (int cid, GSTYPE type, GSPROTOCOL pro) {
//     _gs_sock[cid].type     = type;
//     _gs_sock[cid].protocol = pro;
//     _gs_sock[cid].connect  = true;
//     if (_gs_sock[cid].data == NULL) {
//         _gs_sock[cid].data = new CircBuffer<char>(GS_DATA_SIZE);
//     } else {
//         _gs_sock[cid].data->flush();
//     }
//     _gs_sock[cid].lcid     = 0;
//     _gs_sock[cid].received = false;
//     _gs_sock[cid].onGsReceive.detach();
// }

// int8_t GSwifi::close (int cid) {
//     char cmd[GS_CMD_SIZE];

//     if (! _gs_sock[cid].connect) {
//         return -1;
//     }

//     _gs_sock[cid].connect = false;
//     delete _gs_sock[cid].data;
//     _gs_sock[cid].data = NULL;
//     sprintf(cmd, P("AT+NCLOSE=%X"), cid);
//     command(cmd, GSRES_NORMAL);
//     if (did_timeout_) {
//         return -1;
//     }
//     return 0;
// }

void GSwifi::reset () {

    // memset(&_gs_sock, 0, sizeof(_gs_sock));
    memset(&_mac, 0, sizeof(_mac));
    _joined         = false;
    _power_status   = GSPOWERSTATUS_READY;
    _escape         = false;
    resetResponse(GSRES_NONE);
    _gs_mode        = GSMODE_COMMAND;
    _dhcp           = false;
    ring_clear(_buf_cmd);
}

void GSwifi::loop() {
    checkActivity( 0 );
}

// received a character from UART
void GSwifi::parse(uint8_t dat) {
    // Serial.print(dat, HEX);
    // if (dat > 0x0D) {
    //     Serial.print(" ");
    //     Serial.write(dat);
    // }
    // Serial.println();

    static int len, mode;
    static char tmp[20];

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
                _gs_mode = GSMODE_DATA_RX;
                mode = 0;
                break;
            case 'u':
                Serial.println("GSMODE_DATA_RXUDP");
                _gs_mode = GSMODE_DATA_RXUDP;
                mode = 0;
                break;
            case 'Z':
            case 'H':
                Serial.println("GSMODE_DATA_RX_BULK");
                _gs_mode = GSMODE_DATA_RX_BULK;
                mode = 0;
                break;
            case 'y':
                Serial.println("GSMODE_DATA_RXUDP_BULK");
                _gs_mode = GSMODE_DATA_RXUDP_BULK;
                mode = 0;
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
                parseResponse();
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
        if (mode == 0) {
            // cid
            _cid = x2i(dat);
            // _gs_sock[_cid].received = false;
            mode ++;
            if (_gs_mode == GSMODE_DATA_RX) {
                mode = 3;
            }
            len = 0;
        } else
        if (mode == 1) {
            // ip
            if ((dat < '0' || dat > '9') && dat != '.') {
                int ip1, ip2, ip3, ip4;
                tmp[len] = 0;
                sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                _from.setIp(IpAddr(ip1, ip2, ip3, ip4));
                mode ++;
                len = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        } else
        if (mode == 2) {
            // port
            if (dat < '0' || dat > '9') {
                tmp[len] = 0;
                _from.setPort(atoi(tmp));
                mode ++;
                len = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        } else
        if (_escape) {
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
        } else {
            if (dat == 0x1b) {
                _escape = true;
            } else {
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
        if (mode == 0) {
            // cid
            _cid = x2i(dat);
            // _gs_sock[_cid].received = false;
            mode ++;
            if (_gs_mode == GSMODE_DATA_RX_BULK) {
                mode = 3;
            }
            len = 0;
        } else
        if (mode == 1) {
            // ip
            if ((dat < '0' || dat > '9') && dat != '.') {
                int ip1, ip2, ip3, ip4;
                tmp[len] = 0;
                sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                _from.setIp(IpAddr(ip1, ip2, ip3, ip4));
                mode ++;
                len = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        } else
        if (mode == 2) {
            // port
            if (dat < '0' || dat > '9') {
                tmp[len] = 0;
                _from.setPort(atoi(tmp));
                mode ++;
                len = 0;
                break;
            }
            if (len < sizeof(tmp) - 1) {
                tmp[len] = dat;
                len ++;
            }
        } else
        if (mode == 3) {
            // length
            tmp[len] = dat;
            len ++;
            if (len >= 4) {
                tmp[len] = 0;
                len = atoi(tmp);
                mode ++;
                break;
            }
        } else
            if (mode == 4) {
                // data
                // if (_gs_sock[_cid].data != NULL) {
                //     _gs_sock[_cid].data->queue(dat);
                //     len  --;

                //     if (len && _gs_sock[_cid].data->isFull()) {
                //         // buffer full
                //         // recv interrupt
                //         _gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available());
                //     }
                // }
                // if (len == 0) {
                //     // Serial.println("recv binary %d", _cid);
                //     _gs_sock[_cid].received = true;
                //     _escape = false;
                //     _gs_mode = GSMODE_COMMAND;

                //     if (_gs_sock[_cid].protocol == GSPROT_HTTPGET) {
                //         // recv interrupt
                //         if (_gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available()) == 0) {
                //             _gs_sock[_cid].received = false;
                //         }
                //     }
                // }
            }
        break;
    }
}

void GSwifi::parseResponse () {
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

        parseCmdResponse(buf);

        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[8] >= '0' && buf[8] <= 'F' && buf[9] != 0) {
            int cid = x2i(buf[8]);
            // if (_gs_sock[cid].type == GSTYPE_SERVER) {
            //     // fork (server socket)
            //     int acid, ip1, ip2, ip3, ip4;
            //     char *tmp = buf + 12;

            //     acid = x2i(buf[10]);
            //     Serial.print(P("connect ")); Serial.print(cid);
            //     Serial.print(P(" -> "));     Serial.println(acid);

            //     // newSock(acid, _gs_sock[cid].type, _gs_sock[cid].protocol);
            //     _gs_sock[acid].onGsReceive = _gs_sock[cid].onGsReceive;
            //     _gs_sock[acid].lcid = cid;
            //     sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            //     _gs_sock[acid].host.setIp(IpAddr(ip1, ip2, ip3, ip4));
            //     tmp = strstr(tmp, " ") + 1;
            //     _gs_sock[acid].host.setPort(atoi(tmp));
            //     _gs_sock[acid].onGsReceive.call(acid, 0); // event connected
            // }
        } else
        if (strncmp(buf, "DISCONNECT ", 11) == 0) {
            int cid = x2i(buf[11]);
            Serial.println(P("disconnect ")); Serial.println(cid);
            // _gs_sock[cid].connect = false;
            // _gs_sock[cid].onGsReceive.call(cid, -1); // event disconnected
        } else
        if (strncmp(buf, "DISASSOCIATED", 13) == 0 ||
          strncmp(buf, "Disassociated", 13) == 0 ||
          strncmp(buf, "Disassociation Event", 20) == 0 ) {
            _joined = false;
            // for (i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
        } else
        if (strncmp(buf, "UnExpected Warm Boot", 20) == 0 ||
          strncmp(buf, "APP Reset-APP SW Reset", 22) == 0 ||
          strncmp(buf, "APP Reset-Wlan Except", 21) == 0 ) {
            Serial.println("disassociate");
            _joined      = false;
            _power_status = GSPOWERSTATUS_READY;
            _escape       = false;
            resetResponse(GSRES_NONE);
            _gs_mode      = GSMODE_COMMAND;
            // for (i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
        } else
        if (strncmp(buf, "Out of StandBy-Timer", 20) == 0 ||
          strncmp(buf, "Out of StandBy-Alarm", 20) == 0) {
            if (_power_status == GSPOWERSTATUS_STANDBY) {
                _power_status = GSPOWERSTATUS_WAKEUP;
            }
        } else
        if (strncmp(buf, "Out of Deep Sleep", 17) == 0 ) {
            if (_power_status == GSPOWERSTATUS_DEEPSLEEP) {
                _power_status = GSPOWERSTATUS_READY;
            }
        }
        // Serial.print(P("status: ")); Serial.println(_power_status, HEX);
    }
}

void GSwifi::parseCmdResponse (char *buf) {
    Serial.print(P("parseCmd: ")); Serial.println(buf);

    if (_gs_expected_response == GSRES_NONE) return;

    if (strcmp(buf, "OK") == 0) {
        _gs_ok = true;
    } else
    if (strncmp(buf, "ERROR", 5) == 0) {
        _gs_failure = true;
    }

    switch(_gs_expected_response) {
    case GSRES_NORMAL:
        _gs_response_lines = RESPONSE_LINES_ENDED;
        break;
    case GSRES_CONNECT:
        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[9] == 0) {
            _cid = x2i(buf[8]);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSRES_DHCP:
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
    case GSRES_MACADDRESS:
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
    case GSRES_DNSLOOKUP:
        if (strncmp(buf, "IP:", 3) == 0) {
            int ip1, ip2, ip3, ip4;
            sscanf(&buf[3], "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _resolv = IpAddr(ip1, ip2, ip3, ip4);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    case GSRES_HTTP:
        if (buf[0] >= '0' && buf[0] <= 'F' && buf[1] == 0) {
            _cid = x2i(buf[0]);
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    // case GSRES_RSSI:
    //     if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
    //         _rssi = atoi(buf);
    //         _gs_response_lines = RESPONSE_LINES_ENDED;
    //     }
    //     break;
    case GSRES_STATUS:
        if (_gs_response_lines == 0 && strncmp(buf, "NOT ASSOCIATED", 14) == 0) {
            _joined = false;
            // for (int i = 0; i < 16; i ++) {
            //     _gs_sock[i].connect = false;
            // }
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        if (_gs_response_lines == 0 && strncmp(buf, "MODE:", 5) == 0) {
            _gs_response_lines ++;
        }
        else if (_gs_response_lines == 1 && strncmp(buf, "BSSID:", 6) == 0) {
            char *tmp = strstr(buf, "SECURITY:") + 2;
            // if (strncmp(tmp, "WEP (OPEN)", 10) == NULL) {
            //     _sec = GSSECURITY_OPEN;
            // } else
            // if (strncmp(tmp, "WEP (SHARED)", 12) == NULL) {
            //     _sec = GSSECURITY_WEP;
            // } else
            // if (strncmp(tmp, "WPA-PERSONAL", 12) == NULL) {
            //     _sec = GSSECURITY_WPA_PSK;
            // } else
            // if (strncmp(tmp, "WPA2-PERSONAL", 13) == NULL) {
            //     _sec = GSSECURITY_WPA2_PSK;
            // }
            _gs_response_lines = RESPONSE_LINES_ENDED;
        }
        break;
    }

    return;
}

void GSwifi::command (const char *cmd, GSRESPONCE res, uint32_t timeout) {
    Serial.print(P("c> "));

    resetResponse(res);

    _serial->println(cmd);
    Serial.println(cmd);

    setBusy(true);
    waitResponse(timeout);
}

void GSwifi::resetResponse (GSRESPONCE res) {
    _gs_ok                = false;
    _gs_failure           = false;
    _gs_response_lines    = 0;
    _gs_expected_response = res;
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

        parse( _serial->read() );

        if ( (_gs_ok || _gs_failure) &&
             (_gs_response_lines == RESPONSE_LINES_ENDED || _gs_expected_response == GSRES_NONE) ) {
            _gs_expected_response = GSRES_NONE;
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
    command(PB("AT+BDATA=1",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
#endif

    disconnect();

    // infrastructure mode
    command(PB("AT+WM=0",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // dhcp
    if (dhcp) {
        command(PB("AT+NDHCP=1",1), GSRES_NORMAL);
    } else {
        command(PB("AT+NDHCP=0",1), GSRES_NORMAL);
    }
    if (did_timeout_) {
        return -1;
    }

    switch (sec) {
    case GSSECURITY_NONE:
    case GSSECURITY_OPEN:
    case GSSECURITY_WEP:
        sprintf(cmd, P("AT+WAUTH=%d"), sec);
        command(cmd, GSRES_NORMAL);
        if (sec != GSSECURITY_NONE) {
            sprintf(cmd, P("AT+WWEP1=%s"), pass);
            command(cmd, GSRES_NORMAL);
            // wait_ms(100);
        }
        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println(P("retry"));
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
        break;
    case GSSECURITY_WPA_PSK:
        command(PB("AT+WAUTH=0",1), GSRES_NORMAL);

        sprintf(cmd, P("AT+WWPA=%s"), pass);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println(P("retry"));
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
        break;
    case GSSECURITY_WPA2_PSK:
        command(PB("AT+WAUTH=0",1), GSRES_NORMAL);
        sprintf(cmd, P("AT+WPAPSK=%s,%s"), ssid, pass);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, P("AT+WA=%s"), ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println(P("retry"));
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
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
        command(cmd, GSRES_NORMAL);
    }

    _joined = true;
    _dhcp   = dhcp;
    return 0;
}

int GSwifi::disconnect () {
    int i;

    _joined = false;
    // for (i = 0; i < 16; i ++) {
    //     _gs_sock[i].connect = false;
    // }
    command(PB("AT+NCLOSEALL",1), GSRES_NORMAL);
    command(PB("AT+WD",1),        GSRES_NORMAL);
    command(PB("AT+NDHCP=0",1),   GSRES_NORMAL);
    return 0;
}

int GSwifi::setAddress (char *name) {
    command(PB("AT+NDHCP=1",1), GSRES_DHCP, GS_TIMEOUT2);
    if (did_timeout_) {
        return -1;
    }
    if (_ipaddr.isNull()) return -1;
    return 0;
}

int GSwifi::setAddress (IpAddr ipaddr, IpAddr netmask, IpAddr gateway, IpAddr nameserver) {
    int r;
    char cmd[GS_CMD_SIZE];

    command(PB("AT+NDHCP=0",1), GSRES_NORMAL);
    // wait_ms(100);

    sprintf(cmd, P("AT+NSET=%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d"),
        ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3],
        netmask[0], netmask[1], netmask[2], netmask[3],
        gateway[0], gateway[1], gateway[2], gateway[3]);
    command(cmd, GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    _ipaddr = ipaddr;
    _netmask = netmask;
    _gateway = gateway;

    if (ipaddr != nameserver) {
        sprintf(cmd, P("AT+DNSSET=%d.%d.%d.%d"),
            nameserver[0], nameserver[1], nameserver[2], nameserver[3]);
        command(cmd, GSRES_NORMAL);
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

    command(PB("AT+NMAC=?",1), GSRES_MACADDRESS);
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
    command(cmd, GSRES_DNSLOOKUP);
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
    command(cmd, GSRES_DNSLOOKUP);
    if (did_timeout_) {
        return -1;
    }

    host.setIp(_resolv);
    return 0;
}

bool GSwifi::isJoined () {
    return _joined;
}

GSwifi::GSPOWERSTATUS GSwifi::getPowerStatus () {
    return _power_status;
}

int GSwifi::getRssi () {
    command(PB("AT+WRSSI=?",1), GSRES_RSSI);
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
    command("", GSRES_NORMAL);

    return 0;
}

int GSwifi::setRegion (int reg) {
    char cmd[GS_CMD_SIZE];

    if (_power_status != GSPOWERSTATUS_READY) return -1;

    sprintf(cmd, P("AT+WREGDOMAIN=%d"), reg);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

#ifdef GS_ENABLE_MDNS
/**
 * mDNS
 */
int8_t GSwifi::mDNSStart() {
    command(PB("AT+MDNSSTART",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSRegisterHostname(const char *hostname) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSHNREG=%s,local"), hostname);
    command(cmd, GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDeregisterHostname(const char *hostname) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSHNDEREG=%s,local"), hostname);
    command(cmd, GSRES_NORMAL);
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
    command(cmd, GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDeregisterService(const char *name, const char *subtype, const char *type, const char *protocol) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSSRVDEREG=%s,%s,%s,%s,local"), name, subtype, type, protocol );
    command(cmd, GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSAnnounceService() {
    command(PB("AT+MDNSANNOUNCE",1), GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    return 0;
}

int8_t GSwifi::mDNSDiscoverService(const char *subtype, const char *type, const char *protocol) {
    char cmd[GS_CMD_SIZE];
    sprintf(cmd, P("AT+MDNSSD=%s,%s,%s,local"), subtype, type, protocol);
    command(cmd, GSRES_NORMAL);
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
