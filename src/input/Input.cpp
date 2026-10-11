#include <abyssal/input/Input.hpp>

libinput_device* Abyssal::IPointer::getLibinputHandle() {
    return nullptr;
}

libinput_device* Abyssal::IKeyboard::getLibinputHandle() {
    return nullptr;
}

libinput_device* Abyssal::ITouch::getLibinputHandle() {
    return nullptr;
}

libinput_device* Abyssal::ISwitch::getLibinputHandle() {
    return nullptr;
}

libinput_device* Abyssal::ITabletTool::getLibinputHandle() {
    return nullptr;
}

libinput_tablet_tool* Abyssal::ITabletTool::getLibinputTool() {
    return nullptr;
}

libinput_device* Abyssal::ITablet::getLibinputHandle() {
    return nullptr;
}

libinput_device* Abyssal::ITabletPad::getLibinputHandle() {
    return nullptr;
}

void Abyssal::IKeyboard::updateLEDs(uint32_t leds) {
    ;
}
