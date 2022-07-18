//
// Created by bakatrouble on 11/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITE_H


#include "USBDevice.h"
#include "USBCompositeSubDevice.h"

class USBComposite : public USBDevice {
public :
    explicit USBComposite(uint16_t vendor_id = 0xbaca,
                          uint16_t product_id = 0x0001,
                          uint16_t product_release = 0x0001);
    ~USBComposite() override;

    void bind(USBCompositeSubDevice* subdevice);
    void lock() override { USBDevice::lock(); }
    void unlock() override { USBDevice::unlock(); }
    void assert_locked() override { USBDevice::assert_locked(); }

    EndpointResolver resolver;

protected:
    void callback_reset() override;
    void callback_state_change(DeviceState new_state) override;
    void callback_request(const setup_packet_t *setup) override;
    void callback_request_xfer_done(const setup_packet_t *setup, bool aborted) override;
    void callback_set_configuration(uint8_t configuration) override;
    void callback_set_interface(uint16_t interface, uint8_t alternate) override;

    const uint8_t * configuration_desc(uint8_t index) override;
    const uint8_t * device_desc() override;
    const uint8_t * string_iinterface_desc() override;
    const uint8_t * string_iproduct_desc() override;
    const uint8_t * string_iserial_desc() override;
    const uint8_t * string_iconfiguration_desc() override;
    const uint8_t * string_imanufacturer_desc() override;

    std::vector<USBCompositeSubDevice*> _subdevices;
    uint8_t _configuration_descriptor[512];

};


#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITE_H
