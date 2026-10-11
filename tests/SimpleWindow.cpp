#include <abyssal/backend/Backend.hpp>
#include <abyssal/output/Output.hpp>
#include <abyssal/input/Input.hpp>
#include <iostream>
#include <wayland-server.h>

using namespace Abyssutils::Signal;
using namespace Abyssutils::Memory;
#define SP CSharedPointer

static const char* aqLevelToString(Abyssal::eBackendLogLevel level) {
    switch (level) {
        case Abyssal::eBackendLogLevel::ABYSSAL_LOG_TRACE: return "TRACE";
        case Abyssal::eBackendLogLevel::ABYSSAL_LOG_DEBUG: return "DEBUG";
        case Abyssal::eBackendLogLevel::ABYSSAL_LOG_ERROR: return "ERROR";
        case Abyssal::eBackendLogLevel::ABYSSAL_LOG_WARNING: return "WARNING";
        case Abyssal::eBackendLogLevel::ABYSSAL_LOG_CRITICAL: return "CRITICAL";
        default: break;
    }

    return "UNKNOWN";
}

void aqLog(Abyssal::eBackendLogLevel level, std::string msg) {
    std::cout << "[AQ] [" << aqLevelToString(level) << "] " << msg << "\n";
}

CAbyssSignalListener     newOutputListener, outputFrameListener, outputStateListener, mouseMotionListener, keyboardKeyListener, newMouseListener, newKeyboardListener;
SP<Abyssal::IOutput> output;

//
void onFrame() {
    std::cout << "[Client] onFrame\n";

    auto buf = output->swapchain->next(nullptr);

    output->state->setBuffer(buf);
    output->commit();
}

void onState(const Abyssal::IOutput::SStateEvent& event) {
    std::cout << "[Client] onState with size " << std::format("{}", event.size) << "\n";

    output->state->setEnabled(true);
    output->state->setCustomMode(makeShared<Abyssal::SOutputMode>(Abyssal::SOutputMode{.pixelSize = event.size}));
    output->state->setFormat(DRM_FORMAT_XRGB8888);

    output->commit();
}

int main(int argc, char** argv, char** envp) {
    Abyssal::SBackendOptions options;
    options.logFunction = aqLog;

    std::vector<Abyssal::SBackendImplementationOptions> implementations;
    Abyssal::SBackendImplementationOptions              waylandOptions;
    waylandOptions.backendType        = Abyssal::eBackendType::ABYSSAL_BACKEND_WAYLAND;
    waylandOptions.backendRequestMode = Abyssal::eBackendRequestMode::ABYSSAL_BACKEND_REQUEST_IF_AVAILABLE;
    implementations.emplace_back(waylandOptions);

    auto aqBackend = Abyssal::CBackend::create(implementations, options);

    newOutputListener = aqBackend->events.newOutput.listen([](const SP<Abyssal::IOutput> newOutput) {
        output = newOutput;

        std::cout << "[Client] Got a new output named " << output->name << "\n";

        outputFrameListener = output->events.frame.listen([] { onFrame(); });
        outputStateListener = output->events.state.listen([](const Abyssal::IOutput::SStateEvent& event) { onState(event); });
    });

    newMouseListener = aqBackend->events.newPointer.listen([](const SP<Abyssal::IPointer>& pointer) {
        mouseMotionListener = pointer->events.warp.listen(
            [](const Abyssal::IPointer::SWarpEvent& event) { std::cout << "[Client] Mouse warped to " << std::format("{}", event.absolute) << "\n"; });
    });

    newKeyboardListener = aqBackend->events.newKeyboard.listen([](const SP<Abyssal::IKeyboard>& keyboard) {
        keyboardKeyListener = keyboard->events.key.listen(
            [](const Abyssal::IKeyboard::SKeyEvent& event) { std::cout << "[Client] Key " << std::format("{}", event.key) << " state: " << event.pressed << " \n"; });
    });

    if (!aqBackend || !aqBackend->start()) {
        std::cout << "Failed to start the aq backend\n";
        return 1;
    }

    // FIXME: write an event loop.
    // aqBackend->enterLoop();

    return 0;
}
