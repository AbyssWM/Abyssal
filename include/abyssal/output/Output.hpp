#pragma once

#include <chrono>
#include <array>
#include <vector>
#include <optional>
#include <abyssutils/signal/Signal.hpp>
#include <abyssutils/memory/SharedPtr.hpp>
#include <abyssutils/math/Region.hpp>
#include <abyssutils/math/Mat3x3.hpp>
#include <abyssutils/os/FileDescriptor.hpp>
#include <drm_fourcc.h>
#include <xf86drmMode.h>
#include "../allocator/Swapchain.hpp"
#include "../buffer/Buffer.hpp"
#include "../backend/Misc.hpp"

namespace Abyssal {

    class IBackendImplementation;

    struct SOutputMode {
        Abyssutils::Math::Vector2D      pixelSize;
        unsigned int                   refreshRate = 0 /* in mHz */;
        bool                           preferred   = false;
        std::optional<drmModeModeInfo> modeInfo; // if this is a drm mode, this will be populated.
    };

    enum eOutputPresentationMode : uint32_t {
        ABYSSAL_OUTPUT_PRESENTATION_VSYNC = 0,
        ABYSSAL_OUTPUT_PRESENTATION_IMMEDIATE, // likely tearing
    };

    enum eSubpixelMode : uint32_t {
        ABYSSAL_SUBPIXEL_UNKNOWN = 0,
        ABYSSAL_SUBPIXEL_NONE,
        ABYSSAL_SUBPIXEL_HORIZONTAL_RGB,
        ABYSSAL_SUBPIXEL_HORIZONTAL_BGR,
        ABYSSAL_SUBPIXEL_VERTICAL_RGB,
        ABYSSAL_SUBPIXEL_VERTICAL_BGR,
    };

    enum eOutputColorRange : uint32_t {
        ABYSSAL_OUTPUT_COLOR_RANGE_AUTO = 0, // leave the driver default
        ABYSSAL_OUTPUT_COLOR_RANGE_FULL,     // force full range
        ABYSSAL_OUTPUT_COLOR_RANGE_LIMITED,  // force limited (16-235) range
    };

    class IOutput;

    class COutputState {
      public:
        enum eOutputStateProperties : uint32_t {
            ABYSSAL_OUTPUT_STATE_DAMAGE             = (1 << 0),
            ABYSSAL_OUTPUT_STATE_ENABLED            = (1 << 1),
            ABYSSAL_OUTPUT_STATE_ADAPTIVE_SYNC      = (1 << 2),
            ABYSSAL_OUTPUT_STATE_PRESENTATION_MODE  = (1 << 3),
            ABYSSAL_OUTPUT_STATE_GAMMA_LUT          = (1 << 4),
            ABYSSAL_OUTPUT_STATE_MODE               = (1 << 5),
            ABYSSAL_OUTPUT_STATE_FORMAT             = (1 << 6),
            ABYSSAL_OUTPUT_STATE_BUFFER             = (1 << 7),
            ABYSSAL_OUTPUT_STATE_EXPLICIT_IN_FENCE  = (1 << 8),
            ABYSSAL_OUTPUT_STATE_EXPLICIT_OUT_FENCE = (1 << 9),
            ABYSSAL_OUTPUT_STATE_CTM                = (1 << 10),
            ABYSSAL_OUTPUT_STATE_HDR                = (1 << 11),
            ABYSSAL_OUTPUT_STATE_DEGAMMA_LUT        = (1 << 12),
            ABYSSAL_OUTPUT_STATE_WCG                = (1 << 13),
            ABYSSAL_OUTPUT_STATE_CURSOR_SHAPE       = (1 << 14),
            ABYSSAL_OUTPUT_STATE_CURSOR_POS         = (1 << 15),
        };

        struct SInternalState {
            uint32_t                                       committed = 0; // enum eOutputStateProperties

            Abyssutils::Math::CRegion                       damage;
            bool                                           enabled          = false;
            bool                                           adaptiveSync     = false;
            eOutputPresentationMode                        presentationMode = ABYSSAL_OUTPUT_PRESENTATION_VSYNC;
            std::vector<uint16_t>                          gammaLut;   // Gamma lut in the format [r,g,b]+
            std::vector<uint16_t>                          degammaLut; // Gamma lut in the format [r,g,b]+
            Abyssutils::Math::Vector2D                      lastModeSize;
            Abyssutils::Memory::CWeakPointer<SOutputMode>   mode;
            Abyssutils::Memory::CSharedPointer<SOutputMode> customMode;
            uint32_t                                       drmFormat = DRM_FORMAT_INVALID;
            Abyssutils::Memory::CSharedPointer<IBuffer>     buffer;
            int32_t                                        explicitInFence = -1, explicitOutFence = -1;
            Abyssutils::Math::Mat3x3                        ctm            = Abyssutils::Math::Mat3x3::identity();
            bool                                           wideColorGamut = false;
            hdr_output_metadata                            hdrMetadata    = {};
            uint16_t                                       contentType    = DRM_MODE_CONTENT_TYPE_GRAPHICS;
            eOutputColorRange                              colorRange     = ABYSSAL_OUTPUT_COLOR_RANGE_AUTO;
        };

