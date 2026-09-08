#pragma once

#include <limits>

namespace pz::projection {

struct screen_point {
    float x{};
    float y{};
};

[[nodiscard]] constexpr float valid_zoom(const float zoom) noexcept
{
    return zoom > 0.0f && zoom <= std::numeric_limits<float>::max() ? zoom : 1.0f;
}

[[nodiscard]] constexpr screen_point from_exact(
    const float raw_x, const float raw_y, const float zoom) noexcept
{
    const float scale = valid_zoom(zoom);
    return { raw_x / scale, raw_y / scale };
}

enum class esp_kind { humanoid, vehicle, animal, item };

struct esp_box {
    float left{};
    float top{};
    float right{};
    float bottom{};
    float label_y{};
};

[[nodiscard]] constexpr esp_box esp_box_for(
    const esp_kind kind, const float x, const float y, const float zoom) noexcept
{
    const float scale = valid_zoom(zoom);
    const float half_width = 14.0f / scale;
    const float height = (kind == esp_kind::humanoid ? 56.0f :
        kind == esp_kind::vehicle ? 40.0f : kind == esp_kind::animal ? 36.0f : 0.0f) / scale;
    return { x - half_width, y - height, x + half_width, y, y - height - 8.0f };
}

// The bottom edge of each text row stays eight screen pixels above its anchor.
[[nodiscard]] constexpr float label_above(const float anchor_y, const float text_height) noexcept
{
    return anchor_y - 8.0f - text_height;
}

} // namespace pz::projection
