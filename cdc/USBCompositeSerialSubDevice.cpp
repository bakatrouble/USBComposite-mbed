//
// Created by bakatrouble on 14/07/22.
//

#include "USBCompositeSerialSubDevice.h"
#include "../core/USBComposite.h"

USBCompositeSerialSubDevice::USBCompositeSerialSubDevice(USBComposite *usbComposite)
        : USBCompositeCDCSubDevice(usbComposite) {
    _settings_changed_callback = nullptr;
}

int USBCompositeSerialSubDevice::_putc(int c) {
    if (send((uint8_t *)&c, 1)) {
        return c;
    } else {
        return -1;
    }
}

int USBCompositeSerialSubDevice::_getc() {
    uint8_t c = 0;
    if (receive(&c, sizeof(c))) {
        return c;
    } else {
        return -1;
    }
}

uint8_t USBCompositeSerialSubDevice::available() {
    USBCompositeSerialSubDevice::lock();

    uint8_t size = 0;
    if (!_rx_in_progress) {
        size = _rx_size > 0xFF ? 0xFF : _rx_size;
    }

    USBCompositeSerialSubDevice::unlock();
    return size;
}

bool USBCompositeSerialSubDevice::connected() {
    return _terminal_connected;
}

void USBCompositeSerialSubDevice::data_rx() {
    USBCompositeCDCSubDevice::data_rx();
}

void USBCompositeSerialSubDevice::line_coding_changed(int baud, int bits, int parity, int stop) {
    _usbComposite->assert_locked();

    if (_settings_changed_callback) {
        _settings_changed_callback(baud, bits, parity, stop);
    }
}
