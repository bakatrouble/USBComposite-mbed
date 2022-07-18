//
// Created by bakatrouble on 14/07/22.
//

#ifndef WIRELESS_DONGLE_MBED_USBCOMPOSITECDCSUBDEVICE_H
#define WIRELESS_DONGLE_MBED_USBCOMPOSITECDCSUBDEVICE_H


#include "../core/USBCompositeSubDevice.h"
#include "OperationList.h"
#include "USBDescriptor.h"

#define CDC_MAX_PACKET_SIZE   64

#define INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH 0x08
#define INTERFACE_ASSOCIATION_DESCRIPTOR 0x0B
#define CLASS_CDC 0x02
#define SUBCLASS_ACM 0x02
#define PROTOCOL_V250 0x01

#define FUNCTIONAL_DESCRIPTOR 0x24
#define FD_CDC_SUBTYPE 0x00
#define FD_CM_SUBTYPE 0x01
#define FD_ACM_SUBTYPE 0x02
#define FD_UNION_SUBTYPE 0x06
#define FD_CDC_LENGTH 0x05
#define FD_CM_LENGTH 0x05
#define FD_ACM_LENGTH 0x04
#define FD_UNION_LENGTH 0x05

#define CDC_VERSION_1_10 (0x0110)

#define CLASS_CDC_DATA 0x0A

#define CDC_CONFIGURATION_DESCRIPTOR_LENGTH (INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH + \
                                             INTERFACE_DESCRIPTOR_LENGTH + FD_CDC_LENGTH + \
                                             FD_CM_LENGTH + FD_ACM_LENGTH + FD_UNION_LENGTH + \
                                             ENDPOINT_DESCRIPTOR_LENGTH + INTERFACE_DESCRIPTOR_LENGTH + \
                                             ENDPOINT_DESCRIPTOR_LENGTH + ENDPOINT_DESCRIPTOR_LENGTH)

class USBCompositeCDCSubDevice : public USBCompositeSubDevice {
public:
    USBCompositeCDCSubDevice(USBComposite* usbComposite);

    void init() override;
    void callback_state_change(USBDevice::DeviceState new_state) override;
    bool callback_request(const USBDevice::setup_packet_t *setup, complete_request_t *resp) override;
    bool callback_request_xfer_done(const USBDevice::setup_packet_t *setup) override;
    void callback_set_configuration() override;
    uint8_t num_interfaces() override;
    uint8_t configuration_descriptor_length() override;
    const uint8_t * configuration_descriptor(uint8_t start_if) override;

    bool ready();
    void wait_ready();
    bool send(uint8_t *buffer, uint32_t size);
    void send_nb(uint8_t *buffer, uint32_t size, uint32_t *actual, bool now = true);
    bool receive(uint8_t *buffer, uint32_t size, uint32_t *actual = NULL);
    void receive_nb(uint8_t *buffer, uint32_t size, uint32_t *actual);

protected:
    virtual void line_coding_changed(int baud, int bits, int parity, int stop) {};
    virtual void data_rx() {}
    virtual void data_tx() {}

    uint8_t _configuration_descriptor[CDC_CONFIGURATION_DESCRIPTOR_LENGTH] = {};

protected:
    class AsyncWrite;
    class AsyncRead;
    class AsyncWait;

    void _change_terminal_connected(bool connected);

    void _send_isr_start();
    void _send_isr();

    void _receive_isr_start();
    void _receive_isr();

    usb_ep_t _bulk_in;
    usb_ep_t _bulk_out;
    usb_ep_t _int_in;

    uint8_t _line_coding[7];
    uint8_t _new_line_coding[7];

    OperationList<AsyncWait> _connected_list;
    bool _terminal_connected;

    OperationList<AsyncWrite> _tx_list;
    bool _tx_in_progress;
    uint8_t _tx_buffer[CDC_MAX_PACKET_SIZE];
    uint8_t *_tx_buf;
    uint32_t _tx_size;

    OperationList<AsyncRead> _rx_list;
    bool _rx_in_progress;
    uint8_t _rx_buffer[CDC_MAX_PACKET_SIZE];
    uint8_t *_rx_buf;
    uint32_t _rx_size;
};


#endif //WIRELESS_DONGLE_MBED_USBCOMPOSITECDCSUBDEVICE_H
