#include "Shared.hpp"
#include <cstdlib>

bool Abyssal::envEnabled(const std::string& env) {
    auto e = getenv(env.c_str());
    return e && e == std::string{"1"};
}

bool Abyssal::envExplicitlyDisabled(const std::string& env) {
    auto e = getenv(env.c_str());
    return e && e == std::string{"0"};
}

static bool trace = []() -> bool { return Abyssal::envEnabled("ABYSSAL_TRACE"); }();

bool        Abyssal::isTrace() {
    return trace;
}
