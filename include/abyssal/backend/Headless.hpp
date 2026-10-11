#pragma once

#include "./Backend.hpp"
#include "../allocator/Swapchain.hpp"
#include "../output/Output.hpp"
#include <abyssutils/memory/WeakPtr.hpp>
#include <abyssutils/os/FileDescriptor.hpp>

namespace Abyssal {
    class CBackend;
    class CHeadlessBackend;
    class IAllocator;

    class CHeadlessOutput : public IOutput {
      public:
        virtual ~CHeadlessOutput();
        virtual bool                                                      commit();
        virtual bool                                                      test();
        virtual Abyssutils::Memory::CSharedPointer<IBackendImplementation> getBackend();
        virtual void                                                      scheduleFrame(const scheduleFrameReason reason = ABYSSAL_SCHEDULE_UNKNOWN);
        virtual bool                                                      destroy();
        virtual std::vector<SDRMFormat>                                   getRenderFormats();
        virtual bool                                                      pendingPageFlip();
        virtual bool                                                      pendingIdleFrame();

        Abyssutils::Memory::CWeakPointer<CHeadlessOutput>                  self;

      private:
        CHeadlessOutput(const std::string& name_, Abyssutils::Memory::CWeakPointer<CHeadlessBackend> backend_);

        Abyssutils::Memory::CWeakPointer<CHeadlessBackend>        backend;

        Abyssutils::Memory::CSharedPointer<std::function<void()>> framecb;
        bool                                                     frameScheduled = false;
        std::chrono::steady_clock::time_point                    lastFrame;

        friend class CHeadlessBackend;
    };

    class CHeadlessBackend : public IBackendImplementation {
      public:
        virtual ~CHeadlessBackend();
        virtual eBackendType                                               type();
        virtual bool                                                       start();
        virtual std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>>    pollFDs();
        virtual int                                                        drmFD();
        virtual bool                                                       dispatchEvents();
        virtual uint32_t                                                   capabilities();
        virtual bool                                                       setCursor(Abyssutils::Memory::CSharedPointer<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot);
        virtual void                                                       onReady();
        virtual std::vector<SDRMFormat>                                    getRenderFormats();
        virtual std::vector<SDRMFormat>                                    getCursorFormats();
        virtual bool                                                       createOutput(const std::string& name = "");
        virtual Abyssutils::Memory::CSharedPointer<IAllocator>              preferredAllocator();
        virtual std::vector<Abyssutils::Memory::CSharedPointer<IAllocator>> getAllocators();
        virtual Abyssutils::Memory::CWeakPointer<IBackendImplementation>    getPrimary();

        Abyssutils::Memory::CWeakPointer<CHeadlessBackend>                  self;
        virtual int                                                        drmRenderNodeFD();

      private:
        CHeadlessBackend(Abyssutils::Memory::CSharedPointer<CBackend> backend_);

        Abyssutils::Memory::CWeakPointer<CBackend>                       backend;
        std::vector<Abyssutils::Memory::CSharedPointer<CHeadlessOutput>> outputs;

        size_t                                                          outputIDCounter = 0;

        class CTimer {
          public:
            std::chrono::steady_clock::time_point when;
            std::function<void(void)>             what;
            bool                                  expired();
        };

        struct {
            Abyssutils::OS::CFileDescriptor timerfd;
            std::vector<CTimer>            timers;
        } timers;

        void dispatchTimers();
        void updateTimerFD();
        void addTimer(std::chrono::steady_clock::time_point when, std::function<void(void)> what);

        friend class CBackend;
        friend class CHeadlessOutput;
    };
};
