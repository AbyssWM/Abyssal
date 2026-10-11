#include <abyssal/backend/Headless.hpp>
#include <fcntl.h>
#include <algorithm>
#include <ctime>
#include <sys/timerfd.h>
#include <cstring>
#include <unistd.h>
#include "Shared.hpp"

using namespace Abyssal;
using namespace Abyssutils::Memory;
using namespace Abyssutils::Math;
#define SP CSharedPointer

#define TIMESPEC_NSEC_PER_SEC 1000000000LL

Abyssal::CHeadlessOutput::CHeadlessOutput(const std::string& name_, Abyssutils::Memory::CWeakPointer<CHeadlessBackend> backend_) : backend(backend_) {
    name = name_;

    lastFrame = std::chrono::steady_clock::now();
}

Abyssal::CHeadlessOutput::~CHeadlessOutput() {
    events.destroy.emit();
}

bool Abyssal::CHeadlessOutput::commit() {
    const auto snapshot = state->snapshot();
    events.commit.emit();
    state->consume(snapshot);
    needsFrame = false;
    events.present.emit(IOutput::SPresentEvent{.presented = true});
    return true;
}

bool Abyssal::CHeadlessOutput::test() {
    return true;
}

std::vector<SDRMFormat> Abyssal::CHeadlessOutput::getRenderFormats() {
    return backend->getRenderFormats();
}

bool Abyssal::CHeadlessOutput::pendingPageFlip() {
    return false;
}

bool Abyssal::CHeadlessOutput::pendingIdleFrame() {
    return frameScheduled;
}

Abyssutils::Memory::CSharedPointer<IBackendImplementation> Abyssal::CHeadlessOutput::getBackend() {
    return backend.lock();
}

void Abyssal::CHeadlessOutput::scheduleFrame(const scheduleFrameReason reason) {
    TRACE(backend->backend->log(ABYSSAL_LOG_TRACE,
                                std::format("CHeadlessOutput::scheduleFrame: reason {}, needsFrame {}, frameScheduled {}", (uint32_t)reason, needsFrame, frameScheduled)));

    needsFrame = true;

    if (!framecb) {
        framecb = makeShared<std::function<void()>>([this, weak = self]() {
            if (!weak)
                return;

            frameScheduled = false;
            // not sure about removing this since framecb might be called outside of scheduleFrames no?!
            lastFrame = std::chrono::steady_clock::now();
            events.frame.emit();
        });
    }

    if (frameScheduled)
        return;

    frameScheduled = true;

    // schedule next frame when it should occur with a timer
    int64_t refreshRatemHz = 60000;
    auto&   currentState   = state->state();

    if (currentState.mode)
        refreshRatemHz = currentState.mode->refreshRate;
    else if (currentState.customMode)
        refreshRatemHz = currentState.customMode->refreshRate;

    const auto FRAME_INTERVAL  = std::chrono::nanoseconds(1000LL * TIMESPEC_NSEC_PER_SEC / refreshRatemHz);
    const auto NEXT_FRAME_TIME = lastFrame + FRAME_INTERVAL;

    if (std::chrono::steady_clock::now() >= NEXT_FRAME_TIME) {
        backend->backend->addIdleEvent(framecb);
        return;
    }

    backend->addTimer(NEXT_FRAME_TIME, [this, weak = self, NEXT_FRAME_TIME]() {
        if (!weak)
            return;

        if (framecb && *framecb)
            (*framecb)();

        lastFrame = NEXT_FRAME_TIME;
    });
}

bool Abyssal::CHeadlessOutput::destroy() {
    events.destroy.emit();
    std::erase(backend->outputs, self.lock());
    return true;
}

Abyssal::CHeadlessBackend::~CHeadlessBackend() {
    ;
}

Abyssal::CHeadlessBackend::CHeadlessBackend(SP<CBackend> backend_) : backend(backend_) {
    timers.timerfd = Abyssutils::OS::CFileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC)};
}

eBackendType Abyssal::CHeadlessBackend::type() {
    return eBackendType::ABYSSAL_BACKEND_HEADLESS;
}

bool Abyssal::CHeadlessBackend::start() {
    return true;
}

std::vector<SP<SPollFD>> Abyssal::CHeadlessBackend::pollFDs() {
    return {makeShared<SPollFD>(timers.timerfd.get(), [this]() { dispatchTimers(); })};
}

int Abyssal::CHeadlessBackend::drmFD() {
    return -1;
}

int Abyssal::CHeadlessBackend::drmRenderNodeFD() {
    return -1;
}

bool Abyssal::CHeadlessBackend::dispatchEvents() {
    return true;
}

uint32_t Abyssal::CHeadlessBackend::capabilities() {
    return 0;
}

bool Abyssal::CHeadlessBackend::setCursor(SP<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot) {
    return false;
}

void Abyssal::CHeadlessBackend::onReady() {
    ;
}

