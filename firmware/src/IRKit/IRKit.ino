#include "Arduino.h"
#include "pins.h"
#include "const.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "IrPacker.h"
#include "FullColorLed.h"
#include "GSwifi.h"
#include "Keys.h"
#include "morse.h"
#include "timer.h"
#include "longpressbutton.h"
#include "IRKitHTTPHandler.h"
#include "commands.h"
#include "version.h"

struct morse_t morse_state;
struct long_press_button_state_t long_press_button_state;
static volatile uint8_t reconnect_timer = TIMER_OFF;
static bool morse_error = 0;
static char commands_data[COMMAND_QUEUE_SIZE];
struct RingBuffer commands;

GSwifi gs(&Serial1X);
FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );
Keys keys;
unsigned long now;
volatile char sharedbuffer[ SHARED_BUFFER_SIZE ];
extern uint16_t tree[TREE_SIZE];

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
    color.setLedColor( 1, 0, 0, false ); // red: error

    //--- initialize long press button

    pinMode( CLEAR_BUTTON, INPUT );
    long_press_button_state.pin            = CLEAR_BUTTON;
    long_press_button_state.callback       = &long_pressed;
    long_press_button_state.threshold_time = 5;

    //--- initialize morse listener

    pinMode(MICROPHONE,  INPUT);
    morse_state.letter_callback = &on_morse_letter;
    morse_state.word_callback   = &on_morse_word;
    morse_state.pin            = MICROPHONE;
    morse_setup( &morse_state, 100 );

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize( &on_ir_receive );

    //--- initialize Wifi

    pinMode(LDO33_ENABLE,     OUTPUT);
    reset_3v3();

    gs.setup( &on_disconnect, &on_reset );

    connect();

    // add your own code here!!
}

void loop() {
    now = millis(); // always run first

    morse_loop( &morse_state );

    irkit_http_loop();

    timer_loop();

    long_press_button_loop( &long_press_button_state );

    gs.loop();

    IR_loop();

    if (Serial.available()) {
        static uint8_t last_character = '0';
        static bool command_mode = false;
        last_character = Serial.read();

        Serial.write(last_character);
        Serial.println();
        Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );

        if (last_character == 0x1B) {
            command_mode = ! command_mode;
            Serial.print("command_mode:"); Serial.println(command_mode);
        }

        if (command_mode) {
            Serial1X.write(last_character);
        }
        else if (last_character == 'd') {
            keys.load();

            Serial.println();
            keys.dump();

            Serial.println();
            gs.dump();

            Serial.println();
            IR_dump();
            Serial.println();
            Serial.print("tree:");
            Serial.println(tree[ 0 ]);
            Serial.println(tree[ 1 ]);
        }
        else if (last_character == 'l') {
            long_pressed();
        }
        else if (last_character == 'v') {
            Serial.print("v:"); Serial.println(version);
        }
        /* else if (last_character == 's') { */
        /*     keys.set(GSSECURITY_WPA2_PSK, */
        /*              PB("Rhodos",1), */
        /*              PB("aaaaaaaaaaaaa",2)); */
        /*     keys.setKey(P("5284CF0D43994784897ECAB3D9935498")); */
        /*     keys.save(); */
        /* } */
    }

    // add your own code here!!
}

void reset_3v3 () {
    Serial.println(("!E25"));

    gs.reset();

    digitalWrite( LDO33_ENABLE, LOW );
    delay( 3000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );
}

void long_pressed() {
    Serial.println("long");
    keys.clear();
    keys.save();
    reset_3v3();
}

void timer_loop() {
    if (TIMER_FIRED(reconnect_timer)) {
        TIMER_STOP(reconnect_timer);
        connect();
    }

    while (! ring_isempty(&commands)) {
        char command;
        ring_get( &commands, &command, 1 );

        switch (command) {
        case COMMAND_POST_KEYS:
            irkit_httpclient_post_keys();
            break;
        case COMMAND_SETUP:
            gs.setup( &on_disconnect, &on_reset );
            // vv continues
        case COMMAND_CONNECT:
            connect();
            break;
        case COMMAND_CLOSE:
            ring_get( &commands, &command, 1 );
            gs.close(command);
            break;
        case COMMAND_START:
            start();
            break;
        default:
            break;
        }
    }
}

void on_ir_receive() {
    IR_dump();
    if (IR_packedlength() > 0) {
        irkit_httpclient_post_messages();
    }
}

void on_ir_xmit() {
    Serial.println(("xmit"));
    color.setLedColor( 0, 0, 1, true, 1 ); // xmit: blue blink for 1sec
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
    Serial.println(("!E10"));
    Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );

    ring_put( &commands, COMMAND_SETUP );
    return 0;
}

int8_t on_disconnect() {
    Serial.println(("!E11"));
    Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );

    ring_put( &commands, COMMAND_CONNECT );
    return 0;
}

void connect() {
    IR_state( IR_DISABLED );
    morse_enable( &morse_state, false );

    // load wifi credentials from EEPROM
    keys.load();

    if (keys.isWifiCredentialsSet()) {
        color.setLedColor( 1, 1, 0, true ); // yellow blink if we have valid keys

        gs.join(keys.getSecurity(),
                keys.getSSID(),
                keys.getPassword());
    }

    if (gs.isJoined()) {
        keys.setWifiWasValid(true);
        keys.save();

        color.setLedColor( 0, 1, 0, true ); // green blink: joined successfully, setting up

        irkit_httpserver_register_handler();

        // start http server
        gs.listen(80);

        // start mDNS
        gs.setupMDNS();

        if (gs.isListening()) {
            color.setLedColor( 0, 0, 1, false ); // blue: ready

            if (keys.isAPIKeySet() && ! keys.isValid()) {
                irkit_httpclient_post_door();
            }
            else if (keys.isValid()) {
                ring_put( &commands, COMMAND_START );
            }
        }
    }
    else {
        Serial.println(("!E9"));
        keys.dump();

        if (keys.wasWifiValid()) {
            // retry
            color.setLedColor( 1, 0, 0, false ); // red: error
            TIMER_START(reconnect_timer, 5);
        }
        else {
            keys.clear();
            color.setLedColor( 1, 0, 0, true ); // red blink: listening for morse
            morse_enable( &morse_state, true );
        }
    }
}

void start() {
    irkit_httpclient_start_polling( 0 );

    IR_state( IR_IDLE );
}

void on_morse_letter( char letter ) {
    Serial.print("L:"); Serial.write(letter); Serial.println();

    if (morse_error) {
        return;
    }

    int8_t result = keys.put( letter );
    if (result != 0) {
        // postpone til this "word" ends
        morse_error = true;
    }
}

void on_morse_word() {
    Serial.println("W");

    if (morse_error) {
        morse_error = false;
        keys.clear();
        return;
    }

    int8_t result = keys.putDone();
    if ( result != 0 ) {
        keys.clear();
    }
    else {
        keys.dump();
        keys.save();
        connect();
    }
}
