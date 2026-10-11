#pragma once

#include "Allocator.hpp"

namespace Abyssal {

    class IBackendImplementation;
    class IOutput;

    struct SSwapchainOptions {
        size_t                    length = 0;
        Abyssutils::Math::Vector2D size;
        uint32_t                  format  = DRM_FORMAT_INVALID; // if you leave this on invalid, the swapchain will choose an appropriate format (and modifier) for you.
        bool                      scanout = false, cursor = false /* requires scanout = true */, multigpu = false /* if true, will force linear */;
        Abyssutils::Memory::CWeakPointer<IOutput> scanoutOutput;
    };

    class CSwapchain {
      public:
        static Abyssutils::Memory::CSharedPointer<CSwapchain> create(Abyssutils::Memory::CSharedPointer<IAllocator>             allocator_,
                                                                    Abyssutils::Memory::CSharedPointer<IBackendImplementation> backendImpl_);

        bool                                                 reconfigure(const SSwapchainOptions& options_);

        bool                                                 contains(Abyssutils::Memory::CSharedPointer<IBuffer> buffer);
        Abyssutils::Memory::CSharedPointer<IBuffer>           next(int* age);
        const SSwapchainOptions&                             currentOptions();
        Abyssutils::Memory::CSharedPointer<IAllocator>        getAllocator();

        // rolls the buffers back, marking the last consumed as the next valid.
        // useful if e.g. a commit fails and we don't wanna write to the previous buffer that is
        // in use.
        void rollback();

      private:
        CSwapchain(Abyssutils::Memory::CSharedPointer<IAllocator> allocator_, Abyssutils::Memory::CSharedPointer<IBackendImplementation> backendImpl_);

        bool fullReconfigure(const SSwapchainOptions& options_);
        bool resize(size_t newSize);

        //
        Abyssutils::Memory::CWeakPointer<CSwapchain>             self;
        SSwapchainOptions                                       options;
        Abyssutils::Memory::CSharedPointer<IAllocator>           allocator;
        Abyssutils::Memory::CWeakPointer<IBackendImplementation> backendImpl;
        std::vector<Abyssutils::Memory::CSharedPointer<IBuffer>> buffers;
        int                                                     lastAcquired = 0;

        friend class CGBMBuffer;
    };
};
