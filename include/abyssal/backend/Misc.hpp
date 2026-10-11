#pragma once

#include <cstdint>
#include <vector>

namespace Abyssal {
    struct SGLFormat {
        uint32_t drmFormat = 0;
        uint64_t modifier  = 0;
        bool     external  = false;
    };

    struct SDRMFormat {
        uint32_t              drmFormat = 0; /* DRM_FORMAT_INVALID */
        std::vector<uint64_t> modifiers;
    };
};
