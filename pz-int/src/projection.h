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
enum class character_pose { standing, crawling, floor, sitting };

struct esp_box {
    float left{};
    float top{};
    float right{};
    float bottom{};
    float label_y{};
};

[[nodiscard]] constexpr esp_box esp_box_for(
    const esp_kind kind, const character_pose pose,
    const float x, const float y, const float zoom) noexcept
{
    const float scale = visual_zoom(zoom);
    float height{};
    float half_width{};
    float bottom_offset{};

    if (kind == esp_kind::humanoid) {
        switch (pose) {
        case character_pose::crawling:
            height = 44.0f;
            half_width = 30.0f;
            bottom_offset = 6.0f;
            break;
        case character_pose::floor:
            height = 36.0f;
            half_width = 36.0f;
            bottom_offset = 10.0f;
            break;
        case character_pose::sitting:
            height = 66.0f;
            half_width = 23.0f;
            bottom_offset = 4.0f;
            break;
        case character_pose::standing:
            height = 105.0f;
            half_width = 18.0f;
            break;
        }
    } else if (kind == esp_kind::vehicle) {
        height = 72.0f;
        half_width = 58.0f;
    } else if (kind == esp_kind::animal) {
        height = 50.0f;
        half_width = 24.0f;
    }

    const float top = y - height / scale + bottom_offset / scale;
    const float bottom = y + bottom_offset / scale;
    return { x - half_width / scale, top, x + half_width / scale,
        bottom, top - 8.0f };
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
