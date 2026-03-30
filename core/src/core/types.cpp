#include "core/include/core/types.hpp"

namespace safe {
namespace core {

std::string Version::ToString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

} // namespace core
} // namespace safe
