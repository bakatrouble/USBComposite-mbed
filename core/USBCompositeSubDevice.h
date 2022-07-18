//
// Created by bakatrouble on 13/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITESUBDEVICE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITESUBDEVICE_H

#include "drivers/usb/include/usb/internal/USBDevice.h"
#include "EndpointResolver.h"

class USBComposite;

class USBCompositeSubDevice {
public:
    struct complete_request_t {
        USBDevice::RequestResult result;
        uint8_t *data;
        uint32_t size;
    };

    USBCompositeSubDevice(USBComposite* usbComposite) {
        _usbComposite = usbComposite;
    }

    virtual void init() {};
    virtual void callback_reset() {};
    virtual void callback_state_change(USBDevice::DeviceState new_state) {};
    virtual bool callback_request(const USBDevice::setup_packet_t *setup, complete_request_t *resp) { return false; };
    virtual bool callback_request_xfer_done(const USBDevice::setup_packet_t *setup) { return false; };
    virtual void callback_set_configuration() {};

    virtual uint8_t num_interfaces() { return 0; };
    virtual uint8_t configuration_descriptor_length() { return 0; };
    virtual const uint8_t* configuration_descriptor(uint8_t start_if) { return nullptr; };

protected:
    USBComposite* _usbComposite;
};

#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITESUBDEVICE_H
