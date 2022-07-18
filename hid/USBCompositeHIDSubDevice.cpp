//
// Created by bakatrouble on 13/07/22.
//

#include "USBCompositeHIDSubDevice.h"
#include "EndpointResolver.h"
#include "USBHID_Types.h"
#include "../core/USBComposite.h"


class USBCompositeHIDSubDevice::AsyncSend : public AsyncOp {
public:
    AsyncSend(USBCompositeHIDSubDevice *hid, const HID_REPORT *report) : hid(hid), report(report), result(false) {

    }

    virtual ~AsyncSend() {

    }

    virtual bool process() {
        if (!hid->configured()) {
            result = false;
            return true;
        }

        if (hid->send_nb(report)) {
            result = true;
            return true;
        }

        return false;
    }

    USBCompositeHIDSubDevice *hid;
    const HID_REPORT *report;
    bool result;
};

class USBCompositeHIDSubDevice::AsyncRead : public AsyncOp {
public:
    AsyncRead(USBCompositeHIDSubDevice *hid, HID_REPORT *report) : hid(hid), report(report), result(false) {

    }

    virtual ~AsyncRead() {

    }

    virtual bool process() {
        if (!hid->configured()) {
            result = false;
            return true;
        }

        if (hid->read_nb(report)) {
            result = true;
            return true;
        }

        return false;
    }

    USBCompositeHIDSubDevice *hid;
    HID_REPORT *report;
    bool result;
};

class USBCompositeHIDSubDevice::AsyncWait : public AsyncOp {
public:
    AsyncWait(USBCompositeHIDSubDevice *hid) : hid(hid) {

    }

    virtual ~AsyncWait() {

    }

    virtual bool process() {
        if (hid->configured()) {
            return true;
        }

        return false;
    }

    USBCompositeHIDSubDevice *hid;
};

USBCompositeHIDSubDevice::USBCompositeHIDSubDevice(USBComposite* usbComposite, uint8_t output_report_length, uint8_t input_report_length)
        : USBCompositeSubDevice(usbComposite){
    _send_idle = true;
    _read_idle = true;
    _output_length = output_report_length;
    _input_length = input_report_length;
    reportLength = 0;
    _input_report.length = 0;
    _output_report.length = 0;
}

void USBCompositeHIDSubDevice::init() {
    _int_in = _usbComposite->resolver.endpoint_in(USB_EP_TYPE_INT, MAX_HID_REPORT_SIZE);
    _int_out = _usbComposite->resolver.endpoint_out(USB_EP_TYPE_INT, MAX_HID_REPORT_SIZE);
}

uint8_t USBCompositeHIDSubDevice::num_interfaces() {
    return 1;
}

uint8_t USBCompositeHIDSubDevice::configuration_descriptor_length() {
    return HID_CONFIGURATION_DESCRIPTOR_LENGTH;
}

const uint8_t *USBCompositeHIDSubDevice::configuration_descriptor(uint8_t start_if) {
    report_desc();
    uint8_t descriptorTemp[] = {
            INTERFACE_DESCRIPTOR_LENGTH,        // bLength
            INTERFACE_DESCRIPTOR,               // bDescriptorType
            start_if,                           // bInterfaceNumber
            0x00,                               // bAlternateSetting
            0x02,                               // bNumEndpoints
            HID_CLASS,                          // bInterfaceClass
            HID_SUBCLASS_BOOT,                  // bInterfaceSubClass
            HID_PROTOCOL_KEYBOARD,              // bInterfaceProtocol
            0x00,                               // iInterface

            HID_DESCRIPTOR_LENGTH,              // bLength
            HID_DESCRIPTOR,                     // bDescriptorType
            LSB(HID_VERSION_1_11),              // bcdHID (LSB)
            MSB(HID_VERSION_1_11),              // bcdHID (MSB)
            0x00,                               // bCountryCode
            0x01,                               // bNumDescriptors
            REPORT_DESCRIPTOR,                  // bDescriptorType
            (uint8_t) (LSB(report_desc_length())),   // wDescriptorLength (LSB)
            (uint8_t) (MSB(report_desc_length())),   // wDescriptorLength (MSB)

            ENDPOINT_DESCRIPTOR_LENGTH,         // bLength
            ENDPOINT_DESCRIPTOR,                // bDescriptorType
            _int_in,                            // bEndpointAddress
            E_INTERRUPT,                        // bmAttributes
            LSB(MAX_HID_REPORT_SIZE),           // wMaxPacketSize (LSB)
            MSB(MAX_HID_REPORT_SIZE),           // wMaxPacketSize (MSB)
            1,                                  // bInterval (milliseconds)

            ENDPOINT_DESCRIPTOR_LENGTH,         // bLength
            ENDPOINT_DESCRIPTOR,                // bDescriptorType
            _int_out,                           // bEndpointAddress
            E_INTERRUPT,                        // bmAttributes
            LSB(MAX_HID_REPORT_SIZE),           // wMaxPacketSize (LSB)
            MSB(MAX_HID_REPORT_SIZE),           // wMaxPacketSize (MSB)
            1,                                  // bInterval (milliseconds)
    };
    MBED_ASSERT(sizeof(descriptorTemp) == sizeof(_configuration_descriptor));
    memcpy(_configuration_descriptor, descriptorTemp, HID_CONFIGURATION_DESCRIPTOR_LENGTH);
    return _configuration_descriptor;
}

