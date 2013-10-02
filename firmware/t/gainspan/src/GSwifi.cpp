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

// #include "mbed.h"
#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "GSwifi.h"

// GSwifi::GSwifi (PinName p_tx, PinName p_rx, PinName p_reset, PinName p_alarm, int baud)
//   : _gs(p_tx, p_rx), _reset(p_reset), _buf_cmd(GS_CMD_SIZE) {
GSwifi::GSwifi( HardwareSerial *serial ) :
    _serial(serial),
    _buf_cmd(GS_CMD_SIZE),
    _rts(false)
{
}

int8_t GSwifi::setup() {
    reset();

    _serial->begin(9600);

    command("AT", GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    setBaud( GS_BAUD );

    // disable echo
    command("ATE0", GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    // UART hardware flow control
    if (_rts) {
        command("AT&R1", GSRES_NORMAL);
    }
}

void GSwifi::reset () {

    memset(&_gs_sock, 0, sizeof(_gs_sock));
    memset(&_mac, 0, sizeof(_mac));
    _connect = false;
    _status  = GSSTAT_READY;
    _escape  = false;
    resetResponse(GSRES_NONE);
    _gs_mode = GSMODE_COMMAND;
    _dhcp    = false;
    _ssid    = NULL;
    _pass    = NULL;
    _reconnect      = 0;
    _reconnect_time = 0;
    _buf_cmd.flush();
}

// received a character from UART
void GSwifi::parse(uint8_t dat) {
    Serial.print(P("p< 0x"));
    Serial.print(dat, HEX);
    if (dat > 0x0D) {
        Serial.print(P(" "));
        Serial.write(dat);
    }
    Serial.println();

    static int len, mode;
    static char tmp[20];
    // char dat;

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
        } else {
            if (dat == 0x1b) {
                _escape = true;
            } else
            if (dat == '\n') {
                // end of line
                parseResponse();
            } else
            if (dat != '\r') {
                // command
                _buf_cmd.queue(dat);
            }
        }
        break;

    case GSMODE_DATA_RX:
    case GSMODE_DATA_RXUDP:
        if (mode == 0) {
            // cid
            _cid = x2i(dat);
            _gs_sock[_cid].received = false;
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
                _gs_sock[_cid].received = true;
                _gs_mode = GSMODE_COMMAND;

                if (_gs_sock[_cid].protocol == GSPROT_HTTPGET) {
                    // recv interrupt
                    // if (_gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available()) == 0)
                        _gs_sock[_cid].received = false;
                }
                break;
            default:
                // Serial.println("unknown <ESC> %02x", dat);
                break;
            }
            _escape = false;
        } else {
            if (dat == 0x1b) {
                _escape = true;
            } else {
                // data
                if (_gs_sock[_cid].data != NULL) {
                  _gs_sock[_cid].data->queue(dat);
                  len ++;

                  // if (len < GS_DATA_SIZE && _gs_sock[_cid].data->isFull()) {
                    // buffer full
                    // recv interrupt
                  //   _gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available());
                  // }
                }
            }
        }
        break;

    case GSMODE_DATA_RX_BULK:
    case GSMODE_DATA_RXUDP_BULK:
        if (mode == 0) {
            // cid
            _cid = x2i(dat);
            _gs_sock[_cid].received = false;
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
            if (_gs_sock[_cid].data != NULL) {
              _gs_sock[_cid].data->queue(dat);
              len  --;

              if (len && _gs_sock[_cid].data->isFull()) {
                // buffer full
                // recv interrupt
                // _gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available());
              }
            }
            if (len == 0) {
                // Serial.println("recv binary %d", _cid);
                _gs_sock[_cid].received = true;
                _escape = false;
                _gs_mode = GSMODE_COMMAND;

                if (_gs_sock[_cid].protocol == GSPROT_HTTPGET) {
                    // recv interrupt
                    // if (_gs_sock[_cid].onGsReceive.call(_cid, _gs_sock[_cid].data->available()) == 0)
                    //     _gs_sock[_cid].received = false;
                }
            }
        }
        break;

    }
}

