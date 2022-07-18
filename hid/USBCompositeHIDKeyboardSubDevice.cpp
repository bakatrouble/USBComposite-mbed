//
// Created by bakatrouble on 14/07/22.
//

#include "USBCompositeHIDKeyboardSubDevice.h"
#include "../core/USBComposite.h"

#define NKRO_EPSIZE 16

#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_NKRO 2
#define REPORT_ID_MOUSE 3
#define REPORT_ID_CONSUMER 4
#define REPORT_ID_VENDOR 5

uint8_t *USBCompositeHIDKeyboardSubDevice::report_desc() {
    static const uint8_t reportDescriptor[] = {
            // Keyboard
            USAGE_PAGE(1),      0x01,
            USAGE(1),           0x06,
            COLLECTION(1),      0x01,
            REPORT_ID(1),       REPORT_ID_KEYBOARD,

            // modifiers
            USAGE_PAGE(1),      0x07,
            USAGE_MINIMUM(1),       0xE0,
            USAGE_MAXIMUM(1),       0xE7,
            LOGICAL_MINIMUM(1),     0x00,
            LOGICAL_MAXIMUM(1),     0x01,
            REPORT_SIZE(1),     0x01,
            REPORT_COUNT(1),    0x08,
            INPUT(1),           0x02,
            REPORT_COUNT(1),    0x01,
            REPORT_SIZE(1),     0x08,
            INPUT(1),           0x01,

            // leds
            REPORT_COUNT(1),    0x05,
            REPORT_SIZE(1),     0x01,
            USAGE_PAGE(1),      0x08,
            USAGE_MINIMUM(1),       0x01,
            USAGE_MAXIMUM(1),       0x05,
            OUTPUT(1),          0x02,
            REPORT_COUNT(1),    0x01,
            REPORT_SIZE(1),     0x03,
            OUTPUT(1),          0x01,

            // keycodes
            REPORT_COUNT(1),    0x06,
            REPORT_SIZE(1),     0x08,
            LOGICAL_MINIMUM(1),     0x00,
            LOGICAL_MAXIMUM(2),     0xff, 0x00,
            USAGE_PAGE(1),      0x07,
            USAGE_MINIMUM(1),       0x00,
            USAGE_MAXIMUM(2),       0xff, 0x00,
            INPUT(1),           0x00,
            END_COLLECTION(0),


            // NKRO bitmap
            USAGE_PAGE(1), 0x01,                        // Generic Desktop
            USAGE(1), 0x06,                             // Keyboard
            COLLECTION(1), 0x01,                        // Application
            REPORT_ID(1),       REPORT_ID_NKRO,

            // modifiers
            USAGE_PAGE(1), 0x07,
            USAGE_MINIMUM(1), 0xE0,
            USAGE_MAXIMUM(1), 0xE7,
            LOGICAL_MINIMUM(1), 0x00,
            LOGICAL_MAXIMUM(1), 0x01,
            REPORT_SIZE(1), 0x01,
            REPORT_COUNT(1), 0x08,
            INPUT(1), 0x02,                         // Data, Variable, Absolute
            REPORT_COUNT(1), 0x01,
            REPORT_SIZE(1), 0x08,
            INPUT(1), 0x01,                         // Constant

            // leds
            REPORT_COUNT(1), 0x05,
            REPORT_SIZE(1), 0x01,
            USAGE_PAGE(1), 0x08,                    // LEDs
            USAGE_MINIMUM(1), 0x01,
            USAGE_MAXIMUM(1), 0x05,
            OUTPUT(1), 0x02,                        // Data, Variable, Absolute
            REPORT_COUNT(1), 0x01,
            REPORT_SIZE(1), 0x03,
            OUTPUT(1), 0x03,                        // Constant

            // keycodes
            USAGE_PAGE(1), 0x07,                    // Key Codes  0x07
            REPORT_COUNT(1), (NKRO_EPSIZE-1)*8,
            REPORT_SIZE(1), 0x01,
            LOGICAL_MINIMUM(1), 0x00,
            LOGICAL_MAXIMUM(1), 0x01,
            USAGE_MINIMUM(1), 0x00,
            USAGE_MAXIMUM(1), (NKRO_EPSIZE-1)*8-1,
            INPUT(1), 0x02,
            END_COLLECTION(0),


            // Mouse
            USAGE_PAGE(1),      0x01,           // Generic Desktop
            USAGE(1),           0x02,           // Mouse
            COLLECTION(1),      0x01,           // Application
            USAGE(1),           0x01,           // Pointer
            COLLECTION(1),      0x00,           // Physical
            REPORT_ID(1),       REPORT_ID_MOUSE,
            REPORT_COUNT(1),    0x03,
            REPORT_SIZE(1),     0x01,
            USAGE_PAGE(1),      0x09,           // Buttons
            USAGE_MINIMUM(1),       0x1,
            USAGE_MAXIMUM(1),       0x3,
            LOGICAL_MINIMUM(1),     0x00,
            LOGICAL_MAXIMUM(1),     0x01,
            INPUT(1),           0x02,  // 158
            REPORT_COUNT(1),    0x01,
            REPORT_SIZE(1),     0x05,
            INPUT(1),           0x01,  // 164
            REPORT_COUNT(1),    0x03,
            REPORT_SIZE(1),     0x08,
            USAGE_PAGE(1),      0x01,
            USAGE(1),           0x30,           // X
            USAGE(1),           0x31,           // Y
            USAGE(1),           0x38,           // scroll
            LOGICAL_MINIMUM(1),     0x81,
            LOGICAL_MAXIMUM(1),     0x7f,
            INPUT(1),           0x06,  // 182
            END_COLLECTION(0),
            END_COLLECTION(0),


            // Media Control
            USAGE_PAGE(1), 0x0C,
            USAGE(1), 0x01,
            COLLECTION(1), 0x01,
            REPORT_ID(1), REPORT_ID_CONSUMER,
            USAGE_PAGE(1), 0x0C,
            LOGICAL_MINIMUM(1), 0x00,
            LOGICAL_MAXIMUM(1), 0x01,
            REPORT_SIZE(1), 0x01,
            REPORT_COUNT(1), 0x07,
            USAGE(1), 0xB5,             // Next Track
            USAGE(1), 0xB6,             // Previous Track
            USAGE(1), 0xB7,             // Stop
            USAGE(1), 0xCD,             // Play / Pause
            USAGE(1), 0xE2,             // Mute
            USAGE(1), 0xE9,             // Volume Up
            USAGE(1), 0xEA,             // Volume Down
            INPUT(1), 0x02,             // Input (Data, Variable, Absolute)
            REPORT_COUNT(1), 0x01,
            INPUT(1), 0x01,
            END_COLLECTION(0),


            // Vendor
            USAGE_PAGE(2), LSB(0xFF1C), MSB(0xFF1C),        // Vendor Specific?
            USAGE(1), 0x92,                                 // Vendor Usage 1
            COLLECTION(1), 0x01,                            // Application
            REPORT_ID(1), REPORT_ID_VENDOR,

            USAGE_MINIMUM(1), 0x00,
            USAGE_MAXIMUM(2), 0xFF, 0x00,
            LOGICAL_MINIMUM(1), 0x00,
            LOGICAL_MAXIMUM(2), 0xFF, 0x00,
            REPORT_SIZE(1), 0x08,
            REPORT_COUNT(1), 0x3F,
            OUTPUT(1), 0x00,

            USAGE_MINIMUM(1), 0x00,
            USAGE_MAXIMUM(1), 0xFF,
            INPUT(1), 0x00,
            END_COLLECTION(0),
    };
    reportLength = sizeof(reportDescriptor);
    return (uint8_t*)reportDescriptor;
}