void USBCompositeHIDSubDevice::callback_state_change(USBDevice::DeviceState new_state) {
    if (new_state != USBDevice::Configured) {
        if (!_send_idle) {
            _usbComposite->endpoint_abort(_int_in);
            _send_idle = true;
        }
        if (!_read_idle) {
            _usbComposite->endpoint_abort(_int_out);
            _read_idle = true;
        }
    }
    _send_list.process();
    _read_list.process();
    _connect_list.process();
}

bool USBCompositeHIDSubDevice::callback_request(const USBDevice::setup_packet_t *setup,
                                                USBCompositeSubDevice::complete_request_t *resp) {
    if (setup->bmRequestType.Type == STANDARD_TYPE) {
        if (setup->bRequest == GET_DESCRIPTOR) {
            switch (DESCRIPTOR_TYPE(setup->wValue)) {
                case REPORT_DESCRIPTOR:
                    if ((report_desc() != nullptr) && (report_desc_length() != 0)) {
                        resp->size = report_desc_length();
                        resp->data = (uint8_t *) report_desc();
                        resp->result = USBDevice::Send;
                    }
                    return true;
                case HID_DESCRIPTOR:
                    resp->size = HID_DESCRIPTOR_LENGTH;
                    resp->data = &_configuration_descriptor[INTERFACE_DESCRIPTOR_LENGTH];
                    resp->result = USBDevice::Send;
                    return true;
            }
        }
    }

    if (setup->bmRequestType.Type == CLASS_TYPE) {
        switch (setup->bRequest) {
            case SET_REPORT:
                // First byte will be used for report ID
                _output_report.data[0] = setup->wValue & 0xff;
                _output_report.length = setup->wLength + 1;

                resp->size = sizeof(_output_report.data) - 1;
                resp->data = &_output_report.data[1];
                resp->result = USBDevice::Send;
                return true;
            case HID_GET_PROTOCOL:
                resp->result = USBDevice::Send;
                resp->data = (uint8_t*)&(_bootProtocol ? BOOT_PROTOCOL : REPORT_PROTOCOL);
                resp->size = 1;
                return true;
            case HID_SET_PROTOCOL:
                _bootProtocol = setup->wValue == 0x00;
                resp->result = USBDevice::Send;
                resp->size = 0;
                return true;
        }
    }

    return false;
}

bool USBCompositeHIDSubDevice::callback_request_xfer_done(const USBDevice::setup_packet_t *setup) {
    if (setup->bmRequestType.Type == STANDARD_TYPE) {
        if (setup->bRequest == GET_DESCRIPTOR) {
            uint8_t descriptorType = DESCRIPTOR_TYPE(setup->wValue);
            if (descriptorType == REPORT_DESCRIPTOR || descriptorType == HID_DESCRIPTOR)
                return true;
        }
    }
    if (setup->bmRequestType.Type == CLASS_TYPE) {
        if (setup->bRequest == SET_REPORT || setup->bRequest == HID_GET_PROTOCOL || setup->bRequest == HID_SET_PROTOCOL)
            return true;
    }
    return false;
}