        class CSnapshot {
          public:
            CSnapshot(CSnapshot&&) = default;

            const SInternalState& state() const;
            bool                  needsReconfig() const;
            int                   error() const;

          private:
            CSnapshot(const COutputState* owner, const SInternalState& state, const std::array<uint64_t, 16>& generations);

            const COutputState*            m_owner = nullptr;
            SInternalState                 m_state;
            std::array<uint64_t, 16>       m_generations = {};
            Abyssutils::OS::CFileDescriptor m_explicitInFence;
            int                            m_error = 0;

            friend class COutputState;
        };

        const SInternalState& state();
        const SInternalState& state() const;
        CSnapshot             snapshot() const;
        void                  consume(const CSnapshot& snapshot);
        void                  rearm(const CSnapshot& snapshot);

        bool                  needsReconfig() const;
        void                  addDamage(const Abyssutils::Math::CRegion& region);
        void                  clearDamage();
        void                  setEnabled(bool enabled);
        void                  setAdaptiveSync(bool enabled);
        void                  setPresentationMode(eOutputPresentationMode mode);
        void                  setGammaLut(const std::vector<uint16_t>& lut);
        void                  setDeGammaLut(const std::vector<uint16_t>& lut);
        void                  setMode(Abyssutils::Memory::CSharedPointer<SOutputMode> mode);
        void                  setCustomMode(Abyssutils::Memory::CSharedPointer<SOutputMode> mode);
        void                  setFormat(uint32_t drmFormat);
        void                  setBuffer(Abyssutils::Memory::CSharedPointer<IBuffer> buffer);
        void                  setExplicitInFence(int32_t fenceFD); // -1 removes
        void                  enableExplicitOutFenceForNextCommit();
        void                  resetExplicitFences();
        void                  setCTM(const Abyssutils::Math::Mat3x3& ctm);
        void                  setWideColorGamut(bool wcg);
        void                  setHDRMetadata(const hdr_output_metadata& metadata);
        void                  setContentType(const uint16_t drmContentType);
        void                  setColorRange(eOutputColorRange range);

      private:
        SInternalState           internalState;
        std::array<uint64_t, 16> propertyGenerations = {};
        uint64_t                 nextGeneration      = 0;

        void                     markCommitted(uint32_t properties);

        friend class IOutput;
        friend class CWaylandOutput;
        friend class CDRMOutput;
        friend class CDRMBackend;
        friend class CHeadlessOutput;
        friend struct SDRMConnector;
    };

    class IOutput {
      public:
        virtual ~IOutput();

        enum eOutputCommitCapabilities : uint32_t {
            ABYSSAL_OUTPUT_COMMIT_CAPABILITY_QUEUED      = (1 << 0), // Supports commitAsync.
            ABYSSAL_OUTPUT_COMMIT_CAPABILITY_TIMED       = (1 << 1), // Supports targetPresentation.
            ABYSSAL_OUTPUT_COMMIT_CAPABILITY_VRR         = (1 << 2), // Supports queued commits while adaptive sync is active.
            ABYSSAL_OUTPUT_COMMIT_CAPABILITY_TEARING     = (1 << 3), // Supports queued immediate-presentation commits.
            ABYSSAL_OUTPUT_COMMIT_CAPABILITY_LATE_CURSOR = (1 << 4), // Supports lateCursor.
        };

        enum eOutputCommitStatus : uint32_t {
            ABYSSAL_OUTPUT_COMMIT_SUBMITTED = 0,
            ABYSSAL_OUTPUT_COMMIT_FAILED,
            ABYSSAL_OUTPUT_COMMIT_CANCELLED,
        };

        // commitAsync snapshots pending state before returning. Requesting an option without its corresponding capability is rejected with ENOTSUP.
        struct SCommitOptions {
            // requires TIMED when set. The value is the desired presentation time; nullopt requests immediate submission.
            std::optional<std::chrono::steady_clock::time_point> targetPresentation;
            // requires LATE_CURSOR when set. The cursor position is the only state allowed to change after commitAsync returns.
            bool lateCursor = false;
        };

        // rejection leaves pending output state untouched. Pending VRR or tearing state requires the corresponding capability.
        struct SCommitSubmission {
            uint64_t id    = 0; // 0 means rejected.
            int      error = 0; // Positive errno, 0 on acceptance.
        };

        // every accepted id receives exactly one result after commitAsync returns, on the backend's dispatch thread. A submitted result precedes its presentation.
        // cancellation results are emitted before destroy.
        struct SCommitResult {
            uint64_t            id           = 0;
            eOutputCommitStatus status       = ABYSSAL_OUTPUT_COMMIT_FAILED;
            int                 error        = 0;     // positive errno, 0 when submitted.
            bool                missedTarget = false; // backend submission completed after targetPresentation. Valid only for timed, submitted commits.
        };

