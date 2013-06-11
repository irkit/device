#ifndef __BLE112_H__
#define __BLE112_H__

#include "BGLib.h"

class BLE112 {
    public:
        BLE112(HardwareSerial *module);
        void setup();
        void loop();
        void reset();
        void hello();
        void setMode();
        void getRSSI();
        void writeAttribute();
        void readAttribute();
        void encryptStart();
        void getBonds();
        void passkeyEntry();
        void setBondableMode();
        void setOobData();
        void setParameters();

    private:
        // create BGLib object:
        //  - use SoftwareSerial por for module comms
        //  - use nothing for passthrough comms (0 = null pointer)
        //  - enable packet mode on API protocol since flow control is unavailable
        BGLib bglib;

};

#endif
