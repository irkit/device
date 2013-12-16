#include "Arduino.h"
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "FullColorLed.h"
#include "version.h"
#include "GSwifi.h"
#include "Keys.h"
#include "FlexiTimer2.h"
#include "Global.h"
#include "MorseListener.h"
#include "timer.h"
#include "LongPressButton.h"
#include "base64encoder.h"
#include "IRKitHTTPClient.h"
#include "CommandQueue.h"

// Serial1(RX=D0,TX=D1) is Wifi module's UART interface
GSwifi gs(&Serial1X);
FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );
static MorseListener listener(MICROPHONE,100);
static LongPressButton clear_button(RESET_SWITCH, 5);
Keys keys;
IRKitHTTPClient client(&gs);

volatile static uint8_t reconnect_timer       = TIMER_OFF;

static bool morse_error = 0;

static char command_queue_data[COMMAND_QUEUE_SIZE + 1];
CommandQueue commands(command_queue_data, COMMAND_QUEUE_SIZE + 1);

void setup() {
    Serial.begin(115200);

    while ( ! Serial ) ;

    //--- initialize LED

    FlexiTimer2::set( TIMER_INTERVAL, &onTimer );
    FlexiTimer2::start();
    color.setLedColor( 1, 0, 0, false ); // red: error

    //--- initialize long press button

    clear_button.callback = &longPressed;

    //--- initialize morse listener

    pinMode(MICROPHONE,  INPUT);

    listener.letterCallback = &letterCallback;
    listener.wordCallback   = &wordCallback;
    listener.setup();

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize( &onIRReceive );

    //--- initialize Wifi

    pinMode(LDO33_ENABLE,     OUTPUT);
    reset3V3();

    gs.setup( &onDisconnect, &onReset );

    connect();

    // add your own code here!!
}

void loop() {
    global.loop(); // always run first

    listener.loop();

    timerLoop();

    clear_button.loop();

    // wifi
    gs.loop();

    IR_loop();

    // Wifi UART interface test
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
        }
        else if (last_character == 'l') {
            longPressed();
        }
        // else if (last_character == 's') {
        //     keys.set(GSSECURITY_WPA2_PSK,
        //              PB("Rhodos",1),
        //              PB("aaaaaaaaaaaaa",2));
        //     keys.setKey(P("5284CF0D43994784897ECAB3D9935498"));
        //     keys.save();
        // }
    }

    // add your own code here!!
}

void reset3V3 () {
    Serial.println(("!E25"));

    gs.reset();

    digitalWrite( LDO33_ENABLE, LOW );
    delay( 3000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );
}

void longPressed() {
    Serial.println("long");
    keys.clear();
    keys.save();
    reset3V3();
}

void timerLoop() {
    client.loop();

    // reconnect
    if (TIMER_FIRED(reconnect_timer)) {
        TIMER_STOP(reconnect_timer);
        connect();
    }

    while (! commands.is_empty()) {
        char command;
        commands.get( &command );

        switch (command) {
        case COMMAND_POST_KEYS:
            client.postKeys();
            break;
        case COMMAND_SETUP:
            gs.setup( &onDisconnect, &onReset );
            // vv continues
        case COMMAND_CONNECT:
            connect();
            break;
        case COMMAND_CLOSE:
            commands.get( &command );
            gs.close(command);
            break;
        case COMMAND_START:
            startNormalOperation();
            break;
        default:
            break;
        }
    }
}

void onIRReceive() {
    IR_dump();
    if (IR_packedlength() > 0) {
        client.postMessages();
    }
}

// inside ISR, be careful
void onTimer() {
    color.onTimer(); // 200msec blink

    client.onTimer();

    TIMER_TICK( reconnect_timer );

    gs.onTimer();

    IR_timer();

    clear_button.onTimer();
}

int8_t onReset() {
    Serial.println(("!E10"));
    Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );

    commands.put( COMMAND_SETUP );
    return 0;
}

int8_t onDisconnect() {
    Serial.println(("!E11"));
    Serial.print(P("F: 0x")); Serial.println( freeMemory(), HEX );

    commands.put( COMMAND_CONNECT );
    return 0;
}

void connect() {
    IR_state( IR_DISABLED );
    listener.enable(false);

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

        client.registerRequestHandler();

        // start http server
        gs.listen(80);

        // start mDNS
        gs.setupMDNS();

        if (gs.isListening()) {
            color.setLedColor( 0, 0, 1, false ); // blue: ready

            if (keys.isAPIKeySet() && ! keys.isValid()) {
                client.postDoor();
            }
            else if (keys.isValid()) {
                commands.put( COMMAND_START );
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
            listener.enable(true);
        }
    }
}

void startNormalOperation() {
    client.startTimer( 0 );

    IR_state( IR_IDLE );
}

void letterCallback( char letter ) {
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

void wordCallback() {
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
