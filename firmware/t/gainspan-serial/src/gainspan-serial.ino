#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "HardwareSerialX.h"

#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 64
extern volatile struct RingBuffer rx_buffer1;
extern volatile char rx_buffer1_data[RX_BUFFER_SIZE + 1];
extern volatile struct RingBuffer tx_buffer1;
extern volatile char tx_buffer1_data[TX_BUFFER_SIZE + 1];
extern volatile bool is_limiting;
extern volatile bool send_xon;
extern volatile bool send_xoff;

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ) ;

    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // gainspan
    Serial1X.begin(38400);
}

void loop() {
    // gainspan -> usb
    if (Serial1X.available()) {
        Serial.write(Serial1X.read());
    }

    // usb -> gainspan
    if (Serial.available()) {
        static uint8_t last_character = '0';

        last_character = Serial.read();

        if (last_character == 'd') {
            Serial.println("rx:");
            volatile struct RingBuffer *buf = &rx_buffer1;
            Serial.print("size: "); Serial.println(buf->size);
            Serial.print("r: "); Serial.println(buf->addr_r);
            Serial.print("w: "); Serial.println(buf->addr_w);
            for (uint8_t i=0; i<64; i++) {
                Serial.print(rx_buffer1_data[i], HEX); Serial.print(" ");
            }
            Serial.println();
            Serial.println("tx:");
            buf = &tx_buffer1;
            Serial.print("size: "); Serial.println(buf->size);
            Serial.print("r: "); Serial.println(buf->addr_r);
            Serial.print("w: "); Serial.println(buf->addr_w);
            for (uint8_t i=0; i<64; i++) {
                Serial.print(tx_buffer1_data[i], HEX); Serial.print(" ");
            }
            Serial.println();
            Serial.print("is_limiting:"); Serial.println(is_limiting);
            Serial.print("send_xon:");    Serial.println(send_xon);
            Serial.print("send_xoff:");   Serial.println(send_xoff);
        }
        else {
            Serial1X.write(last_character);
            Serial.write(last_character);
        }
    }
}
