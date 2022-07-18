//
// Created by bakatrouble on 13/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDSUBDEVICE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDSUBDEVICE_H


#include "../core/USBCompositeSubDevice.h"
#include "OperationList.h"
#include "USBHID_Types.h"
#include "USBDescriptor.h"

#define HID_CONFIGURATION_DESCRIPTOR_LENGTH (INTERFACE_DESCRIPTOR_LENGTH + HID_DESCRIPTOR_LENGTH + (2 * ENDPOINT_DESCRIPTOR_LENGTH))
#define HID_GET_PROTOCOL 0x3
#define HID_SET_PROTOCOL 0xb

const uint8_t BOOT_PROTOCOL = 0x00;
const uint8_t REPORT_PROTOCOL = 0x01;

class USBCompositeHIDSubDevice : public USBCompositeSubDevice {
public:
    explicit USBCompositeHIDSubDevice(USBComposite* usbComposite, uint8_t output_report_length = 64, uint8_t input_report_length = 64);

    void init() override;
    void callback_state_change(USBDevice::DeviceState new_state) override;
    bool callback_request(const USBDevice::setup_packet_t *setup, complete_request_t *resp) override;
    bool callback_request_xfer_done(const USBDevice::setup_packet_t *setup) override;
    void callback_set_configuration() override;
    uint8_t num_interfaces() override;
    uint8_t configuration_descriptor_length() override;
    const uint8_t * configuration_descriptor(uint8_t start_if) override;

    bool configured();
    bool ready();
    void wait_ready();
    bool send(const HID_REPORT *report);
    bool send_nb(const HID_REPORT *report);
    bool read(HID_REPORT *report);
    bool read_nb(HID_REPORT *report);

protected:
    virtual uint8_t* report_desc();
    uint16_t report_desc_length();

    virtual void report_tx() {};
    virtual void report_rx() {};

    uint16_t reportLength;
    uint8_t reportDescriptor[27];
    usb_ep_t _int_in;
    usb_ep_t _int_out;

    uint8_t _configuration_descriptor[HID_CONFIGURATION_DESCRIPTOR_LENGTH] = {};

protected:
    void _send_isr();
    void _read_isr();

    class AsyncSend;
    class AsyncRead;
    class AsyncWait;

    OperationList<AsyncWait> _connect_list;
    OperationList<AsyncSend> _send_list;
    bool _send_idle;
    OperationList<AsyncRead> _read_list;
    bool _read_idle;

    HID_REPORT _input_report;
    HID_REPORT _output_report;
    uint8_t _output_length;
    uint8_t _input_length;

    bool _bootProtocol;
};


#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITEHIDSUBDEVICE_H
