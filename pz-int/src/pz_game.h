#pragma once
// ============================================================================
// pz_game.h - contract between the menu/overlay and the JNI game bridge.
//
// All game data comes through JNI calls into the Project Zomboid JVM
// (jvm.dll is loaded in-process by ProjectZomboid64.exe). The bridge
// attaches its own render thread to the JVM once and keeps the attach
// for its lifetime.
//
// Implementation notes are in pz_game.cpp. The menu must ONLY use this
// header - it never touches JNI directly.
// ============================================================================

#include <cstdint>
#include <cmath>
#include <vector>
#include <string>

namespace pz {

    // Entity classification for the ESP overlay.
    enum class entity_type : uint8_t {
        zombie,
        player,
        vehicle,
        animal,
        item
    };

    // One world entity collected per frame.
    struct entity {
        // world position (isometric tile coords)
        float wx{ 0.0f };
        float wy{ 0.0f };
        float wz{ 0.0f };
        // screen position (pixels, after IsoUtils.XToScreen/YToScreen)
        float sx{ 0.0f };
        float sy{ 0.0f };
        // true when the projected point lands inside the screen bounds
        bool on_screen{ false };
        // tile-space distance from the local player
        float dist{ 0.0f };
        // display name (UTF-8, from descriptor/username)
        char name[ 64 ]{};
        float health{ 0.0f };
        entity_type type{ entity_type::zombie };
        bool is_local{ false };
        // Smoothed world position used only for rendering. Camera projection
        // remains exact every frame while 30 Hz JNI snapshots are blended.
        float render_wx{ 0.0f };
        float render_wy{ 0.0f };
        float render_wz{ 0.0f };
        bool render_position_valid{ false };
    };

    struct aim_direction {
        float x{ 0.0f };
        float y{ 0.0f };
        bool valid{ false };
    };

    [[nodiscard]] inline aim_direction aim_direction_to(
        const float origin_x, const float origin_y,
        const float target_x, const float target_y) noexcept
    {
        const float dx = target_x - origin_x;
        const float dy = target_y - origin_y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (!(length > 0.0001f) || !std::isfinite(length)) return {};
        return { dx / length, dy / length, true };
    }

    // ---- lifecycle ----------------------------------------------------------

    // True once jvm.dll is loaded in-process AND a JavaVM was retrieved AND
    // the render thread is attached AND the first successful IsoPlayer
    // getInstance() round-trip happened (game fully in a world).
    bool game_ready( );
	// Release JNI global references and restore reversible toggles. Returns
	// false when restoration must be retried before the DLL can be released.
	bool shutdown( );

    // ---- per-frame data collection -----------------------------------------

    // Local player world position. Returns false if not in-game.
    bool local_player_pos( float& x, float& y, float& z );

	// Refresh the cached, render-visible entity list at a bounded cadence.
	// Calls between refreshes return immediately and preserve the last snapshot.
	void collect_entities( std::vector< entity >& out );

    // Zoom used by the cached frame projection; no JNI calls. Zero when invalid.
    float projection_zoom( ) noexcept;

    // ---- item database / spawning ------------------------------------------

    // Number of known item definitions (ScriptManager.instance.getAllItems()).
    int item_db_count( );

    // Iterate the item database. Returns nullptr when idx is out of range.
    // *out_full_type  - e.g. "Base.Axe" (the world-spawn API input)
    // *out_display    - human readable name for the list UI
    const char* item_db_get( int idx, const char** out_display );

    // One-click spawn on the local player's current ground square.
    bool spawn_item( const char* full_type );

    // Custom ground spawn: any item, chosen condition
    // (0-100, -1 = leave default), and ammo count for ranged weapons
    // (-1 = leave default). Client multiplayer builds refuse ground spawning
    // because installed Build 42 only transmits these overloads from GameServer.
    bool spawn_item_custom( const char* full_type, int condition, int ammo );

	struct survival_features {
		bool full_bright{ false };
		bool night_vision{ false };
		bool zombie_ignore{ false };
		bool god_mode{ false };
		bool anti_hunger{ false };
		bool unlimited_carry{ false };
		bool anti_thirst{ false };
		bool auto_heal{ false };
		bool infinite_ammo{ false };
		bool unlimited_endurance{ false };
		bool instant_actions{ false };
		bool aim_assist{ false };
		float aim_assist_max_dist{ 20.0f };
		bool perfect_accuracy{ false };
		bool always_critical{ false };
		bool one_hit{ false };
		bool anti_fatigue{ false };
		bool all_needs{ false };
		bool invisible{ false };
		bool noclip{ false };
		bool debug_bypass{ false };
	};

	// Applies transition-based world and combat overrides. The entity snapshot
	// drives per-frame aim assist; reversible holds run at 10 Hz.
	void apply_survival_features( const survival_features& features,
		const std::vector< entity >& entities );
    // Refill ammo of the currently held weapon to max once (menu button).
    // One-shot: set the local player's access level to "admin".
    void grant_admin( );

    // ---- debug info for the Debug tab --------------------------------------
    struct debug_info {
        bool jni_env_valid;
        bool class_loader_valid;
        bool resolved;
        int screen_w, screen_h, tile_scale, player_idx;
        float cam_off_x, cam_off_y, zoom;
        bool frame_ctx_valid;
        int zombie_count, player_count, vehicle_count, animal_count, item_count;
        bool isoutils_available;
        bool climate_method_api;   // true = Build 42 setOverride/setEnableOverride available
        bool climate_fields_ok;    // true = at least desaturation field resolved
        bool world_spawn_api_available; // true = Build 42 square spawn methods available, not delivery proof
        char last_spawn_result[128];
    };
    debug_info get_debug_info( );
    // Write a deep diagnostic dump to %TEMP%/pzint_dump.txt.
    // Tests every JNI method, reads live climate state, attempts a one-shot
    // fullbright toggle, and reports everything.
    void dump_deep_debug( const std::vector< entity >& entities );
    void refill_ammo( );

} // namespace pz