void USBCompositeHIDSubDevice::callback_set_configuration() {
    _usbComposite->endpoint_add(_int_in, MAX_HID_REPORT_SIZE, USB_EP_TYPE_INT, mbed::callback(this, &USBCompositeHIDSubDevice::_send_isr));
    _usbComposite->endpoint_add(_int_out, MAX_HID_REPORT_SIZE, USB_EP_TYPE_INT, mbed::callback(this, &USBCompositeHIDSubDevice::_read_isr));

    _usbComposite->read_start(_int_out, _output_report.data, MAX_HID_REPORT_SIZE);

    _read_idle = false;
}

bool USBCompositeHIDSubDevice::configured() {
    return _usbComposite->configured();
}

uint8_t *USBCompositeHIDSubDevice::report_desc() {
    uint8_t reportDescriptorTemp[] = {
            USAGE_PAGE(2), LSB(0xFFAB), MSB(0xFFAB),
            USAGE(2), LSB(0x0200), MSB(0x0200),
            COLLECTION(1), 0x01, // Collection (Application)

            REPORT_SIZE(1), 0x08, // 8 bits
            LOGICAL_MINIMUM(1), 0x00,
            LOGICAL_MAXIMUM(1), 0xFF,

            REPORT_COUNT(1), _input_length,
            USAGE(1), 0x01,
            INPUT(1), 0x02, // Data, Var, Abs

            REPORT_COUNT(1), _output_length,
            USAGE(1), 0x02,
            OUTPUT(1), 0x02, // Data, Var, Abs

            END_COLLECTION(0),
    };
    reportLength = sizeof(reportDescriptor);
    MBED_ASSERT(sizeof(reportDescriptorTemp) == sizeof(reportDescriptor));
    memcpy(reportDescriptor, reportDescriptorTemp, sizeof(reportDescriptor));
    return reportDescriptor;
}

uint16_t USBCompositeHIDSubDevice::report_desc_length() {
    return reportLength;
}

bool USBCompositeHIDSubDevice::ready() {
    return configured();
}

void USBCompositeHIDSubDevice::wait_ready() {
    _usbComposite->lock();

    AsyncWait wait_op(this);
    _connect_list.add(&wait_op);

    _usbComposite->unlock();

    wait_op.wait(NULL);
}

bool USBCompositeHIDSubDevice::send(const HID_REPORT *report) {
    _usbComposite->lock();

    AsyncSend send_op(this, report);
    _send_list.add(&send_op);

    _usbComposite->unlock();

    send_op.wait(NULL);
    return send_op.result;
}

bool USBCompositeHIDSubDevice::send_nb(const HID_REPORT *report) {
    _usbComposite->lock();

    if (!configured()) {
        _usbComposite->unlock();
        return false;
    }

    bool success = false;
    if (_send_idle) {
        memcpy(&_input_report, report, sizeof(_input_report));
        _usbComposite->write_start(_int_in, _input_report.data, _input_report.length);
        _send_idle = false;
        success = true;
    }

    _usbComposite->unlock();
    return success;
}

bool USBCompositeHIDSubDevice::read(HID_REPORT *report) {
    _usbComposite->lock();

    AsyncRead read_op(this, report);
    _read_list.add(&read_op);

    _usbComposite->unlock();

    read_op.wait(NULL);
    return read_op.result;
}

bool USBCompositeHIDSubDevice::read_nb(HID_REPORT *report) {
    _usbComposite->lock();

    if (!configured()) {
        _usbComposite->unlock();
        return false;
    }

    bool success = false;
    if (_read_idle) {
        memcpy(report, &_output_report, sizeof(_output_report));
        _usbComposite->read_start(_int_out, _output_report.data, MAX_HID_REPORT_SIZE);
        _read_idle = false;
        success = true;
    }

    _usbComposite->unlock();
    return success;
}

void USBCompositeHIDSubDevice::_send_isr() {
    _usbComposite->assert_locked();

    _usbComposite->write_finish(_int_in);
    _send_idle = true;

    _send_list.process();
    if (_send_idle) {
        report_tx();
    }
}

void USBCompositeHIDSubDevice::_read_isr() {
    _usbComposite->assert_locked();

    _output_report.length = _usbComposite->read_finish(_int_out);
    _read_idle = true;

    _read_list.process();
    if (_read_idle) {
        report_rx();
    }
}
