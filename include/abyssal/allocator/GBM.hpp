#pragma once

#include "Allocator.hpp"

struct gbm_device;
struct gbm_bo;

namespace Abyssal {
    class CGBMAllocator;
    class CBackend;
    class CSwapchain;

    class CGBMBuffer : public IBuffer {
      public:
        virtual ~CGBMBuffer();

        virtual eBufferCapability                      caps();
        virtual eBufferType                            type();
        virtual void                                   update(const Abyssutils::Math::CRegion& damage);
        virtual bool                                   isSynchronous();
        virtual bool                                   good();
        virtual SDMABUFAttrs                           dmabuf();
        virtual std::tuple<uint8_t*, uint32_t, size_t> beginDataPtr(uint32_t flags);
        virtual void                                   endDataPtr();

      private:
        CGBMBuffer(const SAllocatorBufferParams& params, Abyssutils::Memory::CWeakPointer<CGBMAllocator> allocator_, Abyssutils::Memory::CSharedPointer<CSwapchain> swapchain);

        Abyssutils::Memory::CWeakPointer<CGBMAllocator> allocator;

        // gbm stuff
        gbm_bo*      bo         = nullptr;
        void*        boBuffer   = nullptr;
        void*        gboMapping = nullptr;
        SDMABUFAttrs attrs{.success = false};

        friend class CGBMAllocator;
    };

    class CGBMAllocator : public IAllocator {
      public:
        ~CGBMAllocator();
        static Abyssutils::Memory::CSharedPointer<CGBMAllocator> create(int drmfd_, Abyssutils::Memory::CWeakPointer<CBackend> backend_);

        virtual Abyssutils::Memory::CSharedPointer<IBuffer>      acquire(const SAllocatorBufferParams& params, Abyssutils::Memory::CSharedPointer<CSwapchain> swapchain_);
        virtual Abyssutils::Memory::CSharedPointer<CBackend>     getBackend();
        virtual int                                             drmFD();
        virtual eAllocatorType                                  type();
        virtual void                                            destroyBuffers();

        //
        Abyssutils::Memory::CWeakPointer<CGBMAllocator> self;

      private:
        CGBMAllocator(int fd_, Abyssutils::Memory::CWeakPointer<CBackend> backend_);

        // a vector purely for tracking (debugging) the buffers and nothing more
        std::vector<Abyssutils::Memory::CWeakPointer<CGBMBuffer>> buffers;

        int                                                      fd = -1;
        Abyssutils::Memory::CWeakPointer<CBackend>                backend;

        // gbm stuff
        gbm_device* gbmDevice            = nullptr;
        std::string gbmDeviceBackendName = "";
        std::string drmName              = "";

        friend class CGBMBuffer;
        friend class CDRMRenderer;
    };
};
