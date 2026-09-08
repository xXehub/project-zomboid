#pragma once

#include <array>

namespace pz::feature_policy {

[[nodiscard]] constexpr std::array<float, 6> full_bright_values() noexcept
{
    return { 0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 1.0f };
}

[[nodiscard]] constexpr int anti_overload_weight() noexcept
{
    return 1'000'000;
}

} // namespace pz::feature_policy
