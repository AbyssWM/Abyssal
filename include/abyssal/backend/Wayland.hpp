#pragma once

#include "./Backend.hpp"
#include "./FrameScheduler.hpp"
#include "../allocator/Swapchain.hpp"
#include "../output/Output.hpp"
#include "../input/Input.hpp"
#include <abyssutils/memory/WeakPtr.hpp>
#include <wayland-client.h>
#include <wayland.hpp>
#include <xdg-shell.hpp>
#include <linux-dmabuf-v1.hpp>
#include <tuple>

namespace Abyssal {
    class CBackend;
    class CWaylandBackend;
    class CWaylandOutput;
    class CWaylandPointer;

    typedef std::function<void(void)> FIdleCallback;

    class CWaylandBuffer {
      public:
        CWaylandBuffer(Abyssutils::Memory::CSharedPointer<IBuffer> buffer_, Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend_);
        ~CWaylandBuffer();
        bool good();

        bool pendingRelease = false;

      private:
        struct {
            Abyssutils::Memory::CSharedPointer<CCWlBuffer> buffer;
        } waylandState;

        Abyssutils::Memory::CWeakPointer<IBuffer>         buffer;
        Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend;

        friend class CWaylandOutput;
    };

    class CWaylandOutput : public IOutput {
      public:
        virtual ~CWaylandOutput();
        virtual bool                                                      commit();
        virtual bool                                                      test();
        virtual Abyssutils::Memory::CSharedPointer<IBackendImplementation> getBackend();
        virtual bool                                                      setCursor(Abyssutils::Memory::CSharedPointer<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot);
        virtual void                                                      moveCursor(const Abyssutils::Math::Vector2D& coord, bool skipSchedule = false);
        virtual void                                                      scheduleFrame(const scheduleFrameReason reason = ABYSSAL_SCHEDULE_UNKNOWN);
        virtual Abyssutils::Math::Vector2D                                 cursorPlaneSize();
        virtual bool                                                      destroy();
        virtual std::vector<SDRMFormat>                                   getRenderFormats();
        virtual bool                                                      pendingPageFlip();
        virtual bool                                                      pendingIdleFrame();

        Abyssutils::Memory::CWeakPointer<CWaylandOutput>                   self;

      private:
        CWaylandOutput(const std::string& name_, Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend_);

        Abyssutils::Memory::CWeakPointer<CWaylandBackend>  backend;

        Abyssutils::Memory::CSharedPointer<CWaylandBuffer> wlBufferFromBuffer(Abyssutils::Memory::CSharedPointer<IBuffer> buffer);

        void                                              onFrameDone();
        void                                              onEnter(Abyssutils::Memory::CSharedPointer<CCWlPointer> pointer, uint32_t serial);

        // frame loop — unified scheduler shared with DRM. See CFrameScheduler.
        CFrameScheduler                                              sched;
        Abyssutils::Signal::CAbyssSignalListener                       frameReadyListener;
        Abyssutils::Signal::CAbyssSignalListener                       rescheduleListener;
        Abyssutils::Memory::CSharedPointer<std::function<void(void)>> frameIdle;

        struct {
            std::vector<std::pair<Abyssutils::Memory::CWeakPointer<IBuffer>, Abyssutils::Memory::CSharedPointer<CWaylandBuffer>>> buffers;
        } backendState;

        struct {
            Abyssutils::Memory::CSharedPointer<IBuffer>     cursorBuffer;
            Abyssutils::Memory::CSharedPointer<CCWlSurface> cursorSurface;
            Abyssutils::Memory::CSharedPointer<CCWlBuffer>  cursorWlBuffer;
            uint32_t                                       serial = 0;
            Abyssutils::Math::Vector2D                      hotspot;
        } cursorState;

        struct {
            Abyssutils::Memory::CSharedPointer<CCWlSurface>   surface;
            Abyssutils::Memory::CSharedPointer<CCXdgSurface>  xdgSurface;
            Abyssutils::Memory::CSharedPointer<CCXdgToplevel> xdgToplevel;
            Abyssutils::Memory::CSharedPointer<CCWlCallback>  frameCallback;
            Abyssutils::Math::Vector2D                        surfaceSize;
        } waylandState;

        friend class CWaylandBackend;
        friend class CWaylandPointer;
    };

    class CWaylandKeyboard : public IKeyboard {
      public:
        CWaylandKeyboard(Abyssutils::Memory::CSharedPointer<CCWlKeyboard> keyboard_, Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend_);
        virtual ~CWaylandKeyboard();

        virtual const std::string&                       getName();

        Abyssutils::Memory::CSharedPointer<CCWlKeyboard>  keyboard;
        Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend;

      private:
        const std::string name = "wl_keyboard";
    };

    class CWaylandPointer : public IPointer {
      public:
        CWaylandPointer(Abyssutils::Memory::CSharedPointer<CCWlPointer> pointer_, Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend_);
        virtual ~CWaylandPointer();

        virtual const std::string&                       getName();

        Abyssutils::Memory::CSharedPointer<CCWlPointer>   pointer;
        Abyssutils::Memory::CWeakPointer<CWaylandBackend> backend;

      private:
        void              emitWarp(uint32_t timeMs, wl_fixed_t x, wl_fixed_t y);

        const std::string name = "wl_pointer";
    };

    class CWaylandBackend : public IBackendImplementation {
      public:
        virtual ~CWaylandBackend();
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

        Abyssutils::Memory::CWeakPointer<CWaylandBackend>                   self;
        virtual int                                                        drmRenderNodeFD();

      private:
        CWaylandBackend(Abyssutils::Memory::CSharedPointer<CBackend> backend);

        void initSeat();
        void initShell();
        bool initDmabuf();

        //
        Abyssutils::Memory::CWeakPointer<CBackend>                        backend;
        std::vector<Abyssutils::Memory::CSharedPointer<CWaylandOutput>>   outputs;
        std::vector<Abyssutils::Memory::CSharedPointer<CWaylandKeyboard>> keyboards;
        std::vector<Abyssutils::Memory::CSharedPointer<CWaylandPointer>>  pointers;
        std::vector<FIdleCallback>                                       idleCallbacks;

        // pointer focus
        Abyssutils::Memory::CWeakPointer<CWaylandOutput> focusedOutput;
        uint32_t                                        lastEnterSerial = 0;

        // state
        size_t lastOutputID = 0;

        // dmabuf formats
        std::vector<SDRMFormat> dmabufFormats;

        struct {
            wl_display* display = nullptr;

            // hw-s types
            Abyssutils::Memory::CSharedPointer<CCWlRegistry>               registry;
            Abyssutils::Memory::CSharedPointer<CCWlSeat>                   seat;
            Abyssutils::Memory::CSharedPointer<CCWlShm>                    shm;
            Abyssutils::Memory::CSharedPointer<CCXdgWmBase>                xdg;
            Abyssutils::Memory::CSharedPointer<CCWlCompositor>             compositor;
            Abyssutils::Memory::CSharedPointer<CCZwpLinuxDmabufV1>         dmabuf;
            Abyssutils::Memory::CSharedPointer<CCZwpLinuxDmabufFeedbackV1> dmabufFeedback;

            // control
            bool dmabufFailed = false;
        } waylandState;

        struct {
            int         fd       = -1;
            std::string nodeName = "";
        } drmState;

        friend class CBackend;
        friend class CWaylandKeyboard;
        friend class CWaylandPointer;
        friend class CWaylandOutput;
        friend class CWaylandBuffer;
    };
};
