/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Arduino.h"
#include <avr/wdt.h>
#include "pins.h"
#include "const.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "IrPacker.h"
#include "FullColorLed.h"
#include "GSwifi.h"
#include "Keys.h"
#include "timer.h"
#include "longpressbutton.h"
#include "IRKitHTTPHandler.h"
#include "commands.h"
#include "version.h"
#include "log.h"
#include "config.h"

static struct long_press_button_state_t long_press_button_state;
static volatile uint8_t reconnect_timer = TIMER_OFF;
static char commands_data[COMMAND_QUEUE_SIZE];
static FullColorLed color;

struct RingBuffer commands;
GSwifi gs(&Serial1X);
Keys keys;
volatile char sharedbuffer[ SHARED_BUFFER_SIZE ];

void setup() {
    Serial.begin(115200);

    // while ( ! Serial ) ;

    ring_init( &commands, commands_data, COMMAND_QUEUE_SIZE );

    //--- initialize timer

    timer_init( on_timer );
    timer_start( TIMER_INTERVAL );

    //--- initialize full color led

    pinMode(FULLCOLOR_LED_R, OUTPUT);
    pinMode(FULLCOLOR_LED_G, OUTPUT);
    pinMode(FULLCOLOR_LED_B, OUTPUT);
    if (config::ledFeedback < config::LED_OFF) {
        color.setLedColor( 1, 0, 0, FullColorLed::ALWAYS_ON ); // red: error
    }

    //--- initialize long press button

    pinMode( CLEAR_BUTTON, INPUT );
    long_press_button_state.pin            = CLEAR_BUTTON;
    long_press_button_state.callback       = &long_pressed;
    long_press_button_state.threshold_time = 5;

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize( &on_ir_receive );

    //--- initialize Wifi

    pinMode(LDO33_ENABLE,     OUTPUT);
    wifi_hardware_reset();

    // add your own code here!!
}

void loop() {
    irkit_http_loop();

    if (TIMER_FIRED(reconnect_timer)) {
        TIMER_STOP(reconnect_timer);
        connect();
    }

    process_commands();

    gs.loop();

    IR_loop();

#ifdef DEBUG
    if (Serial.available()) {
        static uint8_t last_character = '0';
        static bool command_mode = false;
        last_character = Serial.read();

        MAINLOG_WRITE(last_character);
        MAINLOG_PRINTLN();

        /* if (last_character == 0x1B) { */
        /*     command_mode = ! command_mode; */
        /*     MAINLOG_PRINT("command_mode:"); MAINLOG_PRINTLN(command_mode); */
        /* } */
        /* if (command_mode) { */
        /*     Serial1X.write(last_character); */
        /* } */
        /* else if (last_character == 'd') { */
        /*     MAINLOG_PRINTLN(); */
        /*     keys.load(); */
        /*     keys.dump(); */

        /*     MAINLOG_PRINTLN(); */
        /*     gs.dump(); */

        /*     MAINLOG_PRINTLN(); */
        /*     IR_dump(); */
        /* } */
        /* else if (last_character == 'l') { */
        /*     long_pressed(); */
        /* } */
        /* else if (last_character == 'v') { */
        /*     MAINLOG_PRINT("v:"); MAINLOG_PRINTLN(version); */
        /* } */
        /* else if (last_character == 's') { */
        /*     keys.set(GSSECURITY_WPA2_PSK, */
        /*              PB("Rhodos",1), */
        /*              PB("aaaaaaaaaaaaa",2)); */
        /*     keys.setKey(P("5284CF0D43994784897ECAB3D9935498")); */
        /*     keys.save(); */
        /* } */
    }
#endif

    // add your own code here!!
}

void wifi_hardware_reset () {
    MAINLOG_PRINTLN("!E25");

    // UART line powers GS and pulls 3.3V line up to around 1.4V so GS won't reset without this
    Serial1X.end();

    digitalWrite( LDO33_ENABLE, LOW );
    delay( 1000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );

    ring_put( &commands, COMMAND_SETUP );
}

void long_pressed() {
    color.setLedColor( 1, 0, 0, FullColorLed::ALWAYS_ON ); // red: error

    keys.clear();
    keys.save();
    software_reset();
}

void process_commands() {
    while (! ring_isempty(&commands)) {
        char command;
        ring_get( &commands, &command, 1 );

        switch (command) {
        case COMMAND_POST_KEYS:
            irkit_httpclient_post_keys();
            break;
        case COMMAND_SETUP:
            irkit_http_init();
            gs.setup( &on_disconnect, &on_reset );

            // vv continues
        case COMMAND_CONNECT:
            connect();
            break;
        case COMMAND_CLOSE:
            ring_get( &commands, &command, 1 );
            gs.close(command);
            break;
        case COMMAND_START_POLLING:
            irkit_httpclient_start_polling( 0 );
            break;
        case COMMAND_POST_DOOR:
            irkit_httpclient_post_door();
            break;
        case COMMAND_SETREGDOMAIN:
            {
                char regdomain;
                ring_get( &commands, &regdomain, 1 );
                gs.setRegDomain( regdomain );
            }
            break;
        default:
            break;
        }
    }
}

