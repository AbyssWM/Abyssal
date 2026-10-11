#pragma once

#include <abyssutils/signal/Signal.hpp>
#include <abyssutils/math/Vector2D.hpp>
#include <abyssutils/memory/WeakPtr.hpp>

struct libinput_device;
struct libinput_tablet_tool;

namespace Abyssal {
    class IOutput;
    class ITabletTool;

    class IKeyboard {
      public:
        virtual ~IKeyboard() {
            events.destroy.emit();
        }
        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName() = 0;
        virtual void               updateLEDs(uint32_t leds);

        struct SKeyEvent {
            uint32_t timeMs  = 0;
            uint32_t key     = 0;
            bool     pressed = false;
        };

        struct SModifiersEvent {
            uint32_t depressed = 0, latched = 0, locked = 0, group = 0;
        };

        struct {
            Abyssutils::Signal::CSignalT<>                destroy;
            Abyssutils::Signal::CSignalT<SKeyEvent>       key;
            Abyssutils::Signal::CSignalT<SModifiersEvent> modifiers;
        } events;
    };

    class IPointer {
      public:
        virtual ~IPointer() {
            events.destroy.emit();
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName() = 0;

        enum ePointerAxis : uint32_t {
            ABYSSAL_POINTER_AXIS_VERTICAL = 0,
            ABYSSAL_POINTER_AXIS_HORIZONTAL,
        };

        enum ePointerAxisSource : uint32_t {
            ABYSSAL_POINTER_AXIS_SOURCE_WHEEL = 0,
            ABYSSAL_POINTER_AXIS_SOURCE_FINGER,
            ABYSSAL_POINTER_AXIS_SOURCE_CONTINUOUS,
            ABYSSAL_POINTER_AXIS_SOURCE_TILT,
        };

        enum ePointerAxisRelativeDirection : uint32_t {
            ABYSSAL_POINTER_AXIS_RELATIVE_IDENTICAL = 0,
            ABYSSAL_POINTER_AXIS_RELATIVE_INVERTED,
        };

        struct SMoveEvent {
            uint32_t                  timeMs = 0;
            Abyssutils::Math::Vector2D delta, unaccel;
        };

        struct SWarpEvent {
            uint32_t                                 timeMs = 0;
            Abyssutils::Math::Vector2D                absolute;
            Abyssutils::Memory::CWeakPointer<IOutput> output; // absolute is local to this output when present
        };

        struct SButtonEvent {
            uint32_t timeMs  = 0;
            uint32_t button  = 0;
            bool     pressed = false;
        };

        struct SAxisEvent {
            uint32_t                      timeMs    = 0;
            ePointerAxis                  axis      = ABYSSAL_POINTER_AXIS_VERTICAL;
            ePointerAxisSource            source    = ABYSSAL_POINTER_AXIS_SOURCE_WHEEL;
            ePointerAxisRelativeDirection direction = ABYSSAL_POINTER_AXIS_RELATIVE_IDENTICAL;
            double                        delta = 0.0, discrete = 0.0;
        };

        struct SSwipeBeginEvent {
            uint32_t timeMs  = 0;
            uint32_t fingers = 0;
        };

        struct SSwipeUpdateEvent {
            uint32_t                  timeMs  = 0;
            uint32_t                  fingers = 0;
            Abyssutils::Math::Vector2D delta;
        };

        struct SSwipeEndEvent {
            uint32_t timeMs    = 0;
            bool     cancelled = false;
        };

        struct SPinchBeginEvent {
            uint32_t timeMs  = 0;
            uint32_t fingers = 0;
        };

        struct SPinchUpdateEvent {
            uint32_t                  timeMs  = 0;
            uint32_t                  fingers = 0;
            Abyssutils::Math::Vector2D delta;
            double                    scale = 1.0, rotation = 0.0;
        };

        struct SPinchEndEvent {
            uint32_t timeMs    = 0;
            bool     cancelled = false;
        };

        struct SHoldBeginEvent {
            uint32_t timeMs  = 0;
            uint32_t fingers = 0;
        };

        struct SHoldEndEvent {
            uint32_t timeMs    = 0;
            bool     cancelled = false;
        };

        struct {
            Abyssutils::Signal::CSignalT<>                  destroy;
            Abyssutils::Signal::CSignalT<SMoveEvent>        move;
            Abyssutils::Signal::CSignalT<SWarpEvent>        warp;
            Abyssutils::Signal::CSignalT<SButtonEvent>      button;
            Abyssutils::Signal::CSignalT<SAxisEvent>        axis;
            Abyssutils::Signal::CSignalT<>                  frame;

            Abyssutils::Signal::CSignalT<SSwipeBeginEvent>  swipeBegin;
            Abyssutils::Signal::CSignalT<SSwipeUpdateEvent> swipeUpdate;
            Abyssutils::Signal::CSignalT<SSwipeEndEvent>    swipeEnd;

            Abyssutils::Signal::CSignalT<SPinchBeginEvent>  pinchBegin;
            Abyssutils::Signal::CSignalT<SPinchUpdateEvent> pinchUpdate;
            Abyssutils::Signal::CSignalT<SPinchEndEvent>    pinchEnd;

            Abyssutils::Signal::CSignalT<SHoldBeginEvent>   holdBegin;
            Abyssutils::Signal::CSignalT<SHoldEndEvent>     holdEnd;
        } events;
    };

