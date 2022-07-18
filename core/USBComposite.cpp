//
// Created by bakatrouble on 11/07/22.
//

#include "USBComposite.h"
#include "usb_phy_api.h"
#include "USBDescriptor.h"
#include "USBPhyHw.h"


#define DEFAULT_CONFIGURATION (1)


USBComposite::USBComposite(uint16_t vendor_id, uint16_t product_id, uint16_t product_release)
    : USBDevice(get_usb_phy(), vendor_id, product_release, product_release), resolver(endpoint_table()) {
    init();
    resolver.endpoint_ctrl(64);
}

USBComposite::~USBComposite() {
    deinit();
}

void USBComposite::bind(USBCompositeSubDevice *subdevice) {
    disconnect();
    _subdevices.push_back(subdevice);
    subdevice->init();
    MBED_ASSERT(resolver.valid());
}

void USBComposite::callback_reset() {
    assert_locked();
    for (auto subdevice : _subdevices) {
        subdevice->callback_reset();
    }
}

void USBComposite::callback_state_change(USBDevice::DeviceState new_state) {
    assert_locked();
    for (auto subdevice : _subdevices) {
        subdevice->callback_state_change(new_state);
    }
}

void USBComposite::callback_request(const USBDevice::setup_packet_t *setup) {
    assert_locked();

    USBCompositeSubDevice::complete_request_t resp = {
            .result = PassThrough,
            .data = nullptr,
            .size = 0,
    };
    for (auto subdevice : _subdevices) {
        if (subdevice->callback_request(setup, &resp)) {
            break;
        }
    }

    complete_request(resp.result, resp.data, resp.size);
}

void USBComposite::callback_request_xfer_done(const USBDevice::setup_packet_t *setup, bool aborted) {
    assert_locked();

    if (aborted) {
        complete_request_xfer_done(false);
        return;
    }

    for (auto subdevice : _subdevices) {
        if (subdevice->callback_request_xfer_done(setup)) {
            complete_request_xfer_done(true);
            return;
        }
    }
    complete_request_xfer_done(false);
}

void USBComposite::callback_set_configuration(uint8_t configuration) {
    assert_locked();

    if (configuration != DEFAULT_CONFIGURATION) {
        complete_set_configuration(false);
    } else {
        endpoint_remove_all();
        for (auto subdevice : _subdevices) {
            subdevice->callback_set_configuration();
        }
        complete_set_configuration(true);
    }
}

void USBComposite::callback_set_interface(uint16_t interface, uint8_t alternate) {
    assert_locked();
    complete_set_interface(true);  // TODO?
}

const uint8_t *USBComposite::configuration_desc(uint8_t index) {
    if (index != 0)
        return nullptr;

    uint8_t num_interfaces = 0;
    uint16_t ptr = CONFIGURATION_DESCRIPTOR_LENGTH;
    for (auto subdevice : _subdevices) {
        const uint8_t *subdeviceDescriptor = subdevice->configuration_descriptor(num_interfaces);
        uint8_t subdeviceDescriptorLen = subdevice->configuration_descriptor_length();
        memcpy(_configuration_descriptor + ptr, subdeviceDescriptor, subdeviceDescriptorLen);
        num_interfaces += subdevice->num_interfaces();
        ptr += subdeviceDescriptorLen;
    }
    uint8_t descriptorHeader[] = {
            CONFIGURATION_DESCRIPTOR_LENGTH,    // bLength
            CONFIGURATION_DESCRIPTOR,           // bDescriptorType
            uint8_t(LSB(ptr)),                  // wTotalLength (LSB)
            uint8_t(MSB(ptr)),                  // wTotalLength (MSB)
            num_interfaces,                     // bNumInterfaces
            DEFAULT_CONFIGURATION,              // bConfigurationValue
            0x00,                               // iConfiguration
            C_RESERVED | C_SELF_POWERED,        // bmAttributes
            C_POWER(500),                       // bMaxPower
    };
    memcpy(_configuration_descriptor, descriptorHeader, sizeof(descriptorHeader));
    return _configuration_descriptor;
}

const uint8_t *USBComposite::device_desc() {
    uint8_t device_descriptor_temp[] = {
            DEVICE_DESCRIPTOR_LENGTH,       /* bLength */
            DEVICE_DESCRIPTOR,              /* bDescriptorType */
            LSB(USB_VERSION_2_0),           /* bcdUSB (LSB) */
            MSB(USB_VERSION_2_0),           /* bcdUSB (MSB) */
            0xEF,                           /* bDeviceClass */
            0x02,                           /* bDeviceSubClass */
            0x01,                           /* bDeviceprotocol */
            (uint8_t)MAX_PACKET_SIZE_EP0,  /* bMaxPacketSize0 */
            (uint8_t)(LSB(vendor_id)),                 /* idVendor (LSB) */
            (uint8_t)(MSB(vendor_id)),                 /* idVendor (MSB) */
            (uint8_t)(LSB(product_id)),                /* idProduct (LSB) */
            (uint8_t)(MSB(product_id)),                /* idProduct (MSB) */
            (uint8_t)(LSB(product_release)),           /* bcdDevice (LSB) */
            (uint8_t)(MSB(product_release)),           /* bcdDevice (MSB) */
            STRING_OFFSET_IMANUFACTURER,    /* iManufacturer */
            STRING_OFFSET_IPRODUCT,         /* iProduct */
            STRING_OFFSET_ISERIAL,          /* iSerialNumber */
            0x01                            /* bNumConfigurations */
    };
    MBED_ASSERT(sizeof(device_descriptor_temp) == sizeof(device_descriptor));
    memcpy(device_descriptor, device_descriptor_temp, sizeof(device_descriptor));
    return device_descriptor;
}

const uint8_t *USBComposite::string_iinterface_desc() {
    return USBDevice::string_iinterface_desc();
}

const uint8_t *USBComposite::string_iproduct_desc() {
    static const uint8_t stringIproductDescriptor[] = {
            0x2a,
            STRING_DESCRIPTOR,
            'C', 0, 'o', 0, 'm', 0, 'p', 0, 'o', 0, 's', 0, 'i', 0, 't', 0, 'e', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    };
    return stringIproductDescriptor;
}

const uint8_t *USBComposite::string_iserial_desc() {
    return USBDevice::string_iserial_desc();
}

const uint8_t *USBComposite::string_iconfiguration_desc() {
    return USBDevice::string_iconfiguration_desc();
}

const uint8_t *USBComposite::string_imanufacturer_desc() {
    static const uint8_t stringImanufacturerDescriptor[] = {
            0x18,
            STRING_DESCRIPTOR,
            'b', 0, 'a', 0, 'k', 0, 'a', 0, 't', 0, 'r', 0, 'o', 0, 'u', 0, 'b', 0, 'l', 0, 'e', 0,
    };
    return stringImanufacturerDescriptor;
}