        enum scheduleFrameReason : uint32_t {
            ABYSSAL_SCHEDULE_UNKNOWN = 0,
            ABYSSAL_SCHEDULE_NEW_CONNECTOR,
            ABYSSAL_SCHEDULE_CURSOR_VISIBLE,
            ABYSSAL_SCHEDULE_CURSOR_SHAPE,
            ABYSSAL_SCHEDULE_CURSOR_MOVE,
            ABYSSAL_SCHEDULE_CLIENT_UNKNOWN,
            ABYSSAL_SCHEDULE_DAMAGE,
            ABYSSAL_SCHEDULE_NEW_MONITOR,
            ABYSSAL_SCHEDULE_RENDER_MONITOR,
            ABYSSAL_SCHEDULE_NEEDS_FRAME,
            ABYSSAL_SCHEDULE_ANIMATION,
            ABYSSAL_SCHEDULE_ANIMATION_DAMAGE,
        };

        struct SHDRMetadata {
            float desiredContentMaxLuminance      = 0;
            float desiredMaxFrameAverageLuminance = 0;
            float desiredContentMinLuminance      = 0;
            bool  supportsPQ                      = false;
        };

        struct xy {
            double x = 0;
            double y = 0;
        };

        struct SChromaticityCoords {
            xy red;
            xy green;
            xy blue;
            xy white;
        };

        struct SParsedEDID {
            std::string                        make, serial, model;
            std::optional<SHDRMetadata>        hdrMetadata;
            std::optional<SChromaticityCoords> chromaticityCoords;
            bool                               supportsBT2020 = false;
        };

        virtual bool                                                      commit()           = 0;
        virtual bool                                                      test()             = 0;
        virtual Abyssutils::Memory::CSharedPointer<IBackendImplementation> getBackend()       = 0;
        virtual std::vector<SDRMFormat>                                   getRenderFormats() = 0;
        virtual Abyssutils::Memory::CSharedPointer<SOutputMode>            preferredMode();
        virtual bool                                                      setCursor(Abyssutils::Memory::CSharedPointer<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot);
        virtual void                                                      moveCursor(const Abyssutils::Math::Vector2D& coord, bool skipSchedule = false); // includes the hotspot
        virtual void                                                      setCursorVisible(bool visible); // moving the cursor will make it visible again without this util
        virtual bool                                                      hasCursorPlane() const;
        virtual Abyssutils::Math::Vector2D                                 cursorPlaneSize(); // -1, -1 means no set size, 0, 0 means error
        virtual std::optional<std::chrono::steady_clock::time_point>      nextVBlank() const;
        virtual void                                                      scheduleFrame(const scheduleFrameReason reason = ABYSSAL_SCHEDULE_UNKNOWN);
        virtual size_t                                                    getGammaSize();
        virtual size_t                                                    getDeGammaSize();
        virtual bool                                                      destroy(); // not all backends allow this!!!
        virtual bool                                                      pendingPageFlip()  = 0;
        virtual bool                                                      pendingIdleFrame() = 0;
        virtual uint32_t                                                  commitCapabilities() const;
        virtual SCommitSubmission                                         commitAsync(const SCommitOptions& options);

        std::string                                                       name, description, make, model, serial;
        SParsedEDID                                                       parsedEDID;
        Abyssutils::Math::Vector2D                                         physicalSize;
        bool                                                              enabled    = false;
        bool                                                              nonDesktop = false;
        eSubpixelMode                                                     subpixel   = ABYSSAL_SUBPIXEL_NONE;
        bool                                                              vrrCapable = false, vrrActive = false;
        bool                                                              needsFrame       = false;
        bool                                                              supportsExplicit = false;

        //
        std::vector<Abyssutils::Memory::CSharedPointer<SOutputMode>> modes;
        Abyssutils::Memory::CSharedPointer<COutputState>             state = Abyssutils::Memory::makeShared<COutputState>();

        Abyssutils::Memory::CSharedPointer<CSwapchain>               swapchain;

        //

        enum eOutputPresentFlags : uint32_t {
            ABYSSAL_OUTPUT_PRESENT_VSYNC         = (1 << 0),
            ABYSSAL_OUTPUT_PRESENT_HW_CLOCK      = (1 << 1),
            ABYSSAL_OUTPUT_PRESENT_HW_COMPLETION = (1 << 2),
            ABYSSAL_OUTPUT_PRESENT_ZEROCOPY      = (1 << 3),
        };
        struct SStateEvent {
            Abyssutils::Math::Vector2D size; // if {0,0}, means it needs a reconfigure.
        };

        struct SPresentEvent {
            bool         presented = true;
            timespec*    when      = nullptr;
            unsigned int seq       = 0;
            int          refresh   = 0;
            uint32_t     flags     = 0;
            uint64_t     commitID  = 0; // 0 means the presentation is not associated with a queued commit.
        };

        struct {
            Abyssutils::Signal::CSignalT<>              destroy;
            Abyssutils::Signal::CSignalT<>              frame;
            Abyssutils::Signal::CSignalT<>              needsFrame;
            Abyssutils::Signal::CSignalT<SPresentEvent> present;
            Abyssutils::Signal::CSignalT<>              commit;
            Abyssutils::Signal::CSignalT<SStateEvent>   state;
            Abyssutils::Signal::CSignalT<SCommitResult> commitResult;
        } events;
    };
}