    class ITouch {
      public:
        virtual ~ITouch() {
            events.destroy.emit();
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName() = 0;

        Abyssutils::Math::Vector2D  physicalSize; // in mm, 0,0 if unknown

        struct SDownEvent {
            uint32_t                  timeMs  = 0;
            int32_t                   touchID = 0;
            Abyssutils::Math::Vector2D pos;
        };

        struct SUpEvent {
            uint32_t timeMs  = 0;
            int32_t  touchID = 0;
        };

        struct SMotionEvent {
            uint32_t                  timeMs  = 0;
            int32_t                   touchID = 0;
            Abyssutils::Math::Vector2D pos;
        };

        struct SCancelEvent {
            uint32_t timeMs  = 0;
            int32_t  touchID = 0;
        };

        struct {
            Abyssutils::Signal::CSignalT<>             destroy;
            Abyssutils::Signal::CSignalT<SMotionEvent> move;
            Abyssutils::Signal::CSignalT<SDownEvent>   down;
            Abyssutils::Signal::CSignalT<SUpEvent>     up;
            Abyssutils::Signal::CSignalT<SCancelEvent> cancel;
            Abyssutils::Signal::CSignalT<>             frame;
        } events;
    };

    class ISwitch {
      public:
        virtual ~ISwitch() {
            events.destroy.emit();
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName() = 0;

        enum eSwitchType : uint32_t {
            ABYSSAL_SWITCH_TYPE_UNKNOWN = 0,
            ABYSSAL_SWITCH_TYPE_LID,
            ABYSSAL_SWITCH_TYPE_TABLET_MODE,
        };

        struct SFireEvent {
            uint32_t    timeMs = 0;
            eSwitchType type   = ABYSSAL_SWITCH_TYPE_UNKNOWN;
            bool        enable = false;
        };

        struct {
            Abyssutils::Signal::CSignalT<>           destroy;
            Abyssutils::Signal::CSignalT<SFireEvent> fire;
        } events;
    };

    enum eTabletToolAxes : uint32_t {
        ABYSSAL_TABLET_TOOL_AXIS_X        = (1 << 0),
        ABYSSAL_TABLET_TOOL_AXIS_Y        = (1 << 1),
        ABYSSAL_TABLET_TOOL_AXIS_DISTANCE = (1 << 2),
        ABYSSAL_TABLET_TOOL_AXIS_PRESSURE = (1 << 3),
        ABYSSAL_TABLET_TOOL_AXIS_TILT_X   = (1 << 4),
        ABYSSAL_TABLET_TOOL_AXIS_TILT_Y   = (1 << 5),
        ABYSSAL_TABLET_TOOL_AXIS_ROTATION = (1 << 6),
        ABYSSAL_TABLET_TOOL_AXIS_SLIDER   = (1 << 7),
        ABYSSAL_TABLET_TOOL_AXIS_WHEEL    = (1 << 8),
    };

    class ITablet {
      public:
        virtual ~ITablet() {
            events.destroy.emit();
        }

        virtual libinput_device*   getLibinputHandle();
        virtual const std::string& getName() = 0;

        uint16_t                   usbVendorID = 0, usbProductID = 0;
        Abyssutils::Math::Vector2D  physicalSize; // mm
        std::vector<std::string>   paths;

        struct SAxisEvent {
            Abyssutils::Memory::CSharedPointer<ITabletTool> tool;

            uint32_t                                       timeMs = 0, updatedAxes = 0;
            Abyssutils::Math::Vector2D                      absolute;
            Abyssutils::Math::Vector2D                      delta;
            Abyssutils::Math::Vector2D                      tilt;
            double                                         pressure = 0.0, distance = 0.0, rotation = 0.0, slider = 0.0, wheelDelta = 0.0;
        };

        struct SProximityEvent {
            Abyssutils::Memory::CSharedPointer<ITabletTool> tool;

            uint32_t                                       timeMs = 0;
            Abyssutils::Math::Vector2D                      absolute;
            bool                                           in = false;
        };

        struct STipEvent {
            Abyssutils::Memory::CSharedPointer<ITabletTool> tool;

            uint32_t                                       timeMs = 0;
            Abyssutils::Math::Vector2D                      absolute;
            bool                                           down = false;
        };

