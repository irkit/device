/* Copyright (C) 2013 gsfan, MIT License
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
#ifndef __GSWIFI_CONST_H__
#define __GSWIFI_CONST_H__

#define DOMAIN "api.getirkit.com"

#define GS_TIMEOUT             20 // [s]
#define GS_TIMEOUT_LONG        50 // [s]
#define GS_TIMEOUT_SHORT        1
#define GS_TIMEOUT_NOWAIT       0

#define GS_CMD_SIZE            64

#define GS_MAX_ROUTES           4
#define GS_MAX_PATH_LENGTH      9    // max: "/messages"

enum GSSECURITY {
    GSSECURITY_AUTO     = 0,
    GSSECURITY_NONE     = 0,
    GSSECURITY_OPEN     = 1,
    GSSECURITY_WEP      = 2,
    GSSECURITY_WPA_PSK  = 4,
    GSSECURITY_WPA2_PSK = 8,
};

#endif
