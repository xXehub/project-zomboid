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
#include <vector>
#include <string>

namespace pz {

    // One world entity (zombie or player) collected per frame.
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
        bool is_zombie{ false };
        bool is_local{ false };
    };

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

    // ---- item database / spawning ------------------------------------------

    // Number of known item definitions (ScriptManager.instance.getAllItems()).
    int item_db_count( );

    // Iterate the item database. Returns nullptr when idx is out of range.
    // *out_full_type  - e.g. "Base.Axe" (the string AddItem wants)
    // *out_display    - human readable name for the list UI
    const char* item_db_get( int idx, const char** out_display );

    // One-click spawn into the local player's inventory.
    bool spawn_item( const char* full_type );

    // Custom spawn: any item, chosen condition (0-100, -1 = leave default),
    // and ammo count for ranged weapons (-1 = leave default). Goes through
    // InventoryItemFactory.CreateItem + setCondition + ammo setters and then
    // hands the item to the inventory.
    bool spawn_item_custom( const char* full_type, int condition, int ammo );

	struct survival_features {
		bool full_bright{ false };
		bool zombie_ignore{ false };
		bool god_mode{ false };
		bool anti_hunger{ false };
		bool anti_encumbrance{ false };
		bool anti_thirst{ false };
		bool auto_heal{ false };
	};

	// Applies transition-based world overrides and a 10 Hz local-player hold.
	// Reversible fields are restored when their toggle is disabled.
	void apply_survival_features( const survival_features& features );
    // Refill ammo of the currently held weapon to max once (menu button).
    void refill_ammo( );

} // namespace pz
