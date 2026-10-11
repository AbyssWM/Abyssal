#include <abyssal/allocator/DRMDumb.hpp>
#include <abyssal/backend/Backend.hpp>
#include <abyssal/backend/DRM.hpp>
#include <abyssal/allocator/Swapchain.hpp>
#include "FormatUtils.hpp"
#include "Shared.hpp"
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include "../backend/drm/Renderer.hpp"

using namespace Abyssal;
using namespace Abyssutils::Memory;
#define SP CSharedPointer
#define WP CWeakPointer

Abyssal::CDRMDumbBuffer::CDRMDumbBuffer(const SAllocatorBufferParams& params, Abyssutils::Memory::CWeakPointer<CDRMDumbAllocator> allocator_,
                                           Abyssutils::Memory::CSharedPointer<CSwapchain> swapchain) : allocator(allocator_) {
    attrs.format = params.format;

    if (int ret = drmModeCreateDumbBuffer(allocator->drmFD(), params.size.x, params.size.y, 32, 0, &handle, &stride, &bufferLen); ret < 0) {
        allocator->backend->log(ABYSSAL_LOG_ERROR, std::format("failed to create a drm_dumb buffer: {}", strerror(-ret)));
        return;
    }

    pixelSize = {(double)params.size.x, (double)params.size.y};

    attrs.size          = pixelSize;
    attrs.strides.at(0) = stride;
    attrs.planes        = 1;
    size                = pixelSize;

    uint64_t offset = 0;
    if (int ret = drmModeMapDumbBuffer(allocator->drmFD(), handle, &offset); ret < 0) {
        allocator->backend->log(ABYSSAL_LOG_ERROR, std::format("failed to map a drm_dumb buffer: {}", strerror(-ret)));
        return;
    }

    data = (uint8_t*)mmap(nullptr, bufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, allocator->drmFD(), offset);
    if (data == MAP_FAILED) {
        allocator->backend->log(ABYSSAL_LOG_ERROR, "failed to mmap a drm_dumb buffer");
        return;
    }

    // set the entire buffer so we dont get garbage
    memset(data, 0xFF, bufferLen);

    if (int ret = drmPrimeHandleToFD(allocator->drmFD(), handle, DRM_CLOEXEC, &primeFD); ret < 0) {
        allocator->backend->log(ABYSSAL_LOG_ERROR, std::format("failed to map a drm_dumb buffer: {}", strerror(-ret)));
        return;
    }

    attrs.fds.at(0) = primeFD;

    attrs.success = true;

    allocator->backend->log(ABYSSAL_LOG_DEBUG, std::format("DRM Dumb: Allocated a new buffer with primeFD {}, size {} and format {}", primeFD, attrs.size, fourccToName(attrs.format)));
}

Abyssal::CDRMDumbBuffer::~CDRMDumbBuffer() {
    events.destroy.emit();

    TRACE(allocator->backend->log(ABYSSAL_LOG_TRACE, std::format("DRM Dumb: dropping buffer {}", primeFD)));

    if (handle == 0)
        return;

    if (data)
        munmap(data, bufferLen);

    drmModeDestroyDumbBuffer(allocator->drmFD(), handle);
}

eBufferCapability Abyssal::CDRMDumbBuffer::caps() {
    return eBufferCapability::BUFFER_CAPABILITY_DATAPTR;
}

eBufferType Abyssal::CDRMDumbBuffer::type() {
    return eBufferType::BUFFER_TYPE_DMABUF;
}

void Abyssal::CDRMDumbBuffer::update(const Abyssutils::Math::CRegion& damage) {
    ; // nothing to do
}

bool Abyssal::CDRMDumbBuffer::isSynchronous() {
    return true;
}

bool Abyssal::CDRMDumbBuffer::good() {
    return attrs.success && data;
}

SDMABUFAttrs Abyssal::CDRMDumbBuffer::dmabuf() {
    return attrs;
}

std::tuple<uint8_t*, uint32_t, size_t> Abyssal::CDRMDumbBuffer::beginDataPtr(uint32_t flags) {
    return {data, attrs.format, bufferLen};
}

void Abyssal::CDRMDumbBuffer::endDataPtr() {
    ; // nothing to do
}

Abyssal::CDRMDumbAllocator::~CDRMDumbAllocator() {
    ; // nothing to do
}

SP<CDRMDumbAllocator> Abyssal::CDRMDumbAllocator::create(int drmfd_, Abyssutils::Memory::CWeakPointer<CBackend> backend_) {
    if (drmGetNodeTypeFromFd(drmfd_) != DRM_NODE_PRIMARY) {
        backend_->log(ABYSSAL_LOG_ERROR, "DRM Dumb: Cannot create allocator when drmfd is not the primary node");
        return nullptr;
    }

    uint64_t hasDumb = 0;
    if (drmGetCap(drmfd_, DRM_CAP_DUMB_BUFFER, &hasDumb) < 0) {
        backend_->log(ABYSSAL_LOG_ERROR, "DRM Dumb: Failed to query hasDumb");
        return nullptr;
    }

    if (!hasDumb) {
        backend_->log(ABYSSAL_LOG_ERROR, "DRM Dumb: hasDumb is false, gpu driver doesn't support dumb buffers!");
        return nullptr;
    }

    auto a  = SP<CDRMDumbAllocator>(new CDRMDumbAllocator(drmfd_, backend_));
    a->self = a;

    backend_->log(ABYSSAL_LOG_DEBUG, "DRM Dumb: created a dumb allocator");

    return a;
}

SP<IBuffer> Abyssal::CDRMDumbAllocator::acquire(const SAllocatorBufferParams& params, SP<CSwapchain> swapchain_) {
    auto buf = SP<IBuffer>(new CDRMDumbBuffer(params, self, swapchain_));
    if (!buf->good())
        return nullptr;
    return buf;
}

SP<CBackend> Abyssal::CDRMDumbAllocator::getBackend() {
    return backend.lock();
}

int Abyssal::CDRMDumbAllocator::drmFD() {
    return drmfd;
}

eAllocatorType Abyssal::CDRMDumbAllocator::type() {
    return eAllocatorType::ABYSSAL_ALLOCATOR_TYPE_DRM_DUMB;
}

Abyssal::CDRMDumbAllocator::CDRMDumbAllocator(int fd_, Abyssutils::Memory::CWeakPointer<CBackend> backend_) : backend(backend_), drmfd(fd_) {
    ; // nothing to do
}
