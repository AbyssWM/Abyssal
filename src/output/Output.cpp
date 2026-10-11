#include <abyssal/output/Output.hpp>
#include "Shared.hpp"

#include <bit>
#include <cerrno>
#include <fcntl.h>

using namespace Abyssal;

Abyssal::IOutput::~IOutput() {
    events.destroy.emit();
}

Abyssutils::Memory::CSharedPointer<SOutputMode> Abyssal::IOutput::preferredMode() {
    for (auto const& m : modes) {
        if (m->preferred)
            return m;
    }

    return nullptr;
}

uint32_t Abyssal::IOutput::commitCapabilities() const {
    return 0;
}

Abyssal::IOutput::SCommitSubmission Abyssal::IOutput::commitAsync(const SCommitOptions& options) {
    return {
        .error = ENOTSUP,
    };
}

void Abyssal::IOutput::moveCursor(const Abyssutils::Math::Vector2D& coord, bool skipSchedule) {
    ;
}

bool Abyssal::IOutput::setCursor(Abyssutils::Memory::CSharedPointer<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot) {
    return false;
}

void Abyssal::IOutput::setCursorVisible(bool visible) {
    ;
}

bool Abyssal::IOutput::hasCursorPlane() const {
    return false;
}

void Abyssal::IOutput::scheduleFrame(const scheduleFrameReason reason) {
    ;
}

Abyssutils::Math::Vector2D Abyssal::IOutput::cursorPlaneSize() {
    return {}; // error
}

std::optional<std::chrono::steady_clock::time_point> Abyssal::IOutput::nextVBlank() const {
    return std::nullopt;
}

size_t Abyssal::IOutput::getGammaSize() {
    return 0;
}

size_t Abyssal::IOutput::getDeGammaSize() {
    return 0;
}

bool Abyssal::IOutput::destroy() {
    return false;
}

Abyssal::COutputState::CSnapshot::CSnapshot(const COutputState* owner, const SInternalState& state, const std::array<uint64_t, 16>& generations) :
    m_owner(owner), m_state(state), m_generations(generations) {
    if (!(m_state.committed & ABYSSAL_OUTPUT_STATE_EXPLICIT_IN_FENCE) || m_state.explicitInFence < 0)
        return;

    const int DUPLICATED_FD = fcntl(m_state.explicitInFence, F_DUPFD_CLOEXEC, 0);
    if (DUPLICATED_FD < 0) {
        m_error                 = errno;
        m_state.explicitInFence = -1;
        return;
    }

    m_explicitInFence       = Abyssutils::OS::CFileDescriptor{DUPLICATED_FD};
    m_state.explicitInFence = m_explicitInFence.get();
}

const Abyssal::COutputState::SInternalState& Abyssal::COutputState::CSnapshot::state() const {
    return m_state;
}

bool Abyssal::COutputState::CSnapshot::needsReconfig() const {
    return m_state.committed & (ABYSSAL_OUTPUT_STATE_ENABLED | ABYSSAL_OUTPUT_STATE_FORMAT | ABYSSAL_OUTPUT_STATE_MODE | ABYSSAL_OUTPUT_STATE_HDR | ABYSSAL_OUTPUT_STATE_WCG);
}

int Abyssal::COutputState::CSnapshot::error() const {
    return m_error;
}

const Abyssal::COutputState::SInternalState& Abyssal::COutputState::state() {
    return internalState;
}

const Abyssal::COutputState::SInternalState& Abyssal::COutputState::state() const {
    return internalState;
}

Abyssal::COutputState::CSnapshot Abyssal::COutputState::snapshot() const {
    return CSnapshot{this, internalState, propertyGenerations};
}

void Abyssal::COutputState::consume(const CSnapshot& snapshot) {
    ASSERT(snapshot.m_owner == this);
    if (snapshot.m_owner != this)
        return;

    uint32_t properties = snapshot.m_state.committed;

    while (properties) {
        const uint32_t property = 1U << std::countr_zero(properties);
        const size_t   index    = std::countr_zero(property);
        properties &= ~property;

        if (!(internalState.committed & property) || propertyGenerations.at(index) != snapshot.m_generations.at(index))
            continue;

        internalState.committed &= ~property;
        if (property == ABYSSAL_OUTPUT_STATE_DAMAGE)
            internalState.damage.clear();
    }
}

void Abyssal::COutputState::rearm(const CSnapshot& snapshot) {
    ASSERT(snapshot.m_owner == this);
    if (snapshot.m_owner != this)
        return;

    uint32_t properties = snapshot.m_state.committed & ~(ABYSSAL_OUTPUT_STATE_EXPLICIT_IN_FENCE | ABYSSAL_OUTPUT_STATE_EXPLICIT_OUT_FENCE);
    while (properties) {
        const uint32_t property = 1U << std::countr_zero(properties);
        const size_t   index    = std::countr_zero(property);
        properties &= ~property;

        if (propertyGenerations.at(index) != snapshot.m_generations.at(index))
            continue;

        internalState.committed |= property;
        if (property == ABYSSAL_OUTPUT_STATE_DAMAGE)
            internalState.damage.add(snapshot.m_state.damage);
    }
}

bool Abyssal::COutputState::needsReconfig() const {
    return internalState.committed & (ABYSSAL_OUTPUT_STATE_ENABLED | ABYSSAL_OUTPUT_STATE_FORMAT | ABYSSAL_OUTPUT_STATE_MODE | ABYSSAL_OUTPUT_STATE_HDR | ABYSSAL_OUTPUT_STATE_WCG);
}

void Abyssal::COutputState::addDamage(const Abyssutils::Math::CRegion& region) {
    internalState.damage.add(region);
    markCommitted(ABYSSAL_OUTPUT_STATE_DAMAGE);
}

void Abyssal::COutputState::clearDamage() {
    internalState.damage.clear();
    markCommitted(ABYSSAL_OUTPUT_STATE_DAMAGE);
}

void Abyssal::COutputState::setEnabled(bool enabled) {
    internalState.enabled = enabled;
    markCommitted(ABYSSAL_OUTPUT_STATE_ENABLED);
}

void Abyssal::COutputState::setAdaptiveSync(bool enabled) {
    internalState.adaptiveSync = enabled;
    markCommitted(ABYSSAL_OUTPUT_STATE_ADAPTIVE_SYNC);
}

void Abyssal::COutputState::setPresentationMode(eOutputPresentationMode mode) {
    internalState.presentationMode = mode;
    markCommitted(ABYSSAL_OUTPUT_STATE_PRESENTATION_MODE);
}

void Abyssal::COutputState::setGammaLut(const std::vector<uint16_t>& lut) {
    internalState.gammaLut = lut;
    markCommitted(ABYSSAL_OUTPUT_STATE_GAMMA_LUT);
}

void Abyssal::COutputState::setDeGammaLut(const std::vector<uint16_t>& lut) {
    internalState.degammaLut = lut;
    markCommitted(ABYSSAL_OUTPUT_STATE_DEGAMMA_LUT);
}

void Abyssal::COutputState::setMode(Abyssutils::Memory::CSharedPointer<SOutputMode> mode) {
    internalState.mode       = mode;
    internalState.customMode = nullptr;
    markCommitted(ABYSSAL_OUTPUT_STATE_MODE);
}

void Abyssal::COutputState::setCustomMode(Abyssutils::Memory::CSharedPointer<SOutputMode> mode) {
    internalState.mode.reset();
    internalState.customMode = mode;
    markCommitted(ABYSSAL_OUTPUT_STATE_MODE);
}

void Abyssal::COutputState::setFormat(uint32_t drmFormat) {
    internalState.drmFormat = drmFormat;
    markCommitted(ABYSSAL_OUTPUT_STATE_FORMAT);
}

void Abyssal::COutputState::setBuffer(Abyssutils::Memory::CSharedPointer<IBuffer> buffer) {
    internalState.buffer = buffer;
    markCommitted(ABYSSAL_OUTPUT_STATE_BUFFER);
}

void Abyssal::COutputState::setExplicitInFence(int32_t fenceFD) {
    internalState.explicitInFence = fenceFD;
    markCommitted(ABYSSAL_OUTPUT_STATE_EXPLICIT_IN_FENCE);
}

void Abyssal::COutputState::enableExplicitOutFenceForNextCommit() {
    markCommitted(ABYSSAL_OUTPUT_STATE_EXPLICIT_OUT_FENCE);
}

void Abyssal::COutputState::resetExplicitFences() {
    // fences are now used, let's reset them to not confuse ourselves later.
    internalState.explicitInFence  = -1;
    internalState.explicitOutFence = -1;
}

void Abyssal::COutputState::setCTM(const Abyssutils::Math::Mat3x3& ctm) {
    internalState.ctm = ctm;
    markCommitted(ABYSSAL_OUTPUT_STATE_CTM);
}

void Abyssal::COutputState::setWideColorGamut(bool wcg) {
    internalState.wideColorGamut = wcg;
    markCommitted(ABYSSAL_OUTPUT_STATE_WCG);
}

void Abyssal::COutputState::setHDRMetadata(const hdr_output_metadata& metadata) {
    internalState.hdrMetadata = metadata;
    markCommitted(ABYSSAL_OUTPUT_STATE_HDR);
}

void Abyssal::COutputState::setContentType(const uint16_t drmContentType) {
    internalState.contentType = drmContentType;
}

void Abyssal::COutputState::setColorRange(eOutputColorRange range) {
    internalState.colorRange = range;
}

void Abyssal::COutputState::markCommitted(uint32_t properties) {
    internalState.committed |= properties;

    while (properties) {
        const uint32_t property = 1U << std::countr_zero(properties);
        properties &= ~property;
        propertyGenerations.at(std::countr_zero(property)) = ++nextGeneration;
    }
}
