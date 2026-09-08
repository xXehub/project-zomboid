#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include "pz_game.h"

class Menu {
public:
	static Menu& Get() { static Menu m; return m; }
	void Render();
	void Shutdown();
	void ColorPicker(const char* name, float* color, bool alpha);

	void General();    // tab 0
	void Visual();     // tab 1
	void Spawner();    // tab 2
	void Settings();   // tab 3
	void Debug();      // tab 4

	bool isOpen = false;
};

void DrawOverlay();
extern std::vector< pz::entity > g_entities;
extern bool g_game_ready;

namespace menu_state {
	extern bool full_bright;
	extern bool night_vision;
	extern bool zombie_ignore;
	extern bool god_mode;
	extern bool anti_hunger;
	extern bool unlimited_carry;
	extern bool anti_overload;
	extern bool anti_thirst;
	extern bool auto_heal;
	extern bool infinite_ammo;
	extern bool unlimited_endurance;
	extern bool instant_actions;
	extern bool aim_assist;
	extern float aim_assist_max_dist;
	extern bool perfect_accuracy;
	extern bool one_hit;
	extern bool anti_fatigue;
	extern bool all_needs;
	extern bool invisible;
	extern bool no_reload;
	extern bool debug_bypass;
	extern bool noclip;

	extern bool  zombie_esp_enabled;
	extern float zombie_esp_max_dist;
	extern bool  zombie_esp_box;
	extern bool  zombie_esp_name;
	extern bool  zombie_esp_health;
	extern bool  zombie_esp_show_dist;
	extern float zombie_esp_color[4];

	extern bool  player_esp_enabled;
	extern float player_esp_max_dist;
	extern bool  player_esp_box;
	extern bool  player_esp_name;
	extern bool  player_esp_health;
	extern bool  player_esp_show_dist;
	extern float player_esp_color[4];

	extern bool  vehicle_esp_enabled;
	extern float vehicle_esp_max_dist;
	extern bool  vehicle_esp_box;
	extern bool  vehicle_esp_name;
	extern bool  vehicle_esp_show_dist;
	extern float vehicle_esp_color[4];

	extern bool  animal_esp_enabled;
	extern float animal_esp_max_dist;
	extern bool  animal_esp_box;
	extern bool  animal_esp_name;
	extern bool  animal_esp_health;
	extern bool  animal_esp_show_dist;
	extern float animal_esp_color[4];

	extern bool  item_esp_enabled;
	extern float item_esp_max_dist;
	extern bool  item_esp_name;
	extern bool  item_esp_show_dist;
	extern float item_esp_color[4];

	extern bool  esp_render_enabled;
	extern float menu_color[4];
	extern int   menu_key;
}