std::vector<SDRMFormat> Abyssal::CHeadlessBackend::getRenderFormats() {
    for (const auto& impl : backend->getImplementations()) {
        if (impl->type() != ABYSSAL_BACKEND_DRM || impl->getRenderableFormats().empty())
            continue;
        return impl->getRenderableFormats();
    }

    // formats probably supported by EGL
    return {SDRMFormat{.drmFormat = DRM_FORMAT_XRGB8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_XBGR8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_RGBX8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_BGRX8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_ARGB8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_ABGR8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_RGBA8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_BGRA8888, .modifiers = {DRM_FORMAT_INVALID}},
            SDRMFormat{.drmFormat = DRM_FORMAT_XRGB2101010, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_XBGR2101010, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_RGBX1010102, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_BGRX1010102, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_ARGB2101010, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_ABGR2101010, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_RGBA1010102, .modifiers = {DRM_FORMAT_MOD_LINEAR}},
            SDRMFormat{.drmFormat = DRM_FORMAT_BGRA1010102, .modifiers = {DRM_FORMAT_MOD_LINEAR}}};
}

std::vector<SDRMFormat> Abyssal::CHeadlessBackend::getCursorFormats() {
    return {}; // No cursor support
}

bool Abyssal::CHeadlessBackend::createOutput(const std::string& name) {
    std::string outputName = name;
    if (outputName.empty()) {
        // skip past any auto-name slots already claimed by an explicit createOutput,
        // so we never hand out a duplicate HEADLESS-N (see #185).
        do {
            outputName = std::format("HEADLESS-{}", ++outputIDCounter);
        } while (std::ranges::any_of(outputs, [&outputName](const auto& o) { return o->name == outputName; }));
    } else if (std::ranges::any_of(outputs, [&outputName](const auto& o) { return o->name == outputName; })) {
        backend->log(ABYSSAL_LOG_ERROR, std::format("headless: refusing to create output {}, name already in use", outputName));
        return false;
    }

    auto output = SP<CHeadlessOutput>(new CHeadlessOutput(outputName, self.lock()));
    outputs.emplace_back(output);
    output->modes.emplace_back(SP<SOutputMode>(new SOutputMode(Vector2D{1920, 1080}, 60000, true)));
    output->swapchain = CSwapchain::create(backend->primaryAllocator, self.lock());
    output->self      = output;
    backend->events.newOutput.emit(SP<IOutput>(output));

    return true;
}

void Abyssal::CHeadlessBackend::dispatchTimers() {
    uint64_t expirations;
    if (timers.timerfd.isValid() && timers.timerfd.isReadable() && read(timers.timerfd.get(), &expirations, sizeof(uint64_t)) < 0) {
        backend->log(ABYSSAL_LOG_ERROR, std::format("headless: failed to read timerfd: {}", strerror(errno)));
        return;
    }

    auto                it = std::stable_partition(timers.timers.begin(), timers.timers.end(), [](CTimer& t) { return !t.expired(); });

    std::vector<CTimer> toFire(std::make_move_iterator(it), std::make_move_iterator(timers.timers.end()));
    timers.timers.erase(it, timers.timers.end());

    for (auto& timer : toFire) {
        if (timer.what)
            timer.what();
    }

    updateTimerFD();
}

void Abyssal::CHeadlessBackend::updateTimerFD() {
    auto soonestTimer = std::chrono::steady_clock::now() + std::chrono::minutes(4);

    for (auto const& t : timers.timers) {
        if (t.when < soonestTimer)
            soonestTimer = t.when;
    }

    auto       secs = std::chrono::time_point_cast<std::chrono::seconds>(soonestTimer);
    auto       ns   = std::chrono::time_point_cast<std::chrono::nanoseconds>(soonestTimer) - std::chrono::time_point_cast<std::chrono::nanoseconds>(secs);
    itimerspec ts   = {.it_value = {secs.time_since_epoch().count(), ns.count()}};

    if (timerfd_settime(timers.timerfd.get(), TFD_TIMER_ABSTIME, &ts, nullptr))
        backend->log(ABYSSAL_LOG_ERROR, std::format("headless: failed to arm timerfd: {}", strerror(errno)));
}

void Abyssal::CHeadlessBackend::addTimer(std::chrono::steady_clock::time_point when, std::function<void(void)> what) {
    timers.timers.push_back(CTimer{.when = when, .what = what});
    updateTimerFD();
}

SP<IAllocator> Abyssal::CHeadlessBackend::preferredAllocator() {
    return backend->primaryAllocator;
}

std::vector<SP<IAllocator>> Abyssal::CHeadlessBackend::getAllocators() {
    return {backend->primaryAllocator};
}

Abyssutils::Memory::CWeakPointer<IBackendImplementation> Abyssal::CHeadlessBackend::getPrimary() {
    return {};
}

bool Abyssal::CHeadlessBackend::CTimer::expired() {
    return std::chrono::steady_clock::now() > when;
}
