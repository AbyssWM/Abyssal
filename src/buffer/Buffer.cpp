#include <abyssal/buffer/Buffer.hpp>
#include "Shared.hpp"

using namespace Abyssal;

SDMABUFAttrs Abyssal::IBuffer::dmabuf() {
    return SDMABUFAttrs{};
}

SSHMAttrs Abyssal::IBuffer::shm() {
    return SSHMAttrs{};
}

std::tuple<uint8_t*, uint32_t, size_t> Abyssal::IBuffer::beginDataPtr(uint32_t flags) {
    return {nullptr, 0, 0};
}

void Abyssal::IBuffer::endDataPtr() {
    ; // empty
}

void Abyssal::IBuffer::sendRelease() {
    ;
}

void Abyssal::IBuffer::lock() {
    locks++;
}

void Abyssal::IBuffer::unlock() {
    locks--;

    ASSERT(locks >= 0);

    if (locks <= 0)
        sendRelease();
}

bool Abyssal::IBuffer::locked() {
    return locks;
}

void Abyssal::IBuffer::backendPin() {
    backendPins++;
    lockedByBackend = true;
}

void Abyssal::IBuffer::backendUnpin() {
    ASSERT(backendPins > 0);

    if (backendPins == 0)
        return;

    backendPins--;
    if (backendPins > 0)
        return;

    lockedByBackend = false;
    events.backendRelease.emit();
}

uint32_t Abyssal::IBuffer::backendPinCount() const {
    return backendPins;
}