bool USBCompositeHIDKeyboardSubDevice::move(int16_t x, int16_t y) {
    _mutex.lock();

    bool ret = update(x, y, _button, 0);

    _mutex.unlock();
    return ret;
}

bool USBCompositeHIDKeyboardSubDevice::update(int16_t x, int16_t y, uint8_t button, int8_t z) {
    bool ret;
    _mutex.lock();

    while (x > 127) {
        if (!_mouse_send(127, 0, button, z)) {
            _mutex.unlock();
            return false;
        }
        x = x - 127;
    }
    while (x < -128) {
        if (!_mouse_send(-128, 0, button, z)) {
            _mutex.unlock();
            return false;
        }
        x = x + 128;
    }
    while (y > 127) {
        if (!_mouse_send(0, 127, button, z)) {
            _mutex.unlock();
            return false;
        }
        y = y - 127;
    }
    while (y < -128) {
        if (!_mouse_send(0, -128, button, z)) {
            _mutex.unlock();
            return false;
        }
        y = y + 128;
    }
    ret = _mouse_send(x, y, button, z);

    _mutex.unlock();
    return ret;
}

bool USBCompositeHIDKeyboardSubDevice::_mouse_send(int8_t x, int8_t y, uint8_t buttons, int8_t z) {
    _mutex.lock();

    HID_REPORT report;
    report.data[0] = REPORT_ID_MOUSE;
    report.data[1] = buttons & 0x07;
    report.data[2] = x;
    report.data[3] = y;
    report.data[4] = -z; // >0 to scroll down, <0 to scroll up

    report.length = 5;

    bool ret = send(&report);

    _mutex.unlock();
    return ret;
}

void USBCompositeHIDKeyboardSubDevice::report_rx() {
    _usbComposite->assert_locked();

    HID_REPORT report;
    read_nb(&report);

    // we take [1] because [0] is the report ID
    _lock_status = report.data[1] & 0x07;
}

USBCompositeHIDKeyboardSubDevice::USBCompositeHIDKeyboardSubDevice(USBComposite *usbComposite)
        : USBCompositeHIDSubDevice(usbComposite, 0, 0) {

}


