#include <abyssal/backend/Null.hpp>
#include <fcntl.h>
#include <ctime>
#include <sys/timerfd.h>
#include <cstring>
#include "Shared.hpp"

using namespace Abyssal;
using namespace Abyssutils::Memory;
using namespace Abyssutils::Math;
#define SP CSharedPointer

Abyssal::CNullBackend::~CNullBackend() {
    ;
}

Abyssal::CNullBackend::CNullBackend(SP<CBackend> backend_) : backend(backend_) {
    ;
}

eBackendType Abyssal::CNullBackend::type() {
    return eBackendType::ABYSSAL_BACKEND_NULL;
}

bool Abyssal::CNullBackend::start() {
    return true;
}

std::vector<SP<SPollFD>> Abyssal::CNullBackend::pollFDs() {
    return {};
}

int Abyssal::CNullBackend::drmFD() {
    return -1;
}

int Abyssal::CNullBackend::drmRenderNodeFD() {
    return -1;
}

bool Abyssal::CNullBackend::dispatchEvents() {
    return true;
}

uint32_t Abyssal::CNullBackend::capabilities() {
    return 0;
}

bool Abyssal::CNullBackend::setCursor(SP<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot) {
    return false;
}

void Abyssal::CNullBackend::onReady() {
    ;
}

std::vector<SDRMFormat> Abyssal::CNullBackend::getRenderFormats() {
    for (const auto& impl : backend->getImplementations()) {
        if (impl->type() != ABYSSAL_BACKEND_DRM || impl->getRenderableFormats().empty())
            continue;
        return impl->getRenderableFormats();
    }

    return m_formats;
}

void Abyssal::CNullBackend::setFormats(const std::vector<SDRMFormat>& fmts) {
    m_formats = fmts;
}

std::vector<SDRMFormat> Abyssal::CNullBackend::getCursorFormats() {
    return {}; // No cursor support
}

bool Abyssal::CNullBackend::createOutput(const std::string& name) {
    return false;
}

SP<IAllocator> Abyssal::CNullBackend::preferredAllocator() {
    return backend->primaryAllocator;
}

std::vector<SP<IAllocator>> Abyssal::CNullBackend::getAllocators() {
    return {backend->primaryAllocator};
}

Abyssutils::Memory::CWeakPointer<IBackendImplementation> Abyssal::CNullBackend::getPrimary() {
    return {};
}
