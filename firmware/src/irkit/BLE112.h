#ifndef __BLE112_H__
#define __BLE112_H__

#include "BGLib.h"

// imported from firmware/ble112/attributes.txt
#define ATTRIBUTE_HANDLE_IR_DATA              17
#define ATTRIBUTE_HANDLE_IR_UNREAD_STATUS     20
#define ATTRIBUTE_HANDLE_IR_CONTROL_POINT     24
#define ATTRIBUTE_HANDLE_IR_CARRIER_FREQUENCY 27
#define ATTRIBUTE_HANDLE_IR_AUTH_STATUS       30

class BLE112 {
    public:
        BLE112(HardwareSerial *module);
        void setup();
        void loop();
        void reset();
        void hello();
        void setMode(uint8, uint8);
        void getRSSI();
        void writeAttribute();
        void readAttribute();
        void disconnect();
        void encryptStart();
        void getBonds();
        void passkeyEntry();
        void setBondableMode();
        void setOobData();
        void setParameters();
        void attributesUserReadResponse();
        void attributesUserReadResponseAuthorized(bool);
        void attributesUserWriteResponse();

    private:
        // create BGLib object:
        //  - use SoftwareSerial por for module comms
        //  - use nothing for passthrough comms (0 = null pointer)
        //  - enable packet mode on API protocol since flow control is unavailable
        BGLib bglib;

};

#endif
