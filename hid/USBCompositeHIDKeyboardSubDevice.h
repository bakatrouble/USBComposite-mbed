//
// Created by bakatrouble on 14/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDKEYBOARDSUBDEVICE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDKEYBOARDSUBDEVICE_H

#include "USBCompositeHIDSubDevice.h"
#include "PlatformMutex.h"

class USBCompositeHIDKeyboardSubDevice : public USBCompositeHIDSubDevice {
public:
    USBCompositeHIDKeyboardSubDevice(USBComposite* usbComposite);

    uint8_t* report_desc() override;
    void report_rx() override;

    bool update(int16_t x, int16_t y, uint8_t button, int8_t z);
    bool move(int16_t x, int16_t y);
    uint8_t lockStatus() {
        return _lock_status;
    }

protected:
    PlatformMutex _mutex;

    bool _mouse_send(int8_t x, int8_t y, uint8_t buttons, int8_t z);

    uint8_t _button = 0;
    uint8_t _lock_status = 0;
    uint8_t reportDescriptor[];
};

#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDKEYBOARDSUBDEVICE_H
