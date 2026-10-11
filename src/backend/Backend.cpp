#include <abyssal/backend/Backend.hpp>
#include <abyssal/backend/Wayland.hpp>
#include <abyssal/backend/Headless.hpp>
#include <abyssal/backend/DRM.hpp>
#include <abyssal/backend/Null.hpp>
#include <abyssal/allocator/GBM.hpp>
#include <abyssutils/os/FileDescriptor.hpp>
#include <ranges>
#include <sys/timerfd.h>
#include <ctime>
#include <cstring>
#include <xf86drm.h>
#include <fcntl.h>
#include <unistd.h>

#include "Logger.hpp"

using namespace Abyssutils::Memory;
using namespace Abyssutils::OS;
using namespace Abyssal;
#define SP CSharedPointer

#define TIMESPEC_NSEC_PER_SEC 1000000000LL

static void timespecAddNs(timespec* pTimespec, int64_t delta) {
    int delta_ns_low = delta % TIMESPEC_NSEC_PER_SEC;
    int delta_s_high = delta / TIMESPEC_NSEC_PER_SEC;

    pTimespec->tv_sec += delta_s_high;

    pTimespec->tv_nsec += (long)delta_ns_low;
    if (pTimespec->tv_nsec >= TIMESPEC_NSEC_PER_SEC) {
        pTimespec->tv_nsec -= TIMESPEC_NSEC_PER_SEC;
        ++pTimespec->tv_sec;
    }
}

static const char* backendTypeToName(eBackendType type) {
    switch (type) {
        case ABYSSAL_BACKEND_DRM: return "drm";
        case ABYSSAL_BACKEND_HEADLESS: return "headless";
        case ABYSSAL_BACKEND_WAYLAND: return "wayland";
        default: break;
    }
    return "invalid";
}

Abyssal::CBackend::CBackend() {
    ;
}

Abyssal::SBackendImplementationOptions::SBackendImplementationOptions() : backendType(ABYSSAL_BACKEND_WAYLAND), backendRequestMode(ABYSSAL_BACKEND_REQUEST_IF_AVAILABLE) {
    ;
}

Abyssal::SBackendOptions::SBackendOptions() : logFunction(nullptr) {
    ;
}

Abyssutils::Memory::CSharedPointer<CBackend> Abyssal::CBackend::create(const std::vector<SBackendImplementationOptions>& backends, const SBackendOptions& options) {
    auto backend = SP<CBackend>(new CBackend());

    backend->options               = options;
    backend->logger                = Abyssutils::Memory::makeShared<CLogger>();
    backend->implementationOptions = backends;
    backend->self                  = backend;

    backend->logger->m_loggerConnection = options.logConnection;
    backend->logger->m_logFn            = options.logFunction;
    backend->logger->updateLevels();

    if (backends.size() <= 0)
        return nullptr;

    backend->log(ABYSSAL_LOG_DEBUG, "Creating an Abyssal backend!");

    for (auto const& b : backends) {
        if (b.backendType == ABYSSAL_BACKEND_WAYLAND) {
            auto ref = SP<CWaylandBackend>(new CWaylandBackend(backend));
            backend->implementations.emplace_back(ref);
            ref->self = ref;
        } else if (b.backendType == ABYSSAL_BACKEND_DRM) {
            auto ref = CDRMBackend::attempt(backend);
            if (ref.empty()) {
                backend->log(ABYSSAL_LOG_ERROR, "DRM Backend failed");
                continue;
            }

            for (auto const& r : ref) {
                backend->implementations.emplace_back(r);
            }
        } else if (b.backendType == ABYSSAL_BACKEND_HEADLESS) {
            auto ref = SP<CHeadlessBackend>(new CHeadlessBackend(backend));
            backend->implementations.emplace_back(ref);
            ref->self = ref;
        } else if (b.backendType == ABYSSAL_BACKEND_NULL) {
            auto ref = SP<CNullBackend>(new CNullBackend(backend));
            backend->implementations.emplace_back(ref);
            ref->self = ref;
        } else {
            backend->log(ABYSSAL_LOG_ERROR, std::format("Unknown backend id: {}", (int)b.backendType));
            continue;
        }
    }

    // create a timerfd for idle events
    backend->idle.fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

    return backend;
}

Abyssal::CBackend::~CBackend() {
    if (idle.fd >= 0)
        close(idle.fd);

    // Tear down implementations before the logger is destroyed,
    // as backends may log during teardown (e.g. SDRMConnector::disconnect).
    implementations.clear();
}

bool Abyssal::CBackend::start() {
    log(ABYSSAL_LOG_DEBUG, "Starting the Abyssal backend!");

    int  started = 0;

    auto optionsForType = [this](eBackendType type) -> SBackendImplementationOptions {
        for (auto const& o : implementationOptions) {
            if (o.backendType == type)
                return o;
        }
        return SBackendImplementationOptions{};
    };

    for (size_t i = 0; i < implementations.size(); ++i) {
        const auto& impl = implementations.at(i);
        const bool  ok   = impl->start();

        if (!ok) {
            log(ABYSSAL_LOG_ERROR, std::format("Requested backend ({}) could not start, enabling fallbacks", backendTypeToName(impl->type())));
            if (optionsForType(impl->type()).backendRequestMode == ABYSSAL_BACKEND_REQUEST_MANDATORY) {
                log(ABYSSAL_LOG_CRITICAL, std::format("Requested backend ({}) could not start and it's mandatory, cannot continue!", backendTypeToName(impl->type())));
                implementations.clear();
                return false;
            }
        } else
            started++;
    }

    if (implementations.empty() || started <= 0) {
        log(ABYSSAL_LOG_CRITICAL, "No backend could be opened. Make sure there was a correct backend passed to CBackend, and that your environment supports at least one of them.");
        return false;
    }

    // erase failed impls
    std::erase_if(implementations, [this](const auto& i) {
        bool failed = i->pollFDs().empty() && i->type() != ABYSSAL_BACKEND_NULL;
        if (failed)
            log(ABYSSAL_LOG_ERROR, std::format("Implementation {} failed, erasing.", backendTypeToName(i->type())));
        return failed;
    });

    // TODO: obviously change this when (if) we add different allocators.
    for (auto const& b : implementations) {
        if (b->drmFD() >= 0) {
            auto fd = reopenDRMNode(b->drmFD());
            if (fd < 0) {
                // this is critical, we cannot create an allocator properly
                log(ABYSSAL_LOG_CRITICAL, "Failed to create an allocator (reopenDRMNode failed)");
                return false;
            }
            primaryAllocator = CGBMAllocator::create(fd, self);
            break;
        }
    }

    if (!primaryAllocator && (implementations.empty() || implementations.at(0)->type() != ABYSSAL_BACKEND_NULL)) {
        log(ABYSSAL_LOG_CRITICAL, "Cannot open backend: no allocator available");
        return false;
    }

    ready = true;
    for (auto const& b : implementations) {
        b->onReady();
    }

    if (session)
        session->onReady();

    sessionFDs = session ? session->pollFDs() : std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>>{};

    return true;
}

void Abyssal::CBackend::log(eBackendLogLevel level, const std::string& msg) {
    if (logger)
        logger->log(level, msg);
}

std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>> Abyssal::CBackend::getPollFDs() {
    std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>> result;
    for (auto const& i : implementations) {
        auto pollfds = i->pollFDs();
        for (auto const& p : pollfds) {
            log(ABYSSAL_LOG_DEBUG, std::format("backend: poll fd {} for implementation {}", p->fd, backendTypeToName(i->type())));
            result.emplace_back(p);
        }
    }

    for (auto const& sfd : sessionFDs) {
        log(ABYSSAL_LOG_DEBUG, std::format("backend: poll fd {} for session", sfd->fd));
        result.emplace_back(sfd);
    }

    log(ABYSSAL_LOG_DEBUG, std::format("backend: poll fd {} for idle", idle.fd));
    result.emplace_back(makeShared<SPollFD>(idle.fd, [this]() { dispatchIdle(); }));

    return result;
}

int Abyssal::CBackend::drmFD() {
    for (auto const& i : implementations) {
        int fd = i->drmFD();
        if (fd < 0)
            continue;

        return fd;
    }
    return -1;
}

int Abyssal::CBackend::drmRenderNodeFD() {
    for (auto const& i : implementations) {
        int fd = i->drmRenderNodeFD();
        if (fd < 0)
            continue;

        return fd;
    }
    return -1;
}

bool Abyssal::CBackend::hasSession() {
    return !!session;
}

std::vector<SDRMFormat> Abyssal::CBackend::getPrimaryRenderFormats() {
    for (auto const& b : implementations) {
        if (b->type() != ABYSSAL_BACKEND_DRM && b->type() != ABYSSAL_BACKEND_WAYLAND)
            continue;

        return b->getRenderFormats();
    }

    for (auto const& b : implementations) {
        return b->getRenderFormats();
    }

    return {};
}

const std::vector<SP<IBackendImplementation>>& Abyssal::CBackend::getImplementations() {
    return implementations;
}

void Abyssal::CBackend::addIdleEvent(SP<std::function<void(void)>> fn) {
    auto r = idle.pending.emplace_back(fn);

    updateIdleTimer();
}

void Abyssal::CBackend::updateIdleTimer() {
    uint64_t ADD_NS = idle.pending.empty() ? TIMESPEC_NSEC_PER_SEC * 240ULL /* 240s, 4 mins */ : 0;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespecAddNs(&now, ADD_NS);

    itimerspec ts = {.it_value = now};

    if (timerfd_settime(idle.fd, TFD_TIMER_ABSTIME, &ts, nullptr))
        log(ABYSSAL_LOG_ERROR, std::format("backend: failed to arm timerfd: {}", strerror(errno)));
}

void Abyssal::CBackend::removeIdleEvent(SP<std::function<void(void)>> pfn) {
    std::erase(idle.pending, pfn);
}

void Abyssal::CBackend::dispatchIdle() {
    if (!CFileDescriptor::isReadable(idle.fd))
        log(ABYSSAL_LOG_ERROR, std::format("dispatchIdle: dispatched an non readable idle event on fd : {}", idle.fd));
    else {
        uint64_t expirations;
        read(idle.fd, &expirations, sizeof(expirations));
    }

    auto cpy = idle.pending;
    idle.pending.clear();

    for (auto const& i : cpy) {
        if (i && *i)
            (*i)();
    }

    updateIdleTimer();
}

void Abyssal::CBackend::onNewGpu(std::string path) {
    const auto primary    = std::ranges::find_if(implementations, [](SP<IBackendImplementation> value) { return value->type() == Abyssal::ABYSSAL_BACKEND_DRM; });
    const auto primaryDrm = primary != implementations.end() ? ((Abyssal::CDRMBackend*)(*primary).get())->self.lock() : nullptr;

    auto       ref = CDRMBackend::fromGpu(path, self.lock(), primaryDrm);
    if (!ref) {
        log(ABYSSAL_LOG_ERROR, std::format("DRM Backend failed for device {}", path));
        return;
    }
    if (!ref->start()) {
        log(ABYSSAL_LOG_ERROR, std::format("Couldn't start DRM Backend for device {}", path));
        return;
    }

    implementations.emplace_back(ref);
    events.pollFDsChanged.emit();

    ref->onReady();        // Renderer created here
    ref->recheckOutputs(); // Now we can recheck outputs
}

// Yoinked from wlroots, render/allocator/allocator.c
// Ref-counting reasons, see https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
int Abyssal::CBackend::reopenDRMNode(int drmFD, bool allowRenderNode) {

    if (drmIsMaster(drmFD)) {
        // Only recent kernels support empty leases
        uint32_t lesseeID = 0;
        int      leaseFD  = drmModeCreateLease(drmFD, nullptr, 0, O_CLOEXEC, &lesseeID);
        if (leaseFD >= 0) {
            return leaseFD;
        } else if (leaseFD != -EINVAL && leaseFD != -EOPNOTSUPP) {
            log(ABYSSAL_LOG_ERROR, "reopenDRMNode: drmModeCreateLease failed");
            return -1;
        }
        log(ABYSSAL_LOG_DEBUG, "reopenDRMNode: drmModeCreateLease failed, falling back to open");
    }

    char* name = nullptr;
    if (allowRenderNode)
        name = drmGetRenderDeviceNameFromFd(drmFD);

    if (!name) {
        // primary node or no name
        name = drmGetDeviceNameFromFd2(drmFD);

        if (!name) {
            log(ABYSSAL_LOG_ERROR, "reopenDRMNode: drmGetDeviceNameFromFd2 failed");
            return -1;
        }
    }

    log(ABYSSAL_LOG_DEBUG, std::format("reopenDRMNode: opening node {}", name));

    int newFD = open(name, O_RDWR | O_CLOEXEC);
    if (newFD < 0) {
        log(ABYSSAL_LOG_ERROR, std::format("reopenDRMNode: failed to open node {}", name));
        free(name);
        return -1;
    }

    free(name);

    // We need to authenticate if we are using a DRM primary node and are the master
    if (drmIsMaster(drmFD) && drmGetNodeTypeFromFd(newFD) == DRM_NODE_PRIMARY) {
        drm_magic_t magic;
        if (int ret = drmGetMagic(newFD, &magic); ret < 0) {
            log(ABYSSAL_LOG_ERROR, std::format("reopenDRMNode: drmGetMagic failed: {}", strerror(-ret)));
            close(newFD);
            return -1;
        }

        if (int ret = drmAuthMagic(drmFD, magic); ret < 0) {
            log(ABYSSAL_LOG_ERROR, std::format("reopenDRMNode: drmAuthMagic failed: {}", strerror(-ret)));
            close(newFD);
            return -1;
        }
    }

    return newFD;
}

std::vector<SDRMFormat> Abyssal::IBackendImplementation::getRenderableFormats() {
    return {};
}

eBackendGPUDriver Abyssal::IBackendImplementation::gpuDriver() {
    return ABYSSAL_BACKEND_GPU_DRIVER_UNKNOWN;
}
