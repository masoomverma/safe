#include "core/types.hpp"

namespace safe::core
{
    std::string Version::ToString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

} // namespace safa::core