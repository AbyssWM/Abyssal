#pragma once

#include "CommitThread.hpp"
#include <abyssal/backend/DRM.hpp>
#include <abyssal/backend/drm/Atomic.hpp>

#include <atomic>
#include <memory>

namespace Abyssal {
    class CDRMCursorPositionMailbox {
      public:
        void                      store(const Abyssutils::Math::Vector2D& position);
        Abyssutils::Math::Vector2D load() const;

      private:
        std::atomic<uint64_t> m_sequence = 0;
        std::atomic<int64_t>  m_x        = 0;
        std::atomic<int64_t>  m_y        = 0;
    };

    class CDRMAsyncCommitData : public IDRMCommitThreadData {
      public:
        CDRMAsyncCommitData(COutputState::CSnapshot&& snapshot_);
        virtual ~CDRMAsyncCommitData();

        void                                                               pinBuffers();
        void                                                               releasePins();
        void                                                               rollbackMgpu();
        IDRMCommitSubmitter::SResult                                       submit(int drmFD, uint64_t commitID) const noexcept;

        Abyssutils::Memory::CUniquePointer<COutputState::CSnapshot>         snapshot;
        SDRMConnectorCommitData                                            commitData;
        Abyssutils::Memory::CUniquePointer<CDRMAtomicRequest>               request;
        Abyssutils::Memory::CSharedPointer<CDRMOutput>                      output;
        Abyssutils::Memory::CSharedPointer<SDRMConnector>                   connector;
        Abyssutils::Memory::CSharedPointer<IBuffer>                         mainBuffer;
        Abyssutils::Memory::CSharedPointer<IBuffer>                         cursorBuffer;
        Abyssutils::Memory::CSharedPointer<CSwapchain>                      mgpuSwapchain;
        Abyssutils::OS::CFileDescriptor                                     mgpuFence;
        Abyssutils::Memory::CAtomicSharedPointer<CDRMCursorPositionMailbox> cursorMailbox;
        uint32_t                                                           flags         = 0;
        uintptr_t                                                          flipID        = 0;
        uint32_t                                                           cursorPlaneID = 0;
        uint32_t                                                           cursorXProp   = 0;
        uint32_t                                                           cursorYProp   = 0;
        Abyssutils::Math::Vector2D                                          cursorHotspot;
        bool                                                               lateCursor   = false;
        bool                                                               tearing      = false;
        bool                                                               mainPinned   = false;
        bool                                                               cursorPinned = false;
        bool                                                               mgpuAcquired = false;
    };

    Abyssutils::Memory::CUniquePointer<IDRMCommitSubmitter> makeDRMCommitSubmitter(int drmFD);
}
