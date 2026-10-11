#pragma once

#include <sys/types.h>
#include <abyssutils/signal/Signal.hpp>
#include <abyssutils/memory/SharedPtr.hpp>
#include "../input/Input.hpp"
#include <vector>

struct udev;
struct udev_monitor;
struct udev_device;
struct libseat;
struct libinput;
struct libinput_event;
struct libinput_device;
struct libinput_tablet_tool;

namespace Abyssal {
    class CBackend;
    class CSession;
    class CLibinputDevice;
    struct SPollFD;

    class CSessionDevice {
      public:
        CSessionDevice(Abyssutils::Memory::CSharedPointer<CSession> session_, const std::string& path_);
        ~CSessionDevice();

        static Abyssutils::Memory::CSharedPointer<CSessionDevice> openIfKMS(Abyssutils::Memory::CSharedPointer<CSession> session_, const std::string& path_);

        bool                                                     supportsKMS();
        void                                                     resolveMatchingRenderNode(udev_device* cardDevice);

        int                                                      fd       = -1;
        int                                                      deviceID = -1;
        dev_t                                                    dev;
        std::string                                              path;

        enum eChangeEventType : uint32_t {
            ABYSSAL_SESSION_EVENT_CHANGE_HOTPLUG = 0,
            ABYSSAL_SESSION_EVENT_CHANGE_LEASE,
        };

        struct SChangeEvent {
            eChangeEventType type = ABYSSAL_SESSION_EVENT_CHANGE_HOTPLUG;

            struct {
                uint32_t connectorID = 0, propID = 0;
            } hotplug;
        };

        struct {
            Abyssutils::Signal::CSignalT<SChangeEvent> change;
            Abyssutils::Signal::CSignalT<>             remove;
        } events;

        int renderNodeFd = -1;

      private:
        Abyssutils::Memory::CWeakPointer<CSession> session;
    };

    class CLibinputKeyboard : public IKeyboard {
      public:
        CLibinputKeyboard(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputKeyboard() {
            ;
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();
        virtual void               updateLEDs(uint32_t leds);

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;

        friend class CLibinputDevice;
    };

    class CLibinputMouse : public IPointer {
      public:
        CLibinputMouse(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputMouse() {
            ;
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;

        friend class CLibinputDevice;
    };

    class CLibinputTouch : public ITouch {
      public:
        CLibinputTouch(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputTouch() {
            ;
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;

        friend class CLibinputDevice;
    };

    class CLibinputSwitch : public ISwitch {
      public:
        CLibinputSwitch(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputSwitch() {
            ;
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();

        eSwitchType                type  = ABYSSAL_SWITCH_TYPE_UNKNOWN;
        bool                       state = false;

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;

        friend class CLibinputDevice;
    };

    class CLibinputTablet : public ITablet {
      public:
        CLibinputTablet(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputTablet() {
            ;
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;

        friend class CLibinputDevice;
    };

    class CLibinputTabletTool : public ITabletTool {
      public:
        CLibinputTabletTool(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev, libinput_tablet_tool* tool);
        virtual ~CLibinputTabletTool();

        virtual libinput_device*      getLibinputHandle();
        virtual libinput_tablet_tool* getLibinputTool();
        virtual const std::string&    getName();

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice> device;
        libinput_tablet_tool*                            libinputTool = nullptr;

        friend class CLibinputDevice;
    };

    class CLibinputTabletPad : public ITabletPad {
      public:
        CLibinputTabletPad(Abyssutils::Memory::CSharedPointer<CLibinputDevice> dev);
        virtual ~CLibinputTabletPad();

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName();

      private:
        Abyssutils::Memory::CWeakPointer<CLibinputDevice>               device;

        Abyssutils::Memory::CSharedPointer<ITabletPad::STabletPadGroup> createGroupFromID(int id);

        friend class CLibinputDevice;
    };

    class CLibinputDevice {
      public:
        CLibinputDevice(libinput_device* device, Abyssutils::Memory::CWeakPointer<CSession> session_);
        ~CLibinputDevice();

        void                                                                init();

        libinput_device*                                                    device;
        Abyssutils::Memory::CWeakPointer<CLibinputDevice>                    self;
        Abyssutils::Memory::CWeakPointer<CSession>                           session;
        std::string                                                         name;

        Abyssutils::Memory::CSharedPointer<CLibinputKeyboard>                keyboard;
        Abyssutils::Memory::CSharedPointer<CLibinputMouse>                   mouse;
        Abyssutils::Memory::CSharedPointer<CLibinputTouch>                   touch;
        Abyssutils::Memory::CSharedPointer<CLibinputSwitch>                  switchy; // :)
        Abyssutils::Memory::CSharedPointer<CLibinputTablet>                  tablet;
        Abyssutils::Memory::CSharedPointer<CLibinputTabletPad>               tabletPad;
        std::vector<Abyssutils::Memory::CSharedPointer<CLibinputTabletTool>> tabletTools;

        Abyssutils::Memory::CSharedPointer<CLibinputTabletTool>              toolFrom(libinput_tablet_tool* tool);
    };

    class CSession {
      public:
        ~CSession();

        static Abyssutils::Memory::CSharedPointer<CSession>              attempt(Abyssutils::Memory::CSharedPointer<CBackend> backend_);

        bool                                                            active = true; // whether the current vt is ours
        uint32_t                                                        vt     = 0;    // 0 means unsupported
        std::string                                                     seatName;
        Abyssutils::Memory::CWeakPointer<CSession>                       self;

        std::vector<Abyssutils::Memory::CSharedPointer<CSessionDevice>>  sessionDevices;
        std::vector<Abyssutils::Memory::CSharedPointer<CLibinputDevice>> libinputDevices;

        udev*                                                           udevHandle     = nullptr;
        udev_monitor*                                                   udevMonitor    = nullptr;
        libseat*                                                        libseatHandle  = nullptr;
        libinput*                                                       libinputHandle = nullptr;

        std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>>         pollFDs();
        void                                                            dispatchPendingEventsAsync();
        bool                                                            switchVT(uint32_t vt);
        void                                                            onReady();

        struct SAddDrmCardEvent {
            std::string path;
        };

        struct {
            Abyssutils::Signal::CSignalT<>                 changeActive;
            Abyssutils::Signal::CSignalT<SAddDrmCardEvent> addDrmCard;
            Abyssutils::Signal::CSignalT<>                 destroy;
        } events;

      private:
        Abyssutils::Memory::CWeakPointer<CBackend>               backend;
        std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>> polls;

        void                                                    dispatchUdevEvents();
        void                                                    dispatchLibinputEvents();
        void                                                    dispatchLibseatEvents();
        void                                                    handleLibinputEvent(libinput_event* e);
        void                                                    handleLibinputTabletToolAxis(libinput_event* e);

        friend class CSessionDevice;
        friend class CLibinputDevice;
    };
};
