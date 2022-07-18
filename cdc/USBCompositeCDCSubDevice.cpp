//
// Created by bakatrouble on 14/07/22.
//

#include "USBCompositeCDCSubDevice.h"
#include "../core/USBComposite.h"

#define CDC_SET_LINE_CODING        0x20
#define CDC_GET_LINE_CODING        0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22

#define CLS_DTR   (1 << 0)
#define CLS_RTS   (1 << 1)

class USBCompositeCDCSubDevice::AsyncWrite : public AsyncOp {
public:
    AsyncWrite(USBCompositeCDCSubDevice *serial, uint8_t *buf, uint32_t size) :
            serial(serial), tx_buf(buf), tx_size(size), result(false) {

    }

    virtual ~AsyncWrite() {

    }

    virtual bool process() {
        if (!serial->_terminal_connected) {
            result = false;
            return true;
        }

        uint32_t actual_size = 0;
        serial->send_nb(tx_buf, tx_size, &actual_size, true);
        tx_size -= actual_size;
        tx_buf += actual_size;
        if (tx_size == 0) {
            result = true;
            return true;
        }

        // Start transfer if it hasn't been
        serial->_send_isr_start();
        return false;
    }

    USBCompositeCDCSubDevice *serial;
    uint8_t *tx_buf;
    uint32_t tx_size;
    bool result;
};

class USBCompositeCDCSubDevice::AsyncRead : public AsyncOp {
public:
    AsyncRead(USBCompositeCDCSubDevice *serial, uint8_t *buf, uint32_t size, uint32_t *size_read, bool read_all)
            : serial(serial), rx_buf(buf), rx_size(size), rx_actual(size_read), all(read_all), result(false) {

    }

    virtual ~AsyncRead() {

    }

    virtual bool process() {
        if (!serial->_terminal_connected) {
            result = false;
            return true;
        }

        uint32_t actual_size = 0;
        serial->receive_nb(rx_buf, rx_size, &actual_size);
        rx_buf += actual_size;
        *rx_actual += actual_size;
        rx_size -= actual_size;
        if ((!all && *rx_actual > 0) || (rx_size == 0)) {
            // Wake thread if request is done
            result = true;
            return true;
        }

        serial->_receive_isr_start();
        return false;
    }

    USBCompositeCDCSubDevice *serial;
    uint8_t *rx_buf;
    uint32_t rx_size;
    uint32_t *rx_actual;
    bool all;
    bool result;
};

class USBCompositeCDCSubDevice::AsyncWait : public AsyncOp {
public:
    AsyncWait(USBCompositeCDCSubDevice *serial)
            : serial(serial) {

    }

    virtual ~AsyncWait() {

    }

    virtual bool process() {
        if (serial->_terminal_connected) {
            return true;
        }

        return false;
    }

    USBCompositeCDCSubDevice *serial;
};

USBCompositeCDCSubDevice::USBCompositeCDCSubDevice(USBComposite* usbComposite)
        : USBCompositeSubDevice(usbComposite) {

    _terminal_connected = false;

    _tx_in_progress = false;
    _tx_buf = _tx_buffer;
    _tx_size = 0;

    _rx_in_progress = false;
    _rx_buf = _rx_buffer;
    _rx_size = 0;
}

void USBCompositeCDCSubDevice::init() {
    _bulk_in = _usbComposite->resolver.endpoint_in(USB_EP_TYPE_BULK, CDC_MAX_PACKET_SIZE);
    _bulk_out = _usbComposite->resolver.endpoint_out(USB_EP_TYPE_BULK, CDC_MAX_PACKET_SIZE);
    _int_in = _usbComposite->resolver.endpoint_in(USB_EP_TYPE_INT, CDC_MAX_PACKET_SIZE);
}

void USBCompositeCDCSubDevice::callback_state_change(USBDevice::DeviceState new_state) {
    if (new_state != USBDevice::Configured) {
        _change_terminal_connected(false);
    }
}

