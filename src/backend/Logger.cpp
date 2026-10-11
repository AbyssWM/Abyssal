#include "Logger.hpp"
#include "../include/Shared.hpp"

using namespace Abyssal;

static Abyssutils::CLI::eLogLevel levelToHU(eBackendLogLevel l) {
    switch (l) {
        case Abyssal::ABYSSAL_LOG_DEBUG: return Abyssutils::CLI::LOG_DEBUG;
        case Abyssal::ABYSSAL_LOG_ERROR: return Abyssutils::CLI::LOG_ERR;
        case Abyssal::ABYSSAL_LOG_WARNING: return Abyssutils::CLI::LOG_WARN;
        case Abyssal::ABYSSAL_LOG_CRITICAL: return Abyssutils::CLI::LOG_CRIT;
        case Abyssal::ABYSSAL_LOG_TRACE: return Abyssutils::CLI::LOG_TRACE;
    }
    return Abyssutils::CLI::LOG_DEBUG;
}

CLogger::CLogger() = default;

void CLogger::updateLevels() {
    const auto IS_TRACE = Abyssal::isTrace();
    if (m_loggerConnection && IS_TRACE)
        m_loggerConnection->setLogLevel(Abyssutils::CLI::LOG_TRACE);
}

void CLogger::log(eBackendLogLevel level, const std::string& str) {
    if (m_logFn) {
        m_logFn(level, str);
        return;
    }

    if (m_loggerConnection) {
        m_loggerConnection->log(levelToHU(level), str);
        return;
    }
}