        struct SButtonEvent {
            Abyssutils::Memory::CSharedPointer<ITabletTool> tool;

            uint32_t                                       timeMs = 0, button = 0;
            bool                                           down = false;
        };

        struct {
            Abyssutils::Signal::CSignalT<SAxisEvent>      axis;
            Abyssutils::Signal::CSignalT<SProximityEvent> proximity;
            Abyssutils::Signal::CSignalT<STipEvent>       tip;
            Abyssutils::Signal::CSignalT<SButtonEvent>    button;
            Abyssutils::Signal::CSignalT<>                destroy;
        } events;
    };

    class ITabletTool {
      public:
        virtual ~ITabletTool() {
            events.destroy.emit();
        }

        virtual libinput_device*      getLibinputHandle();
        virtual libinput_tablet_tool* getLibinputTool();
        virtual const std::string&    getName() = 0;

        enum eTabletToolType : uint32_t {
            ABYSSAL_TABLET_TOOL_TYPE_INVALID = 0,
            ABYSSAL_TABLET_TOOL_TYPE_PEN,
            ABYSSAL_TABLET_TOOL_TYPE_ERASER,
            ABYSSAL_TABLET_TOOL_TYPE_BRUSH,
            ABYSSAL_TABLET_TOOL_TYPE_PENCIL,
            ABYSSAL_TABLET_TOOL_TYPE_AIRBRUSH,
            ABYSSAL_TABLET_TOOL_TYPE_MOUSE,
            ABYSSAL_TABLET_TOOL_TYPE_LENS,
            ABYSSAL_TABLET_TOOL_TYPE_TOTEM,
        };

        eTabletToolType type   = ABYSSAL_TABLET_TOOL_TYPE_INVALID;
        uint64_t        serial = 0, id = 0;

        enum eTabletToolCapabilities : uint32_t {
            ABYSSAL_TABLET_TOOL_CAPABILITY_TILT     = (1 << 0),
            ABYSSAL_TABLET_TOOL_CAPABILITY_PRESSURE = (1 << 1),
            ABYSSAL_TABLET_TOOL_CAPABILITY_DISTANCE = (1 << 2),
            ABYSSAL_TABLET_TOOL_CAPABILITY_ROTATION = (1 << 3),
            ABYSSAL_TABLET_TOOL_CAPABILITY_SLIDER   = (1 << 4),
            ABYSSAL_TABLET_TOOL_CAPABILITY_WHEEL    = (1 << 5),
        };

        uint32_t capabilities = 0; // enum eTabletToolCapabilities

        struct {
            Abyssutils::Signal::CSignalT<> destroy;
        } events;
    };

    class ITabletPad {
      public:
        virtual ~ITabletPad() {
            events.destroy.emit();
        }

        struct STabletPadGroup {
            std::vector<uint32_t> buttons, strips, rings;
            uint16_t              modes = 0;
        };

        virtual libinput_device*                                        getLibinputHandle();
        virtual const std::string&                                      getName() = 0;

        uint16_t                                                        buttons = 0, rings = 0, strips = 0;

        std::vector<std::string>                                        paths;
        std::vector<Abyssutils::Memory::CSharedPointer<STabletPadGroup>> groups;

        //

        struct SButtonEvent {
            uint32_t timeMs = 0, button = 0;
            bool     down = false;
            uint16_t mode = 0, group = 0;
        };

        enum eTabletPadRingSource : uint16_t {
            ABYSSAL_TABLET_PAD_RING_SOURCE_UNKNOWN = 0,
            ABYSSAL_TABLET_PAD_RING_SOURCE_FINGER,
        };

        enum eTabletPadStripSource : uint16_t {
            ABYSSAL_TABLET_PAD_STRIP_SOURCE_UNKNOWN = 0,
            ABYSSAL_TABLET_PAD_STRIP_SOURCE_FINGER,
        };

        struct SRingEvent {
            uint32_t             timeMs = 0;
            eTabletPadRingSource source = ABYSSAL_TABLET_PAD_RING_SOURCE_UNKNOWN;
            uint16_t             ring   = 0;
            double               pos    = 0.0;
            uint16_t             mode   = 0;
        };

        struct SStripEvent {
            uint32_t              timeMs = 0;
            eTabletPadStripSource source = ABYSSAL_TABLET_PAD_STRIP_SOURCE_UNKNOWN;
            uint16_t              strip  = 0;
            double                pos    = 0.0;
            uint16_t              mode   = 0;
        };

        struct {
            Abyssutils::Signal::CSignalT<>             destroy;
            Abyssutils::Signal::CSignalT<SButtonEvent> button;
            Abyssutils::Signal::CSignalT<SRingEvent>   ring;
            Abyssutils::Signal::CSignalT<SStripEvent>  strip;
            Abyssutils::Signal::CSignalT<>             attach;
        } events;
    };
}
