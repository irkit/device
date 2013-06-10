#include "Arduino.h"
#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "BLE112.h"
#include "IrCtrl.h"

// iMote git:8fa00b089894132e3f6906fea1009a4e53ce5834
SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart );

void setup() {
    // initialize status LED
    pinMode(BUSY_LED, OUTPUT);
    digitalWrite(BUSY_LED, LOW);

    pinMode(IR_OUT,   OUTPUT);
    pinMode(IR_IN,    INPUT);
    digitalWrite(IR_IN,    HIGH); // pull-up

    // USB serial
    Serial.begin(115200);

    // welcome!
    Serial.println(P("BLE112 BGAPI Scanner Demo"));

    ble112.setup();

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);

    IR_initialize();
}

void ir_recv_loop(void)
{
    if(IrCtrl.state!=IR_RECVED){
        return;
    }

    uint8_t d, i, l;
    uint16_t a;

    l = IrCtrl.len;
    switch (IrCtrl.format) {    /* Which frame arrived? */
#if IR_USE_NEC
    case NEC:    /* NEC format data frame */
        if (l == 32) {    /* Only 32-bit frame is valid */
            Serial.print(P("N "));
            Serial.print(IrCtrl.buff[0], HEX); Serial.print(P(" "));
            Serial.print(IrCtrl.buff[1], HEX); Serial.print(P(" "));
            Serial.print(IrCtrl.buff[2], HEX); Serial.print(P(" "));
            Serial.print(IrCtrl.buff[3], HEX); Serial.println();
        }
        break;
    case NEC|REPT:    /* NEC repeat frame */
        Serial.println(P("N repeat"));
        break;
#endif
#if IR_USE_AEHA
    case AEHA:        /* AEHA format data frame */
        if ((l >= 48) && (l % 8 == 0)) {    /* Only multiple of 8 bit frame is valid */
            Serial.print(P("A"));
            l /= 8;
            for (i = 0; i < l; i++){
                Serial.print(P(" "));
                Serial.print(IrCtrl.buff[i], HEX);
            }
            Serial.println();
        }
        break;
    case AEHA|REPT:    /* AEHA format repeat frame */
        Serial.println(P("A repeat"));
        break;
#endif
#if IR_USE_SONY
    case SONY:
        d = IrCtrl.buff[0];
        a = ((uint16_t)IrCtrl.buff[2] << 9) + ((uint16_t)IrCtrl.buff[1] << 1) + ((d & 0x80) ? 1 : 0);
        d &= 0x7F;
        switch (l) {    /* Only 12, 15 or 20 bit frames are valid */
        case 12:
            //xprintf(PSTR(P("S12 %u %u\n")), d, a & 0x1F);
            Serial.print(P("S12 "));
            Serial.print(d, HEX);        Serial.print(P(" "));
            Serial.print(a & 0x1F, HEX); Serial.println();
            break;
        case 15:
            //xprintf(PSTR(P("S15 %u %u\n")), d, a & 0xFF);
            Serial.print(P("S15 "));
            Serial.print(d, HEX);        Serial.print(P(" "));
            Serial.print(a & 0xFF, HEX); Serial.println();
            break;
        case 20:
            //xprintf(PSTR(P("S20 %u %u\n")), d, a & 0x1FFF);
            Serial.print(P("S20 "));
            Serial.print(d, HEX);        Serial.print(P(" "));
            Serial.print(a & 0x1FFF, HEX); Serial.println();
            break;
        }
        break;
#endif
    }
    IrCtrl.state = IR_IDLE;        /* Ready to receive next frame */
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

        // check if received
        ir_recv_loop();

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

    // AirCon Off -> OK
    // if ( IR_xmit(AEHA, (uint8_t*)"\x14\x63\x00\x10\x10\x02\xFD", 7*8) ) {
    // AirCon On -> OK
    // if ( IR_xmit(AEHA, (uint8_t*)"\x14\x63\x00\x10\x10\xFE\x09\x30\xC1\x04\x50\x00\x00\x00\x28\x93", 16*8) ) {
    /*     Serial.println( "." ); */
    /* } */
}
