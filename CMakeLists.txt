add_library(USBComposite_core INTERFACE)
target_sources(USBComposite_core INTERFACE
        core/USBComposite.cpp)
target_include_directories(USBComposite_core INTERFACE
        core)

add_library(USBComposite_cdc INTERFACE)
target_sources(USBComposite_cdc INTERFACE
        cdc/USBCompositeCDCSubDevice.cpp
        cdc/USBCompositeSerialSubDevice.cpp)
target_include_directories(USBComposite_cdc INTERFACE
        cdc)
add_dependencies(USBComposite_cdc USBComposite_core)

add_library(USBComposite_hid INTERFACE)
target_sources(USBComposite_hid INTERFACE
        hid/USBCompositeHIDSubDevice.cpp
        hid/USBCompositeHIDKeyboardSubDevice.cpp)
target_include_directories(USBComposite_hid INTERFACE
        hid)
add_dependencies(USBComposite_hid USBComposite_core)

add_library(USBComposite::Core ALIAS USBComposite_core)
add_library(USBComposite::CDC ALIAS USBComposite_cdc)
add_library(USBComposite::HID ALIAS USBComposite_hid)
