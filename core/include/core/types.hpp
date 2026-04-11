#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <string>

namespace safe::core
{

    // Common types used throughout the application
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using f32 = float;
    using f64 = double;

    // Application version
    struct Version
    {
        u32 major;
        u32 minor;
        u32 patch;

        [[nodiscard]] std::string ToString() const;
    };

    constexpr Version APP_VERSION = { 1, 0, 0 };

} // namespace safe::core

#endif
