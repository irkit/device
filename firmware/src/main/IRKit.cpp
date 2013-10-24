#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "FullColorLed.h"
#include "version.h"
// #include "GSwifi.h"
#include "WifiCredentials.h"
#include "FlexiTimer2.h"

#define LED_BLINK_INTERVAL 200

// down -1- up -2- down -3- up -4- down -5- up
#define VALID_IR_LEN_MIN   5

// Serial1(RX=D0,TX=D1) is Wifi module's UART interface
GSwifi gs(&Serial1);

FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

void reset3V3 () {
    Serial.println(P("hardware reset"));
    digitalWrite( LDO33_ENABLE, LOW );
    delay( 1000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );
}

void ir_recv_loop(void) {
    if ( IRDidRecvTimeout() ) {
        Serial.println(P("!!!\tIR recv timeout"));
        IR_state(IR_IDLE);
        return;
    }
    if (IrCtrl.state != IR_RECVED) {
        return;
    }
    if (IrCtrl.len < VALID_IR_LEN_MIN) {
        // data is too short = should be noise
        IR_state(IR_IDLE);
        return;
    }

    // can't receive here

    Serial.print(P("overflowed: "));  Serial.println( IrCtrl.overflowed );
    Serial.print(P("free:"));         Serial.println( freeMemory() );
    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );

    // if (ble112.current_bond_handle != INVALID_BOND_HANDLE) {
    //     // notify only when connected & authenticated
    //     ble112.writeAttributeUnreadStatus( 1 );
    // }
}

void onTimer() {
    color.toggleBlink();
}

int8_t onRequest() {
    Serial.println(P("onRequest"));
    while (!ring_isempty(gs._buf_cmd)) {
        char temp;
        ring_get(gs._buf_cmd, &temp, 1);

        Serial.print(temp, HEX);
        if (temp > 0x0D) {
            Serial.print(" ");
            Serial.write(temp);
        }
        Serial.println();
    }

    switch (gs._request.routeid) {
    case 0: // GET /recent
        gs.writeHead(200);
        gs.write(P("{}"));
        gs.end(P(""));
        break;
    case 1: // POST /send

        break;
    }
}

// void onPost(int cid, GSwifi::GS_httpd *httpd) {
//     Serial.println(P("onPost"));
// }

void printGuide(void) {
    Serial.println(P("Operations Menu:"));
    Serial.println(P("h) Print this guide"));

    Serial.println(P("b) change baud rate to 9600"));
    Serial.println(P("B) change baud rate to 115200"));
    Serial.println(P("d) dump"));
    Serial.println(P("s) set credentials"));
    Serial.println(P("v) version"));

    Serial.println(P("Command?"));
}

void IRKit_setup() {
    //--- initialize LED

    FlexiTimer2::set( LED_BLINK_INTERVAL, &onTimer );
    FlexiTimer2::start();
    color.setLedColor( 1, 0, 0 );

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize();

    //--- initialize Wifi

    reset3V3();

    gs.setup();

    // load wifi credentials from EEPROM
    {
        WifiCredentials credentials;

        if (credentials.isValid()) {
            color.setLedColor( 1, 0, 0, true );

            gs.join(credentials.getSecurity(),
                    credentials.getSSID(),
                    credentials.getPassword());
        }
        else {
            Serial.println(P("!!! EEPROM INVALID, CLEARING !!!"));
            credentials.clear();

            color.setLedColor( 1, 0, 0 );
        }

        if (gs.isJoined()) {
            color.setLedColor( 0, 1, 0, true );

            // start http server
            gs.listen(GSwifi::GSPROTOCOL_TCP, 80);

            // 0
            gs.registerRoute( GSwifi::GSMETHOD_GET,  P("/recent") );

            // 1
            gs.registerRoute( GSwifi::GSMETHOD_POST, P("/send") );

            gs.setRequestHandler( &onRequest );
        }

        if (gs.isListening()) {
            color.setLedColor( 0, 1, 0 );
        }
    }

    printGuide();
}

void IRKit_loop() {
    static bool is_command_mode = false;

    // check if received
    ir_recv_loop();

    // wifi
    if ( ! is_command_mode ) {
        gs.loop();
    }
    else {
        if (Serial1.available()) {
            Serial.write(Serial1.read());
        }
    }

    // Wifi UART interface test
    if (Serial.available()) {
        static uint8_t last_character = '0';
        last_character = Serial.read();

        Serial.print(P("> 0x"));
        Serial.print(last_character, HEX);
        Serial.println();
        Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );

        if (is_command_mode) {
            Serial1.write(last_character);

            if ( last_character == 0x1B ) {
                is_command_mode = false;
                Serial.println(P("<< command mode finished !!!!"));
            }
        }
        else if (last_character == 0x1B) {
            is_command_mode = true;
            Serial.println(P(">> entered command mode !!!!"));
        }
        else if (last_character == 'h') {
            printGuide();
        }
        else if (last_character == 'b') {
            gs.setBaud(9600);
        }
        else if (last_character == 'B') {
            gs.setBaud(115200);
        }
        else if (last_character == 'd') {
            WifiCredentials credentials;
            Serial.println(P("---credentials---"));
            credentials.dump();
            Serial.println();

            Serial.println(P("---wifi---"));
            gs.dump();
            Serial.println();

            Serial.println(P("---ir---"));
            IR_dump();
            Serial.println();
        }
        else if (last_character == 's') {
            Serial.println(P("setting credentials in EEPROM"));
            WifiCredentials credentials;
            credentials.set(GSwifi::GSSECURITY_WPA2_PSK,
                            PB("Rhodos",2),
                            PB("aaaaaaaaaaaaa",3));
            credentials.save();
        }
        else if (last_character == 'v') {
            Serial.print(P("version: "));
            Serial.println(version);
        }
    }
}
