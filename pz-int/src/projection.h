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

[[nodiscard]] constexpr float clamp_value(
    const float value, const float min_value, const float max_value) noexcept
{
    return value < min_value ? min_value :
        value > max_value ? max_value : value;
}

[[nodiscard]] constexpr float visual_zoom(const float zoom) noexcept
{
    return valid_zoom(zoom);
}

[[nodiscard]] constexpr screen_point from_exact(
    const float raw_x, const float raw_y, const float zoom) noexcept
{
    const float scale = valid_zoom(zoom);
    return { raw_x / scale, raw_y / scale };
}

[[nodiscard]] constexpr float smooth_toward(
    const float current, const float target, const float delta_seconds) noexcept
{
    const float blend = clamp_value(delta_seconds * 30.0f, 0.0f, 1.0f);
    return current + (target - current) * blend;
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
    const float scale = visual_zoom(zoom);
    const float height = kind == esp_kind::humanoid ? 80.0f / scale :
        kind == esp_kind::vehicle ? 72.0f / scale :
        kind == esp_kind::animal ? 50.0f / scale : 0.0f;
    const float half_width = kind == esp_kind::humanoid ? 16.0f / scale :
        kind == esp_kind::vehicle ? 58.0f / scale :
        kind == esp_kind::animal ? 24.0f / scale : 0.0f;
    return { x - half_width, y - height, x + half_width, y, y - height - 8.0f };
}

// The bottom edge of each text row stays eight screen pixels above its anchor.
[[nodiscard]] constexpr float label_above(const float anchor_y, const float text_height) noexcept
{
    return anchor_y - 8.0f - text_height;
}

// Text starts eight screen pixels below the rendered entity box.
[[nodiscard]] constexpr float label_below(const float anchor_y) noexcept
{
    return anchor_y + 8.0f;
}

[[nodiscard]] constexpr float health_fraction(
    const float health, const float max_health) noexcept
{
    return max_health > 0.0f
        ? clamp_value(health / max_health, 0.0f, 1.0f)
        : 0.0f;
}

} // namespace pz::projection