bool USBCompositeCDCSubDevice::callback_request(const USBDevice::setup_packet_t *setup,
                                                USBCompositeSubDevice::complete_request_t *resp) {
    if (setup->bmRequestType.Type == CLASS_TYPE) {
        switch (setup->bRequest) {
            case CDC_GET_LINE_CODING:
                resp->result = USBDevice::Send;
                resp->data = _line_coding;
                resp->size = 7;
                return true;
            case CDC_SET_LINE_CODING:
                resp->result = USBDevice::Receive;
                resp->data = _new_line_coding;
                resp->size = 7;
                return true;
            case CDC_SET_CONTROL_LINE_STATE:
                if (setup->wValue & CLS_DTR) {
                    _change_terminal_connected(true);
                } else {
                    _change_terminal_connected(false);
                }
                resp->result = USBDevice::Success;
                return true;
        }
    }
    return false;
}

bool USBCompositeCDCSubDevice::callback_request_xfer_done(const USBDevice::setup_packet_t *setup) {
    if (setup->bmRequestType.Type == CLASS_TYPE) {
        if ((setup->bRequest == CDC_SET_LINE_CODING) && (setup->wLength == 7)) {
            if (memcmp(_line_coding, _new_line_coding, 7)) {
                memcpy(_line_coding, _new_line_coding, 7);

                const uint8_t *buf = _line_coding;
                int baud = buf[0] + (buf[1] << 8)
                           + (buf[2] << 16) + (buf[3] << 24);
                int stop = buf[4];
                int bits = buf[6];
                int parity = buf[5];

                line_coding_changed(baud, bits, parity, stop);
            }
            return true;
        }
        if (setup->bRequest == CDC_GET_LINE_CODING) {
            return true;
        }
    }
    return false;
}

void USBCompositeCDCSubDevice::callback_set_configuration() {
    _usbComposite->endpoint_add(_int_in, CDC_MAX_PACKET_SIZE, USB_EP_TYPE_INT);
    _usbComposite->endpoint_add(_bulk_in, CDC_MAX_PACKET_SIZE, USB_EP_TYPE_BULK, mbed::callback(this, &USBCompositeCDCSubDevice::_send_isr));
    _usbComposite->endpoint_add(_bulk_out, CDC_MAX_PACKET_SIZE, USB_EP_TYPE_BULK, mbed::callback(this, &USBCompositeCDCSubDevice::_receive_isr));

    _usbComposite->read_start(_bulk_out, _rx_buf, sizeof(_rx_buffer));

    _rx_in_progress = true;
}

uint8_t USBCompositeCDCSubDevice::num_interfaces() {
    return 2;
}

uint8_t USBCompositeCDCSubDevice::configuration_descriptor_length() {
    return CDC_CONFIGURATION_DESCRIPTOR_LENGTH;
}