void GSwifi::parseResponse () {
    int i;
    char buf[GS_CMD_SIZE];

    Serial.println(P("parseResponse"));

    while (! _buf_cmd.isEmpty()) {
        // received "\n"
        i = 0;
        while ((! _buf_cmd.isEmpty()) && i < sizeof(buf)) {
            _buf_cmd.dequeue(&buf[i]);
            if (buf[i] == '\n') {
                break;
            }
            i ++;
        }
        if (i == 0) continue;
        buf[i] = 0;
        Serial.print(P("parseResponse: ")); Serial.println(buf);

        parseCmdResponse(buf);

        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[8] >= '0' && buf[8] <= 'F' && buf[9] != 0) {
            int cid = x2i(buf[8]);
            if (_gs_sock[cid].type == GSTYPE_SERVER) {
                // fork (server socket)
                int acid, ip1, ip2, ip3, ip4;
                char *tmp = buf + 12;

                acid = x2i(buf[10]);
                // Serial.println("connect %d -> %d", cid, acid);
                // newSock(acid, _gs_sock[cid].type, _gs_sock[cid].protocol);
                // _gs_sock[acid].onGsReceive = _gs_sock[cid].onGsReceive;
                _gs_sock[acid].lcid = cid;
                sscanf(tmp, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                _gs_sock[acid].host.setIp(IpAddr(ip1, ip2, ip3, ip4));
                tmp = strstr(tmp, " ") + 1;
                _gs_sock[acid].host.setPort(atoi(tmp));
                // _gs_sock[acid].onGsReceive.call(acid, 0); // event connected
            }
        } else
        if (strncmp(buf, "DISCONNECT ", 11) == 0) {
            int cid = x2i(buf[11]);
            // Serial.println("disconnect %d", cid);
            _gs_sock[cid].connect = false;
            // _gs_sock[cid].onGsReceive.call(cid, -1); // event disconnected
        } else
        if (strncmp(buf, "DISASSOCIATED", 13) == 0 ||
          strncmp(buf, "Disassociated", 13) == 0 ||
          strncmp(buf, "Disassociation Event", 20) == 0 ) {
            _connect = false;
            for (i = 0; i < 16; i ++) {
                _gs_sock[i].connect = false;
            }
        } else
        if (strncmp(buf, "UnExpected Warm Boot", 20) == 0 ||
          strncmp(buf, "APP Reset-APP SW Reset", 22) == 0 ||
          strncmp(buf, "APP Reset-Wlan Except", 21) == 0 ) {
            Serial.println("disassociate");
            _connect = false;
            _status = GSSTAT_READY;
            _escape = false;
            resetResponse(GSRES_NONE);
            _gs_mode = GSMODE_COMMAND;
            for (i = 0; i < 16; i ++) {
                _gs_sock[i].connect = false;
            }
        } else
        if (strncmp(buf, "Out of StandBy-Timer", 20) == 0 ||
          strncmp(buf, "Out of StandBy-Alarm", 20) == 0) {
            if (_status == GSSTAT_STANDBY) {
                _status = GSSTAT_WAKEUP;
            }
        } else
        if (strncmp(buf, "Out of Deep Sleep", 17) == 0 ) {
            if (_status == GSSTAT_DEEPSLEEP) {
                _status = GSSTAT_READY;
            }
        }
        Serial.print(P("status: ")); Serial.println(_status, HEX);
    }
}

