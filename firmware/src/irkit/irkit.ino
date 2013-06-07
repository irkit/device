#include "Arduino.h"
#include <SoftwareSerial.h>
#include "pins.h"
#include "BLE112.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"

// iMote git:8fa00b089894132e3f6906fea1009a4e53ce5834
SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart );

void setup() {
    // initialize status LED
    pinMode(BUSY_LED, OUTPUT);
    digitalWrite(BUSY_LED, LOW);

    // USB serial
    Serial.begin(115200);

    // welcome!
    Serial.println(P("BLE112 BGAPI Scanner Demo"));

    ble112.setup();

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    Serial.println(P("Operations Menu:"));
    Serial.println(P("0) Reset BLE112 module"));
    Serial.println(P("1) Hello"));
    Serial.println(P("2) Set gap mode(2,2)"));
    Serial.println(P("3) Get rssi"));
    Serial.println(P("4) Write attribute"));
    Serial.println(P("5) Read attribute"));
    Serial.println(P("Command?"));
    while (1) {
        // keep polling for new data from BLE
        ble112.loop();

        // check for input from the user
        if (Serial.available()) {

            Serial.print(P("free:"));
            Serial.println( freeMemory() );

            lastCharacter = Serial.read();
            Serial.print(P("0x"));
            Serial.println( lastCharacter, HEX );

            uint8_t status;
            if (lastCharacter == '0') {
                // Reset BLE112 module
                Serial.println(P("-->\tsystem_reset: { boot_in_dfu: 0 }"));

                ble112.reset();
            }
            else if (lastCharacter == '1') {
                // Say hello to the BLE112 and wait for response
                Serial.println(P("-->\tsystem_hello"));

                ble112.hello();
            }
            else if (lastCharacter == '2') {
                Serial.println(P("-->\tgap_set_mode: { discover: 0x2, connect: 0x2 }"));

                ble112.setMode();
            }
            else if (lastCharacter == '3') {
                Serial.println(P("-->\tconnection_get_rssi"));

                ble112.getRSSI();
            }
            else if (lastCharacter == '4') {
                ble112.writeAttribute();
            }
            else if (lastCharacter == '5') {
                ble112.readAttribute();
            }
        }
    }
}