const uint8_t *USBCompositeCDCSubDevice::configuration_descriptor(uint8_t start_if) {
    uint8_t descriptorTemp[] = {
            // IAD to associate the two CDC interfaces
            INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH,  // bLength
            INTERFACE_ASSOCIATION_DESCRIPTOR,  // bDescriptorType
            start_if,                   // bFirstInterface
            0x02,                       // bInterfaceCount
            CLASS_CDC,                  // bFunctionClass
            SUBCLASS_ACM,               // bFunctionSubClass
            PROTOCOL_V250,              // bFunctionProtocol
            0,                          // iFunction

            // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
            INTERFACE_DESCRIPTOR_LENGTH,  // bLength
            INTERFACE_DESCRIPTOR,       // bDescriptorType
            start_if,                   // bInterfaceNumber
            0,                          // bAlternateSetting
            1,                          // bNumEndpoints
            CLASS_CDC,                  // bInterfaceClass
            SUBCLASS_ACM,               // bInterfaceSubClass
            PROTOCOL_V250,              // bInterfaceProtocol
            0,                          // iInterface

            // CDC Header Functional Descriptor, CDC Spec 5.2.3.1, Table 26
            FD_CDC_LENGTH,              // bFunctionLength
            FUNCTIONAL_DESCRIPTOR,      // bDescriptorType
            FD_CDC_SUBTYPE,             // bDescriptorSubtype
            LSB(CDC_VERSION_1_10),      // bcdCDC
            MSB(CDC_VERSION_1_10),

            // Call Management Functional Descriptor, CDC Spec 5.2.3.2, Table 27
            FD_CM_LENGTH,               // bFunctionLength
            FUNCTIONAL_DESCRIPTOR,      // bDescriptorType
            FD_CM_SUBTYPE,              // bDescriptorSubtype
            0x00,                       // bmCapabilities
            uint8_t(start_if + 1),      // bDataInterface

            // Abstract Control Management Functional Descriptor, CDC Spec 5.2.3.3, Table 28
            FD_ACM_LENGTH,              // bFunctionLength
            FUNCTIONAL_DESCRIPTOR,      // bDescriptorType
            FD_ACM_SUBTYPE,             // bDescriptorSubtype
            0x02,                       // bmCapabilities

            // Union Functional Descriptor, CDC Spec 5.2.3.8, Table 33
            FD_UNION_LENGTH,            // bFunctionLength
            FUNCTIONAL_DESCRIPTOR,      // bDescriptorType
            FD_UNION_SUBTYPE,           // bDescriptorSubtype
            start_if,                   // bMasterInterface
            uint8_t(start_if + 1),      // bSlaveInterface0

            ENDPOINT_DESCRIPTOR_LENGTH, // bLength
            ENDPOINT_DESCRIPTOR,        // bDescriptorType
            _int_in,                    // bEndpointAddress
            E_INTERRUPT,                // bmAttributes
            LSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (LSB)
            MSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (MSB)
            16,                         // bInterval (milliseconds)

            // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
            INTERFACE_DESCRIPTOR_LENGTH,  // bLength
            INTERFACE_DESCRIPTOR,       // bDescriptorType
            uint8_t(start_if + 1),      // bInterfaceNumber
            0,                          // bAlternateSetting
            2,                          // bNumEndpoints
            CLASS_CDC_DATA,             // bInterfaceClass
            0x00,                       // bInterfaceSubClass
            0x00,                       // bInterfaceProtocol
            0,                          // iInterface

            // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
            ENDPOINT_DESCRIPTOR_LENGTH, // bLength
            ENDPOINT_DESCRIPTOR,        // bDescriptorType
            _bulk_in,                   // bEndpointAddress
            E_BULK,                     // bmAttributes (0x02=bulk)
            LSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (LSB)
            MSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (MSB)
            0,                          // bInterval

            // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
            ENDPOINT_DESCRIPTOR_LENGTH, // bLength
            ENDPOINT_DESCRIPTOR,        // bDescriptorType
            _bulk_out,                  // bEndpointAddress
            E_BULK,                     // bmAttributes (0x02=bulk)
            LSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (LSB)
            MSB(CDC_MAX_PACKET_SIZE),   // wMaxPacketSize (MSB)
            0,                          // bInterval
    };
    MBED_ASSERT(sizeof(descriptorTemp) == sizeof(_configuration_descriptor));
    memcpy(_configuration_descriptor, descriptorTemp, CDC_CONFIGURATION_DESCRIPTOR_LENGTH);
    return _configuration_descriptor;
}

bool USBCompositeCDCSubDevice::ready() {
    _usbComposite->lock();

    bool ready = _terminal_connected;

    _usbComposite->unlock();
    return ready;
}

void USBCompositeCDCSubDevice::wait_ready() {
    _usbComposite->lock();

    AsyncWait wait_op(this);
    _connected_list.add(&wait_op);

    _usbComposite->unlock();

    wait_op.wait(nullptr);
}

bool USBCompositeCDCSubDevice::send(uint8_t *buffer, uint32_t size) {
    _usbComposite->lock();

    AsyncWrite write_op(this, buffer, size);
    _tx_list.add(&write_op);

    _usbComposite->unlock();

    write_op.wait(nullptr);
    return write_op.result;
}