void GSwifi::parseCmdResponse (char *buf) {
    if (_gs_res == GSRES_NONE) return;

    if (strcmp(buf, "OK") == 0) {
        _gs_ok = true;
    } else
    if (strncmp(buf, "ERROR", 5) == 0) {
        _gs_failure = true;
    }

    switch(_gs_res) {
    case GSRES_NORMAL:
        _gs_flg = -1;
        break;
    case GSRES_WPS:
        if (_gs_flg == 0 && strncmp(buf, "SSID=", 5) == 0) {
            if (!_ssid) _ssid = (char*)malloc(strlen(&buf[5]) + 1);
            strcpy(_ssid, &buf[5]);
            _gs_flg ++;
        } else
        if (_gs_flg == 1 && strncmp(buf, "CHANNEL=", 8) == 0) {
            _gs_flg ++;
        } else
        if (_gs_flg == 2 && strncmp(buf, "PASSPHRASE=", 11) == 0) {
            if (!_pass) _pass = (char*)malloc(strlen(&buf[11]) + 1);
            strcpy(_pass, &buf[11]);
            _gs_flg = -1;
        }
        break;
    case GSRES_CONNECT:
        if (strncmp(buf, "CONNECT ", 8) == 0 && buf[9] == 0) {
            _cid = x2i(buf[8]);
            _gs_flg = -1;
        }
        break;
    case GSRES_DHCP:
        if (_gs_flg == 0 && strstr(buf, "SubNet") && strstr(buf, "Gateway")) {
            _gs_flg ++;
        } else
        if (_gs_flg == 1) {
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
            _gs_flg = -1;
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
            _gs_flg = -1;
        }
        break;
    case GSRES_DNSLOOKUP:
        if (strncmp(buf, "IP:", 3) == 0) {
            int ip1, ip2, ip3, ip4;
            sscanf(&buf[3], "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
            _resolv = IpAddr(ip1, ip2, ip3, ip4);
            _gs_flg = -1;
        }
        break;
    case GSRES_HTTP:
        if (buf[0] >= '0' && buf[0] <= 'F' && buf[1] == 0) {
            _cid = x2i(buf[0]);
            _gs_flg = -1;
        }
        break;
    case GSRES_RSSI:
        if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
            _rssi = atoi(buf);
            _gs_flg = -1;
        }
        break;
    case GSRES_TIME:
        if (buf[0] >= '0' && buf[0] <= '9') {
            // int year, month, day, hour, min, sec;
            // struct tm t;
            // sscanf(buf, "%d/%d/%d,%d:%d:%d", &day, &month, &year, &hour, &min, &sec);
            // t.tm_sec = sec;
            // t.tm_min = min;
            // t.tm_hour = hour;
            // t.tm_mday = day;
            // t.tm_mon = month - 1;
            // t.tm_year = year - 1900;
            // _time = mktime(&t);
            // _gs_flg = -1;
        }
        break;
    case GSRES_STATUS:
        if (_gs_flg == 0 && strncmp(buf, "NOT ASSOCIATED", 14) == 0) {
            _connect = false;
            for (int i = 0; i < 16; i ++) {
                _gs_sock[i].connect = false;
            }
            _gs_flg = -1;
        }
        if (_gs_flg == 0 && strncmp(buf, "MODE:", 5) == 0) {
            _gs_flg ++;
        }
        else if (_gs_flg == 1 && strncmp(buf, "BSSID:", 6) == 0) {
            char *tmp = strstr(buf, "SECURITY:") + 2;
            if (strncmp(tmp, "WEP (OPEN)", 10) == NULL) {
                _sec = GSSEC_OPEN;
            } else
            if (strncmp(tmp, "WEP (SHARED)", 12) == NULL) {
                _sec = GSSEC_WEP;
            } else
            if (strncmp(tmp, "WPA-PERSONAL", 12) == NULL) {
                _sec = GSSEC_WPA_PSK;
            } else
            if (strncmp(tmp, "WPA2-PERSONAL", 13) == NULL) {
                _sec = GSSEC_WPA2_PSK;
            }
            _gs_flg = -1;
        }
        break;
    }

    return;
}

void GSwifi::poll () {
    int i, j;

    for (i = 0; i < 16; i ++) {
        if (_gs_sock[i].connect && _gs_sock[i].received) {
          if (_gs_sock[i].data && ! _gs_sock[i].data->isEmpty()) {
            // recv interrupt
            _gs_sock[i].received = false;
            // for (j = 0; j < 1500 / GS_DATA_SIZE + 1; j ++) {
            //     if (! _gs_sock[i].connect || _gs_sock[i].data->isEmpty()) break;
            //     _gs_sock[i].onGsReceive.call(i, _gs_sock[i].data->available());
            // }
          }
        }
    }

    // if (_reconnect > 0 && ! _connect) {
    //     if (_reconnect_time < time(NULL)) {
    //         Serial.println("reconnect %d", _reconnect_time);
    //         if (reconnect()) {
    //             _reconnect_time = time(NULL) + _reconnect;
    //         }
    //     }
    // }
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
    _gs_ok      = false;
    _gs_failure = false;
    _gs_flg     = 0;
    _gs_res     = res;
}

bool GSwifi::setBusy(bool busy) {
    if (busy) {
        timeout_start_ = millis();
        did_timeout_ = false;
        // if (onBusy) onBusy();
    } else {
        // lastError = false;
        // if (onIdle) onIdle();
    }
    return busy_ = busy;
}

uint8_t GSwifi::checkActivity(uint32_t timeout_ms) {
    uint16_t character;
    while ( (character = _serial->read()) < 256 &&
            ( (timeout_ms == 0) ||
              millis() - timeout_start_ < timeout_ms ) ) {
        parse(character);

        if (timeout_ms > 0) {
            timeout_start_ = millis();
        }

        if ( (_gs_ok || _gs_failure) &&
             (_gs_flg == -1 || _gs_res == GSRES_NONE) ) {
            _gs_res = GSRES_NONE;
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
    Serial.print(P("waitResponse: ")); Serial.println(ms);

    while ( checkActivity(ms) ) ;
    return;
}

int GSwifi::connect (GSSECURITY sec, const char *ssid, const char *pass, int dhcp, int reconnect, char *name) {
    int r;
    char cmd[GS_CMD_SIZE];

    if (_connect || _status != GSSTAT_READY) return -1;

    if (getMacAddress(_mac)) {
        return -1;
    }

#ifdef GS_BULK
    command("AT+BDATA=1", GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
#endif

    disconnect();

    // infrastructure mode
    command("AT+WM=0", GSRES_NORMAL);

    // dhcp
    if (dhcp && (sec != GSSEC_WPS_BUTTON)) {
        command("AT+NDHCP=1", GSRES_NORMAL);
    } else {
        command("AT+NDHCP=0", GSRES_NORMAL);
    }

    switch (sec) {
    case GSSEC_NONE:
    case GSSEC_OPEN:
    case GSSEC_WEP:
        sprintf(cmd, "AT+WAUTH=%d", sec);
        command(cmd, GSRES_NORMAL);
        if (sec != GSSEC_NONE) {
            sprintf(cmd, "AT+WWEP1=%s", pass);
            command(cmd, GSRES_NORMAL);
            // wait_ms(100);
        }
        sprintf(cmd, "AT+WA=%s", ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println("retry");
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
        break;
    case GSSEC_WPA_PSK:
    case GSSEC_WPA_ENT:
        command("AT+WAUTH=0", GSRES_NORMAL);

        sprintf(cmd, "AT+WWPA=%s", pass);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, "AT+WA=%s", ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println("retry");
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
        break;
    case GSSEC_WPA2_PSK:
    case GSSEC_WPA2_ENT:
        command("AT+WAUTH=0", GSRES_NORMAL);
        sprintf(cmd, "AT+WPAPSK=%s,%s", ssid, pass);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);

        sprintf(cmd, "AT+WA=%s", ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            Serial.println("retry");
            // wait_ms(1000);
            command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        }
        break;
    case GSSEC_WPS_BUTTON:
        command("AT+WAUTH=0", GSRES_NORMAL);
        command("AT+WWPS=1", GSRES_WPS, GS_TIMEOUT2);
        if (did_timeout_) {
            break;
        }
        if (dhcp) {
            r = setAddress(name);
        }
        break;
    case GSSEC_WPS_PIN:
        command("AT+WAUTH=0", GSRES_NORMAL);
        sprintf(cmd, "AT+WWPS=2,%s", pass);
        command(cmd, GSRES_WPS, GS_TIMEOUT2);
        if (did_timeout_) {
            break;
        }
        if (dhcp) {
            r = setAddress(name);
        }
        break;
    default:
        Serial.println("Can't use security");
        r = -1;
        break;
    }

    if (r == 0 && !dhcp) {
        sprintf(cmd, "AT+DNSSET=%d.%d.%d.%d",
            _gateway[0], _gateway[1], _gateway[2], _gateway[3]);
        command(cmd, GSRES_NORMAL);
    }

    if (r == 0) {
        _connect        = true;
        _reconnect      = reconnect;
        _reconnect_time = 0;
        _sec            = sec;
        _dhcp           = dhcp;
        if (_reconnect && ssid) {
            if (!_ssid) _ssid = (char*)malloc(strlen(ssid) + 1);
            strcpy(_ssid, ssid);
        }
        if (_reconnect && pass) {
            if (!_pass) _pass = (char*)malloc(strlen(pass) + 1);
            strcpy(_pass, pass);
        }
    }
    return r;
}

int GSwifi::adhock (GSSECURITY sec, const char *ssid, const char *pass, IpAddr ipaddr, IpAddr netmask) {
    int r;
    char cmd[GS_CMD_SIZE];

    if (_connect || _status != GSSTAT_READY) return -1;

    command(NULL, GSRES_NORMAL);
    command("ATE0", GSRES_NORMAL);
    if (did_timeout_){
        return -1;
    }
    if (_rts) {
        command("AT&R1", GSRES_NORMAL);
    }
    if (getMacAddress(_mac)) return -1;
#ifdef GS_BULK
    command("AT+BDATA=1", GSRES_NORMAL);
#endif

    disconnect();
    command("AT+WM=1", GSRES_NORMAL); // adhock
    // wait_ms(100);
    command("AT+NDHCP=0", GSRES_NORMAL);
    setAddress(ipaddr, netmask, ipaddr, ipaddr);

    switch (sec) {
    case GSSEC_NONE:
    case GSSEC_OPEN:
    case GSSEC_WEP:
        sprintf(cmd, "AT+WAUTH=%d", sec);
        command(cmd, GSRES_NORMAL);
        if (sec != GSSEC_NONE) {
            sprintf(cmd, "AT+WWEP1=%s", pass);
            command(cmd, GSRES_NORMAL);
            // wait_ms(100);
        }
        sprintf(cmd, "AT+WA=%s", ssid);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);
        if (did_timeout_) {
            return -1;
        }
        break;
    default:
        Serial.println("Can't use security");
        r = -1;
        break;
    }

    if (r == 0) _connect = true;
    return r;
}

int GSwifi::limitedap (GSSECURITY sec, const char *ssid, const char *pass, IpAddr ipaddr, IpAddr netmask, char *dns) {
    int r;
    char cmd[GS_CMD_SIZE];

    if (_connect || _status != GSSTAT_READY) return -1;

    command(NULL, GSRES_NORMAL);
    command("ATE0", GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    if (_rts) {
        command("AT&R1", GSRES_NORMAL);
    }
    if (getMacAddress(_mac)) return -1;
#ifdef GS_BULK
    command("AT+BDATA=1", GSRES_NORMAL);
#endif

    disconnect();
    command("AT+WM=2", GSRES_NORMAL); // limited ap
    // wait_ms(100);
    command("AT+NDHCP=0", GSRES_NORMAL);
    setAddress(ipaddr, netmask, ipaddr, ipaddr);
    command("AT+DHCPSRVR=1", GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }
    if (dns) {
        sprintf(cmd, "AT+DNS=1,%s", dns);
    } else {
        strcpy(cmd, "AT+DNS=1," GS_DNSNAME);
    }
    command(cmd, GSRES_NORMAL);
    if (did_timeout_) {
        return -1;
    }

    switch (sec) {
    case GSSEC_NONE:
    case GSSEC_OPEN:
    case GSSEC_WEP:
        sprintf(cmd, "AT+WAUTH=%d", sec);
        command(cmd, GSRES_NORMAL);
        if (sec != GSSEC_NONE) {
            sprintf(cmd, "AT+WWEP1=%s", pass);
            command(cmd, GSRES_NORMAL);
            // wait_ms(100);
        }
        sprintf(cmd, "AT+WA=%s", ssid);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);
        if (did_timeout_) {
            return -1;
        }
        break;
    default:
        Serial.println("Can't use security");
        r = -1;
        break;
    }

    if (r == 0) _connect = true;
    return r;
}

int GSwifi::disconnect () {
    int i;

    _connect = false;
    for (i = 0; i < 16; i ++) {
        _gs_sock[i].connect = false;
    }
    command("AT+NCLOSEALL", GSRES_NORMAL);
    command("AT+WD",        GSRES_NORMAL);
    command("AT+NDHCP=0",   GSRES_NORMAL);
    return 0;
}

int GSwifi::reconnect () {
    int r;
    char cmd[GS_CMD_SIZE];

    if (_connect || _status != GSSTAT_READY) return -1;
    if (!_ssid) return -1;

    switch (_sec) {
    case GSSEC_WPS_BUTTON:
    case GSSEC_WPS_PIN:
        sprintf(cmd, "AT+WPAPSK=%s,%s", _ssid, _pass);
        command(cmd, GSRES_NORMAL, GS_TIMEOUT2);
        // wait_ms(100);
    case GSSEC_NONE:
    case GSSEC_OPEN:
    case GSSEC_WEP:
    case GSSEC_WPA_PSK:
    case GSSEC_WPA_ENT:
    case GSSEC_WPA2_PSK:
    case GSSEC_WPA2_ENT:
        if (_dhcp) {
            command("AT+NDHCP=1", GSRES_NORMAL);
        }
        sprintf(cmd, "AT+WA=%s", _ssid);
        command(cmd, GSRES_DHCP, GS_TIMEOUT2);
        if (did_timeout_) {
            return -1;
        }
        break;
    default:
        Serial.println("Can't use security");
        r = -1;
        break;
    }

    if (r == 0) _connect = true;
    return r;
}

int GSwifi::setAddress (char *name) {
    command("AT+NDHCP=1", GSRES_DHCP, GS_TIMEOUT2);
    if (did_timeout_) {
        return -1;
    }
    if (_ipaddr.isNull()) return -1;
    return 0;
}

int GSwifi::setAddress (IpAddr ipaddr, IpAddr netmask, IpAddr gateway, IpAddr nameserver) {
    int r;
    char cmd[GS_CMD_SIZE];

    command("AT+NDHCP=0", GSRES_NORMAL);
    // wait_ms(100);

    sprintf(cmd, "AT+NSET=%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d",
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
        sprintf(cmd, "AT+DNSSET=%d.%d.%d.%d",
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

    command("AT+NMAC=?", GSRES_MACADDRESS);
    if (_mac[0] || _mac[1] || _mac[2] || _mac[3] || _mac[4] || _mac[5]) {
        memcpy(mac, _mac, 6);
        return 0;
    }
    return -1;
}

int GSwifi::getHostByName (const char* name, IpAddr &addr) {
    char cmd[GS_CMD_SIZE];

    if (! _connect || _status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+DNSLOOKUP=%s", name);
    command(cmd, GSRES_DNSLOOKUP);
    if (did_timeout_) {
        return -1;
    }

    addr = _resolv;
    return 0;
}

int GSwifi::getHostByName (Host &host) {
    char cmd[GS_CMD_SIZE];

    if (! _connect || _status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+DNSLOOKUP=%s", host.getName());
    command(cmd, GSRES_DNSLOOKUP);
    if (did_timeout_) {
        return -1;
    }

    host.setIp(_resolv);
    return 0;
}

bool GSwifi::isConnected () {
/*
    if (_status == GSSTAT_READY) {
        command("AT+WSTATUS", GSRES_STATUS);
    }
*/
    return _connect;
}

GSwifi::GSSTATUS GSwifi::getStatus () {
    return _status;
}

int GSwifi::getRssi () {
    command("AT+WRSSI=?", GSRES_RSSI);
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

    if (_status != GSSTAT_READY) {
        return -1;
    }

    sprintf(cmd, "ATB=%d\r\n", baud);
    _serial->println(cmd);

    _serial->begin(baud);

    // sleep_ms(100);

    resetResponse(GSRES_NORMAL);

    setBusy(true);
    waitResponse(GS_TIMEOUT);

    return 0;
}

int GSwifi::setRegion (int reg) {
    char cmd[GS_CMD_SIZE];

    if (_status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+WREGDOMAIN=%d", reg);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

#ifndef GS_LIB_TINY
int GSwifi::setRFPower (int power) {
    char cmd[GS_CMD_SIZE];

    if (power < 0 || power > 7) return -1;

    sprintf(cmd, "AT+WP=%d", power);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

int GSwifi::powerSave (int active, int save) {
    char cmd[GS_CMD_SIZE];

    if (_status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+WRXACTIVE=%d", active);
    command(cmd, GSRES_NORMAL);
    sprintf(cmd, "AT+WRXPS=%d", save);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

int GSwifi::standby (int msec) {
    int i;
    char cmd[GS_CMD_SIZE];

    if (_status != GSSTAT_READY && _status != GSSTAT_WAKEUP) return -1;

    if (_status == GSSTAT_READY) {
//        command("AT+WRXACTIVE=0", GSRES_NORMAL);
        command("AT+STORENWCONN", GSRES_NORMAL, 100);
    } else {
        command("ATE0", GSRES_NORMAL);
        if (_rts) {
            command("AT&R1", GSRES_NORMAL);
        }
    }
    for (i = 0; i < 16; i ++) {
        _gs_sock[i].connect = false;
    }
    _status = GSSTAT_STANDBY;
    sprintf(cmd, "AT+PSSTBY=%d,0,0,0", msec); // go standby
    command(cmd, GSRES_NORMAL, 0);
    return did_timeout_;
}

int GSwifi::wakeup () {

//     if (_status == GSSTAT_STANDBY && _alarm != NULL) {
//         Timer timeout;
//         _alarm->output(); // low
//         _alarm->write(0);
//         timeout.start();
//         while (_status != GSSTAT_WAKEUP && timeout.read() < GS_TIMEOUT) {
//             poll();
//         }
//         timeout.stop();
//         _alarm->input(); // high
//         _alarm->mode(PullUp);
//     }

//     if (_status == GSSTAT_WAKEUP) {
//         _status = GSSTAT_READY;
//         command("ATE0", GSRES_NORMAL);
//         if (_rts) {
//             command("AT&R1", GSRES_NORMAL);
//         }
// #ifdef GS_BULK
//         command("AT+BDATA=1", GSRES_NORMAL);
// #endif
//         int r = command("AT+RESTORENWCONN", GSRES_NORMAL);
//         wait_ms(100);
// //        return command("AT+WRXACTIVE=1", GSRES_NORMAL);
//         return r;
//     } else
//     if (_status == GSSTAT_DEEPSLEEP) {
//         _status = GSSTAT_READY;
//         return command("AT", GSRES_NORMAL);
//     }
    return -1;
}

int GSwifi::deepSleep () {

    if (_status != GSSTAT_READY) return -1;

    _status = GSSTAT_DEEPSLEEP;
    command("AT+PSDPSLEEP", GSRES_NORMAL, 0); // go deep sleep
    return did_timeout_;
}

int GSwifi::ntpdate (Host host, int sec) {
    char cmd[GS_CMD_SIZE];

    if (! _connect || _status != GSSTAT_READY) return -1;

    if (host.getIp().isNull()) {
        if (getHostByName(host)) {
            if (getHostByName(host)) return -1;
        }
    }

    if (sec) {
        sprintf(cmd, "AT+NTIMESYNC=1,%d.%d.%d.%d,%d,1,%d", host.getIp()[0], host.getIp()[1], host.getIp()[2], host.getIp()[3],
          GS_TIMEOUT / 1000, sec);
    } else {
        sprintf(cmd, "AT+NTIMESYNC=1,%d.%d.%d.%d,%d,0", host.getIp()[0], host.getIp()[1], host.getIp()[2], host.getIp()[3],
          GS_TIMEOUT / 1000);
    }
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

// int GSwifi::setTime (time_t time) {
//     char cmd[GS_CMD_SIZE];
//     struct tm *t;

//     if (_status != GSSTAT_READY) return -1;

//     t = localtime(&time);
//     sprintf(cmd, "AT+SETTIME=%d/%d/%d,%d:%d:%d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
//     return command(cmd, GSRES_NORMAL);
// }

// time_t GSwifi::getTime () {

//     if (command("AT+GETTIME=?", GSRES_TIME)) {
//         return 0;
//     }
//     return _time;
// }

int GSwifi::gpioOut (int port, int out) {
    char cmd[GS_CMD_SIZE];

    if (_status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+DGPIO=%d,%d", port, out);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}

int GSwifi::certAdd (const char *name, const char *cert, int len) {
    char cmd[GS_CMD_SIZE];

    if (! _connect || _status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+TCERTADD=%s,1,%d,1", name, len);  // Hex, ram
    command(cmd, GSRES_NORMAL);

    resetResponse(GSRES_NORMAL);
    // _gs_putc(0x1b);
    // _gs_putc('W');
    // for (i = 0; i < len; i ++) {
    //     _gs_putc(cert[i]);
    // }
    waitResponse(GS_TIMEOUT);
    return did_timeout_;
}

int GSwifi::provisioning (char *user, char *pass) {
    char cmd[GS_CMD_SIZE];

    if (_status != GSSTAT_READY) return -1;

    sprintf(cmd, "AT+WEBPROV=%s,%s", user, pass);
    command(cmd, GSRES_NORMAL);
    return did_timeout_;
}
#endif

int GSwifi::from_hex (int ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

int GSwifi::to_hex (int code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

int GSwifi::x2i (char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

char GSwifi::i2x (int i) {
    if (i >= 0 && i <= 9) {
        return i + '0';
    } else
    if (i >= 10 && i <= 15) {
        return i - 10 + 'A';
    }
    return '0';
}

// for test
// void GSwifi::dump () {
//     int i;

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
// }