void on_irkit_ready() {
    // blue: ready
    if (config::ledFeedback <= config::LED_VERBOSE) {
        color.setLedColor( 0, 0, 1, FullColorLed::ALWAYS_ON ); 
    } else if (config::ledFeedback <= config::LED_QUIET) {
        color.setLedColor( 0, 0, 1, FullColorLed::BLINK_THEN_OFF, 1);
    } else {
        color.off();
    }
}

void on_ir_receive() {
    MAINLOG_PRINTLN("i<");
#ifdef IRLOG
    IR_dump();
#endif
    if (IR_packedlength() > 0) {
        if (IR_looks_like_noise()) {
            IRLOG_PRINTLN("!E31");
            return;
        }
        int8_t cid = irkit_httpclient_post_messages();
        if (cid >= 0) {
            if (config::ledFeedback <= config::LED_QUIET) {
                FullColorLed::LightMode lightMode = (config::ledFeedback == config::LED_VERBOSE)? FullColorLed::BLINK_THEN_ON : FullColorLed::BLINK_THEN_OFF;
                color.setLedColor( 0, 0, 1, lightMode, 1 ); // received: blue blink for 1sec
            } else {
                color.off();
            }
        }
    }
}

void on_ir_xmit() {
    MAINLOG_PRINTLN("i>");
    if (config::ledFeedback <= config::LED_QUIET) {
        FullColorLed::LightMode lightMode = (config::ledFeedback == config::LED_VERBOSE)? FullColorLed::BLINK_THEN_ON : FullColorLed::BLINK_THEN_OFF;
        color.setLedColor( 0, 0, 1, lightMode, 1 ); // xmit: blue blink for 1sec
    } else {
        color.off();
    }
}

// inside ISR, be careful
void on_timer() {
    color.onTimer(); // 200msec blink

    irkit_http_on_timer();

    TIMER_TICK( reconnect_timer );

    gs.onTimer();

    IR_timer();

    long_press_button_ontimer( &long_press_button_state );
}

int8_t on_reset() {
    MAINLOG_PRINTLN("!E10");

    ring_put( &commands, COMMAND_SETUP );
    return 0;
}

int8_t on_disconnect() {
    MAINLOG_PRINTLN("!E11");

    ring_put( &commands, COMMAND_CONNECT );
    return 0;
}

void connect() {
    IR_state( IR_DISABLED );

    // load wifi credentials from EEPROM
    keys.load();

    if (keys.isWifiCredentialsSet()) {
        if (config::ledFeedback < config::LED_OFF) {
            color.setLedColor( 0, 1, 0, FullColorLed::BLINK_THEN_ON ); // green blink: connecting
        }

        gs.join(keys.getSecurity(),
                keys.getSSID(),
                keys.getPassword());
    }

    if (gs.isJoined()) {
        if (config::ledFeedback < config::LED_OFF) {
            color.setLedColor( 0, 1, 1, FullColorLed::BLINK_THEN_ON ); // cyan blink: setting up
        }

        keys.setWifiWasValid(true);
        keys.save();

        // start http server
        gs.listen();
    }

    if (gs.isListening()) {
        
        if (!config::useCloudControl) {
            // No WAN access, we don't need to validate key, we're ready.
            IR_state( IR_IDLE );
            on_irkit_ready();
        } else {
            // start mDNS
            gs.setupMDNS();
    
            if (keys.isAPIKeySet() && ! keys.isValid()) {
                irkit_httpclient_post_door();
            }
            else if (keys.isValid()) {
                IR_state( IR_IDLE );
                ring_put( &commands, COMMAND_START_POLLING );
                on_irkit_ready();
            }
        }
    }
    else {
        keys.dump();

        if (keys.wasWifiValid()) {
            // retry
            if (config::ledFeedback < config::LED_OFF) {
                color.setLedColor( 1, 0, 0, FullColorLed::ALWAYS_ON ); // red: error
            }
            TIMER_START(reconnect_timer, 5);
        }
        else {
            keys.clear();
            color.setLedColor( 1, 0, 0, FullColorLed::BLINK_THEN_ON ); // red blink: listening for POST /wifi
            gs.startLimitedAP();
            if (gs.isLimitedAP()) {
                gs.listen();
            }
        }
    }
}

void software_reset() {
    wdt_enable(WDTO_15MS);
    while (1) ;
}