void USBCompositeCDCSubDevice::send_nb(uint8_t *buffer, uint32_t size, uint32_t *actual, bool now) {
    _usbComposite->lock();

    *actual = 0;
    if (_terminal_connected && !_tx_in_progress) {
        uint32_t free = sizeof(_tx_buffer) - _tx_size;
        uint32_t write_size = free > size ? size : free;
        if (size > 0) {
            memcpy(_tx_buf, buffer, write_size);
        }
        _tx_size += write_size;
        *actual = write_size;
        if (now) {
            _send_isr_start();
        }
    }

    _usbComposite->unlock();
}

bool USBCompositeCDCSubDevice::receive(uint8_t *buffer, uint32_t size, uint32_t *actual) {
    _usbComposite->lock();

    bool read_all = actual == nullptr;
    uint32_t size_read_dummy;
    uint32_t *size_read_ptr = read_all ? &size_read_dummy : actual;
    *size_read_ptr = 0;
    AsyncRead read_op(this, buffer, size, size_read_ptr, read_all);
    _rx_list.add(&read_op);

    _usbComposite->unlock();

    read_op.wait(nullptr);
    return read_op.result;
}

void USBCompositeCDCSubDevice::receive_nb(uint8_t *buffer, uint32_t size, uint32_t *actual) {
    *actual = 0;
    if (_terminal_connected && !_rx_in_progress) {
        // Copy data over
        uint32_t copy_size = _rx_size > size ? size : _rx_size;
        memcpy(buffer, _rx_buf, copy_size);
        *actual = copy_size;
        _rx_buf += copy_size;
        _rx_size -= copy_size;
        if (_rx_size == 0) {
            _receive_isr_start();
        }
    }
}

void USBCompositeCDCSubDevice::_change_terminal_connected(bool connected) {
    _usbComposite->assert_locked();

    _terminal_connected = connected;
    if (!_terminal_connected) {
        // Abort TX
        if (_tx_in_progress) {
            _usbComposite->endpoint_abort(_bulk_in);
            _tx_in_progress = false;
        }
        _tx_buf = _tx_buffer;
        _tx_size = 0;
        _tx_list.process();
        MBED_ASSERT(_tx_list.empty());

        // Abort RX
        if (_rx_in_progress) {
            _usbComposite->endpoint_abort(_bulk_in);
            _rx_in_progress = false;
        }
        _rx_buf = _rx_buffer;
        _rx_size = 0;
        _rx_list.process();
        MBED_ASSERT(_rx_list.empty());

//        endpoint_abort(_int_in);

    }
    _connected_list.process();
}

void USBCompositeCDCSubDevice::_send_isr_start() {
    _usbComposite->assert_locked();

    if (!_tx_in_progress && _tx_size) {
        if (_usbComposite->write_start(_bulk_in, _tx_buffer, _tx_size)) {
            _tx_in_progress = true;
        }
    }
}

void USBCompositeCDCSubDevice::_send_isr() {
    _usbComposite->assert_locked();

    _usbComposite->write_finish(_bulk_in);
    _tx_buf = _tx_buffer;
    _tx_size = 0;
    _tx_in_progress = false;

    _tx_list.process();
    if (!_tx_in_progress) {
        data_tx();
    }
}

void USBCompositeCDCSubDevice::_receive_isr_start() {
    if ((_rx_size == 0) && !_rx_in_progress) {
        // Refill the buffer
        _rx_in_progress = true;
        _usbComposite->read_start(_bulk_out, _rx_buffer, sizeof(_rx_buffer));
    }
}

void USBCompositeCDCSubDevice::_receive_isr() {
    _usbComposite->assert_locked();

    MBED_ASSERT(_rx_size == 0);
    _rx_buf = _rx_buffer;
    _rx_size = _usbComposite->read_finish(_bulk_out);
    _rx_in_progress = false;
    _rx_list.process();
    if (!_rx_in_progress) {
        data_rx();
    }
}
