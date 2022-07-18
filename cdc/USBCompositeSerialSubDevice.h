//
// Created by bakatrouble on 14/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITESERIALSUBDEVICE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITESERIALSUBDEVICE_H


#include <mbed.h>
#include "USBCompositeCDCSubDevice.h"

class USBCompositeSerialSubDevice : public USBCompositeCDCSubDevice, public Stream {
public:
    USBCompositeSerialSubDevice(USBComposite *usbComposite);

    int _putc(int c) override;

    int _getc() override;

    uint8_t available();

    bool connected();

    int readable() {
        return available() ? 1 : 0;
    }

    int writeable() {
        return 1;    // always return 1, for write operation is blocking
    }

    template<typename T>
    void attach(T *tptr, void (T::*mptr)(void)) {
        USBCompositeSerialSubDevice::lock();

        if ((mptr != NULL) && (tptr != NULL)) {
            rx = mbed::Callback<void()>(tptr, mptr);
        }

        USBCompositeSerialSubDevice::unlock();
    }

    void attach(void (*fptr)(void)) {
        USBCompositeSerialSubDevice::lock();

        if (fptr != NULL) {
            rx = mbed::Callback<void()>(fptr);
        }

        USBCompositeSerialSubDevice::unlock();
    }

    void attach(mbed::Callback<void()> &cb) {
        USBCompositeSerialSubDevice::lock();

        rx = cb;

        USBCompositeSerialSubDevice::unlock();
    }

    void attach(void (*fptr)(int baud, int bits, int parity, int stop)) {
        USBCompositeSerialSubDevice::lock();

        _settings_changed_callback = fptr;

        USBCompositeSerialSubDevice::unlock();
    }

protected:
    void data_rx() override;

    void line_coding_changed(int baud, int bits, int parity, int stop) override;

private:
    mbed::Callback<void()> rx;

    void (*_settings_changed_callback)(int baud, int bits, int parity, int stop);
};


#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITESERIALSUBDEVICE_H
