#pragma once

#include <abyssutils/memory/SharedPtr.hpp>
#include "../buffer/Buffer.hpp"
#include <drm_fourcc.h>

namespace Abyssal {
    class CBackend;
    class CSwapchain;

    struct SAllocatorBufferParams {
        Abyssutils::Math::Vector2D size;
        uint32_t                  format  = DRM_FORMAT_INVALID;
        bool                      scanout = false, cursor = false, multigpu = false;
    };

    enum eAllocatorType {
        ABYSSAL_ALLOCATOR_TYPE_GBM = 0,
        ABYSSAL_ALLOCATOR_TYPE_DRM_DUMB,
    };

    class IAllocator {
      public:
        virtual ~IAllocator()                                                                                                                                      = default;
        virtual Abyssutils::Memory::CSharedPointer<IBuffer>  acquire(const SAllocatorBufferParams& params, Abyssutils::Memory::CSharedPointer<CSwapchain> swapchain) = 0;
        virtual Abyssutils::Memory::CSharedPointer<CBackend> getBackend()                                                                                           = 0;
        virtual int                                         drmFD()                                                                                                = 0;
        virtual eAllocatorType                              type()                                                                                                 = 0;
        virtual void                                        destroyBuffers();
    };
};
