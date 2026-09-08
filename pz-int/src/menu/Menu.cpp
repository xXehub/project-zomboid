#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Menu.h"
#include "MenuControls.h"
#include "Icons.h"
#include "IconFont.h"
#include "xorstr.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "../projection.h"
#include <algorithm>
#include <string>
#include <cstring>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <fstream>
#include <atomic>
#include <Windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

extern ImFont* menuFont;
extern ImFont* tabFont;

#define ENCL(str) ([]() -> const char* {                               \
	static char buf[sizeof(str)]{};                                \
	if (buf[0] == '\0') {                                          \
		auto e = xorstr(str);                                      \
		strncpy_s(buf, e.crypt_get(), _TRUNCATE);                  \
	}                                                              \
	return buf;                                                    \
})()

std::vector<pz::entity> g_entities;
bool g_game_ready = false;
float g_menu_accent[4] = {0.47f,0.68f,0.86f,1.0f};

namespace menu_state {
	bool full_bright=false, night_vision=false, zombie_ignore=false, god_mode=false;
	bool anti_hunger=false, unlimited_carry=false, anti_overload=false, anti_thirst=false;
	bool auto_heal=false, infinite_ammo=false;
	bool unlimited_endurance=false, instant_actions=false, aim_assist=false;
	float aim_assist_max_dist=20.f;
	bool perfect_accuracy=false, one_hit=false;
	bool anti_fatigue=false, all_needs=false, invisible=false, noclip=false, no_reload=false;
	bool zombie_esp_enabled=true; float zombie_esp_max_dist=50.f;
	bool zombie_esp_box=true, zombie_esp_name=false, zombie_esp_health=false, zombie_esp_show_dist=false;
	float zombie_esp_color[4]={0.87f,0.27f,0.27f,1.0f};
	bool player_esp_enabled=true; float player_esp_max_dist=50.f;
	bool player_esp_box=true, player_esp_name=true, player_esp_health=false, player_esp_show_dist=false;
	float player_esp_color[4]={0.34f,0.78f,0.36f,1.0f};
	bool vehicle_esp_enabled=true; float vehicle_esp_max_dist=80.f;
	bool vehicle_esp_box=true, vehicle_esp_name=true, vehicle_esp_show_dist=true;
	float vehicle_esp_color[4]={0.40f,0.70f,1.00f,1.0f};
	bool animal_esp_enabled=true; float animal_esp_max_dist=60.f;
	bool animal_esp_box=true, animal_esp_name=true, animal_esp_health=true, animal_esp_show_dist=false;
	float animal_esp_color[4]={1.00f,0.85f,0.30f,1.0f};
	bool item_esp_enabled=true; float item_esp_max_dist=30.f;
	bool item_esp_name=true, item_esp_show_dist=false;
	float item_esp_color[4]={0.80f,0.80f,0.80f,1.0f};
	bool esp_render_enabled=true;
	float menu_color[4]={0.47f,0.68f,0.86f,1.0f};
	int menu_key=VK_INSERT;
}

void Menu::ColorPicker(const char* n, float* c, bool a) {
	auto f=a?ImGuiColorEditFlags_AlphaBar:ImGuiColorEditFlags_NoAlpha;
	ImGui::SameLine(219.f);
	ImGui::ColorEdit4(std::string{"##"}.append(n).append("P").c_str(),c,f|ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoTooltip);
}

static int tab=0;
void Menu::Render() {
	ImGuiStyle* style=&ImGui::GetStyle();
	style->WindowPadding=ImVec2(6,6); style->Alpha=1.0f;
	style->Colors[ImGuiCol_WindowBg]=ImColor(40,40,40,255);
	style->Colors[ImGuiCol_ChildBg]=ImColor(17,17,17,255);
	style->Colors[ImGuiCol_PopupBg]=ImColor(65,65,65,255);
	style->Colors[ImGuiCol_Border]=ImColor(10,10,10,255);
	style->Colors[ImGuiCol_FrameBg]=ImColor(32,32,38,255);
	style->Colors[ImGuiCol_FrameBgHovered]=ImColor(32,32,38,255);
	style->Colors[ImGuiCol_FrameBgActive]=ImColor(32,32,38,255);
	style->Colors[ImGuiCol_TitleBg]=ImColor(38,31,71,255);
	style->Colors[ImGuiCol_TitleBgActive]=ImColor(38,31,71,255);
	style->Colors[ImGuiCol_ScrollbarBg]=ImColor(17,17,17,255);
	style->Colors[ImGuiCol_ScrollbarGrab]=ImColor(48,48,48,255);
	style->Colors[ImGuiCol_ScrollbarGrabHovered]=ImColor(60,60,60,255);
	style->Colors[ImGuiCol_ScrollbarGrabActive]=ImColor(60,60,60,255);
	style->Colors[ImGuiCol_Tab]=ImColor(34,34,34,0);
	style->Colors[ImGuiCol_TabHovered]=ImColor(51,51,51,0);
	style->Colors[ImGuiCol_TabActive]=ImColor(51,51,51,0);
	style->Colors[ImGuiCol_TabText]=ImColor(100,100,100,255);
	style->Colors[ImGuiCol_TabTextHovered]=ImColor(185,185,185,255);
	style->Colors[ImGuiCol_Text]=ImColor(213,213,213,255);
	style->Colors[ImGuiCol_TextShadow]=ImColor(2,2,2,255);
	style->Colors[ImGuiCol_TextDisabled]=ImColor(125,125,125,255);
	style->Colors[ImGuiCol_TextSelectedBg]=ImColor(51,51,51,180);
	style->Colors[ImGuiCol_NavHighlight]=ImVec4(0,0,0,0);
	style->Colors[ImGuiCol_Button]=ImColor(37,37,37,255);
	style->Colors[ImGuiCol_ButtonHovered]=ImColor(47,47,47,255);
	style->Colors[ImGuiCol_ButtonActive]=ImColor(27,27,27,255);
	ImVec4 ac=ImVec4(menu_state::menu_color[0],menu_state::menu_color[1],menu_state::menu_color[2],menu_state::menu_color[3]);
	for(int i=0;i<4;++i)g_menu_accent[i]=menu_state::menu_color[i];
	style->Colors[ImGuiCol_MenuTheme]=ac;
	style->Colors[ImGuiCol_CheckMark]=ac;
	style->Colors[ImGuiCol_SliderGrab]=ac;
	style->Colors[ImGuiCol_SliderGrabActive]=ac;
	ImGui::PushFont(menuFont);
	ImGui::SetNextWindowSize(ImVec2(660.f,560.f));
	ImGui::BeginMenuBackground(ENCL("pz-int"),&isOpen,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoTitleBar);{
		style->Colors[ImGuiCol_MenuTheme]=ac;style->Colors[ImGuiCol_CheckMark]=ac;style->Colors[ImGuiCol_SliderGrab]=ac;style->Colors[ImGuiCol_SliderGrabActive]=ac;
		ImGui::BeginChild("Complete Border",ImVec2(648.f,548.f),false);{}ImGui::EndChild();
		ImGui::SameLine(6.f);style->Colors[ImGuiCol_ChildBg]=ImColor(0,0,0,0);
		ImGui::BeginChild("Menu Contents",ImVec2(648.f,548.f),false);{
			ImGui::ColorBar("unicorn",ImVec2(648.f,2.f));
			style->ItemSpacing=ImVec2(0.f,-1.f);
			ImGui::BeginTabs("Tabs",ImVec2(75.f,542.f),false);{
				style->ItemSpacing=ImVec2(0.f,0.f);style->ButtonTextAlign=ImVec2(0.5f,0.47f);
				ImGui::PopFont();ImGui::PushFont(tabFont);
				ImGui::TabSpacer("##TS",ImVec2(75.f,10.f));
				auto dt=[&](int i,const char*ic){if(tab==i){if(ImGui::SelectedTab(ic,ImVec2(75.f,75.f)))tab=i;}else{if(ImGui::Tab(ic,ImVec2(75.f,75.f)))tab=i;}};
			dt(0,ICON_FA_SHIELD_ALT); dt(1,ICON_FA_EYE); dt(2,ICON_FA_BOX); dt(3,ICON_FA_COG); dt(4,ICON_FA_BOMB);
			ImGui::TabSpacer2("##F1",ImVec2(75.f,75.f));ImGui::TabSpacer2("##BT",ImVec2(75.f,7.f));
				ImGui::PopFont();ImGui::PushFont(menuFont);style->ButtonTextAlign=ImVec2(0.5f,0.5f);
			}ImGui::EndTabs();
			ImGui::SameLine(75.f);
			ImGui::BeginChild("Tab Contents",ImVec2(572.f,542.f),false);{
				style->Colors[ImGuiCol_Border]=ImColor(0,0,0,0);
			switch(tab){case 0:General();break;case 1:Visual();break;case 2:Spawner();break;case 3:Settings();break;case 4:Debug();break;}
				style->Colors[ImGuiCol_Border]=ImColor(10,10,10,255);
			}ImGui::EndChild();
			style->ItemSpacing=ImVec2(4.f,4.f);style->Colors[ImGuiCol_ChildBg]=ImColor(17,17,17,255);
		}ImGui::EndChild();ImGui::PopFont();
	}ImGui::End();
}

void Menu::Shutdown() {
	isOpen=false;
	menu_state::full_bright=false;
	menu_state::night_vision=false;
	menu_state::zombie_ignore=false;
	menu_state::god_mode=false;
	menu_state::anti_hunger=false;
	menu_state::anti_overload=false;
	menu_state::unlimited_carry=false;
	menu_state::no_reload=false;
	menu_state::anti_thirst=false;
	menu_state::auto_heal=false;
	menu_state::infinite_ammo=false;
	menu_state::unlimited_endurance=false;
	menu_state::instant_actions=false;
	menu_state::aim_assist=false;
	menu_state::perfect_accuracy=false;
	menu_state::one_hit=false;
	menu_state::anti_fatigue=false;
	menu_state::all_needs=false;
	menu_state::invisible=false;
	menu_state::noclip=false;
	menu_state::zombie_esp_enabled=false;
	menu_state::zombie_esp_box=false;
	menu_state::zombie_esp_name=false;
	menu_state::zombie_esp_health=false;
	menu_state::zombie_esp_show_dist=false;
	menu_state::player_esp_enabled=false;
	menu_state::player_esp_box=false;
	menu_state::player_esp_name=false;
	menu_state::player_esp_health=false;
	menu_state::player_esp_show_dist=false;
	menu_state::vehicle_esp_enabled=false;
	menu_state::vehicle_esp_box=false;
	menu_state::vehicle_esp_name=false;
	menu_state::vehicle_esp_show_dist=false;
	menu_state::animal_esp_enabled=false;
	menu_state::animal_esp_box=false;
	menu_state::animal_esp_name=false;
	menu_state::animal_esp_health=false;
	menu_state::animal_esp_show_dist=false;
	menu_state::item_esp_enabled=false;
	menu_state::item_esp_name=false;
	menu_state::item_esp_show_dist=false;
	menu_state::esp_render_enabled=false;
	g_entities.clear();
	g_game_ready=false;
}

// ====================== Tab 0: General ============================
void Menu::General() {
	ImGuiStyle* style=&ImGui::GetStyle(); InsertSpacer("Top Spacer");
	ImGui::Columns(2,NULL,false);{
		InsertGroupBoxLeft(ENCL("Character"),292.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("God mode",menu_state::god_mode);
			InsertCheckbox("Auto heal",menu_state::auto_heal);
			InsertCheckbox("Anti hunger",menu_state::anti_hunger);
			InsertCheckbox("Anti thirst",menu_state::anti_thirst);
			InsertCheckbox("Anti fatigue",menu_state::anti_fatigue);
			InsertCheckbox("All needs",menu_state::all_needs);
			InsertCheckbox("Unlimited carry",menu_state::unlimited_carry);
			InsertCheckbox("Anti overload",menu_state::anti_overload);
			InsertCheckbox("Unlimited endurance",menu_state::unlimited_endurance);
			InsertCheckbox("Instant actions",menu_state::instant_actions);
			InsertCheckbox("Invisible",menu_state::invisible);
			InsertCheckbox("No clip",menu_state::noclip);
			InsertCheckbox("Zombies ignore",menu_state::zombie_ignore);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Character Cover"),ENCL("Character"));
		InsertSpacer("C-Cmb Spacer");
		InsertGroupBoxLeft(ENCL("Combat"),214.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Aim assist",menu_state::aim_assist);
			{float t=menu_state::aim_assist_max_dist;InsertSlider("Aim range##aimd",t,3.f,50.f,"%.0f");menu_state::aim_assist_max_dist=t;}
			InsertCheckbox("Perfect accuracy",menu_state::perfect_accuracy);
			InsertCheckbox("One-hit damage",menu_state::one_hit);
			InsertCheckbox("Infinite ammo",menu_state::infinite_ammo);
			InsertCheckbox("No reload",menu_state::no_reload);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			if(ImGui::Button(ENCL("Refill ammo"),ImVec2(108.f,26.f)))pz::refill_ammo();
			ImGui::SameLine(0.f,4.f);
			if(ImGui::Button(ENCL("Reveal map"),ImVec2(108.f,26.f)))pz::reveal_map();
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			if(ImGui::Button(ENCL("Grant admin access"),ImVec2(220.f,26.f)))pz::grant_admin();
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Combat Cover"),ENCL("Combat"));
	}ImGui::NextColumn();{
		InsertGroupBoxRight(ENCL("Skills"),520.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			const int n=pz::perk_count();
			if(n<=0){
				ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(14.f);
				ImGui::TextDisabled(g_game_ready?"No skills available":"Waiting for game...");
			}else{
				for(int i=0;i<n;++i){
					const char* nm=pz::perk_name(i);
					if(!nm||!nm[0])continue;
					const int lvl=pz::perk_level(i);
					float f=(float)lvl;
					char id[80];_snprintf_s(id,sizeof(id),_TRUNCATE,"##sk%d",i);
					ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(14.f);
					ImGui::Text("%s",nm);
					ImGui::SameLine(150.f);ImGui::PushItemWidth(90.f);
					if(ImGui::SliderFloat(id,&f,0.f,10.f,"%.0f")){
						const int nl=(int)(f+0.5f);
						if(nl!=lvl)pz::set_perk_level(i,nl);
					}
					ImGui::PopItemWidth();
				}
			}
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Skills Cover"),ENCL("Skills"));
	}ImGui::Columns(1);
}

// ====================== Tab 1: Visual =============================
void Menu::Visual() {
	ImGuiStyle* style=&ImGui::GetStyle(); InsertSpacer("Top Spacer");
	char lb[64];
	// One ESP groupbox. Each entity type is a main toggle; its sub-options
	// only appear (indented) while that toggle is enabled.
	auto esp_section=[&](const char* base,const char* id,bool& en,float& md,
		float lim,bool* box,bool* nm,bool* hp,bool& sd,float* col){
		_snprintf_s(lb,sizeof(lb),_TRUNCATE,"%s ESP##%se",base,id);
		ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);ImGui::Checkbox(lb,&en);
		_snprintf_s(lb,sizeof(lb),_TRUNCATE,"##%scol",id);ColorPicker(lb,col,true);
		if(!en)return;
		{float t=md;_snprintf_s(lb,sizeof(lb),_TRUNCATE,"Max distance##%sd",id);InsertSlider(lb,t,5.f,lim,"%.0f");md=t;}
		if(box){_snprintf_s(lb,sizeof(lb),_TRUNCATE,"Box##%sb",id);ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(34.f);ImGui::Checkbox(lb,box);}
		if(nm){_snprintf_s(lb,sizeof(lb),_TRUNCATE,"Name##%sn",id);ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(34.f);ImGui::Checkbox(lb,nm);}
		if(hp){_snprintf_s(lb,sizeof(lb),_TRUNCATE,"Health##%sh",id);ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(34.f);ImGui::Checkbox(lb,hp);}
		{_snprintf_s(lb,sizeof(lb),_TRUNCATE,"Distance##%ss",id);ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(34.f);ImGui::Checkbox(lb,&sd);}
		ImGui::CustomSpacing(6.f);
	};
	ImGui::Columns(2,NULL,false);{
		InsertGroupBoxLeft(ENCL("ESP"),498.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			esp_section("Zombie","z",menu_state::zombie_esp_enabled,menu_state::zombie_esp_max_dist,200.f,
				&menu_state::zombie_esp_box,&menu_state::zombie_esp_name,&menu_state::zombie_esp_health,
				menu_state::zombie_esp_show_dist,menu_state::zombie_esp_color);
			esp_section("Player","p",menu_state::player_esp_enabled,menu_state::player_esp_max_dist,200.f,
				&menu_state::player_esp_box,&menu_state::player_esp_name,&menu_state::player_esp_health,
				menu_state::player_esp_show_dist,menu_state::player_esp_color);
			esp_section("Vehicle","v",menu_state::vehicle_esp_enabled,menu_state::vehicle_esp_max_dist,300.f,
				&menu_state::vehicle_esp_box,&menu_state::vehicle_esp_name,nullptr,
				menu_state::vehicle_esp_show_dist,menu_state::vehicle_esp_color);
			esp_section("Animal","a",menu_state::animal_esp_enabled,menu_state::animal_esp_max_dist,200.f,
				&menu_state::animal_esp_box,&menu_state::animal_esp_name,&menu_state::animal_esp_health,
				menu_state::animal_esp_show_dist,menu_state::animal_esp_color);
			esp_section("Item","i",menu_state::item_esp_enabled,menu_state::item_esp_max_dist,100.f,
				nullptr,&menu_state::item_esp_name,nullptr,
				menu_state::item_esp_show_dist,menu_state::item_esp_color);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("ESP Cover"),ENCL("ESP"));
	}ImGui::NextColumn();{
		InsertGroupBoxRight(ENCL("World"),498.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("ESP render",menu_state::esp_render_enabled);
			InsertCheckbox("Full bright",menu_state::full_bright);
			InsertCheckbox("Night vision",menu_state::night_vision);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("World Cover"),ENCL("World"));
	}ImGui::Columns(1);
}

// ====================== Tab 2: Spawner (Elden Ring pattern) ========
namespace spawner {
	static char search[128]="";
	static int selected=-1;
	struct row{const char*full;const char*disp;};
	static std::vector<row> view;
	static int source_count=-1;
	static std::string cached_search;
	static int s_cond=100, s_ammo=-1;

	static bool contains_case_insensitive(const char* text,const std::string& needle){
		if(needle.empty())return true;
		if(!text)return false;
		const auto end=text+std::strlen(text);
		return std::search(text,end,needle.begin(),needle.end(),
			[](char lhs,char rhs){return std::tolower(static_cast<unsigned char>(lhs))==rhs;})!=end;
	}

	static int refresh(){
		const int count=pz::item_db_count();
		const std::string current_search(search);
		if(count==source_count&&current_search==cached_search)return count;
		const bool search_changed=current_search!=cached_search;
		source_count=count;
		cached_search=current_search;
		std::string needle=current_search;
		for(auto&c:needle)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		view.clear();
		view.reserve(count<8192?count:8192);
		for(int i=0;i<count&&static_cast<int>(view.size())<8192;++i){
			const char*display=nullptr;
			const char*full=pz::item_db_get(i,&display);
			if(!full)continue;
			if(!display)display=full;
			if(!contains_case_insensitive(display,needle)&&
				!contains_case_insensitive(full,needle))continue;
			view.push_back({full,display});
		}
		if(search_changed)selected=-1;
		return count;
	}
}

void Menu::Spawner() {
	ImGuiStyle* style=&ImGui::GetStyle(); InsertSpacer("Top Spacer");
	const int item_count=spawner::refresh();
	if(spawner::selected>=static_cast<int>(spawner::view.size()))spawner::selected=-1;

	ImGui::Columns(2,NULL,false);{
		InsertGroupBoxLeft(ENCL("Item Database"),506.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			// Search bar
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			ImGui::PushItemWidth(220.f);
			ImGui::PushStyleColor(ImGuiCol_FrameBg,ImColor(30,30,30,255).Value);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImColor(40,40,40,255).Value);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImColor(22,22,22,255).Value);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0.0f);
			ImGui::InputTextWithHint("##isrch","Search items...",spawner::search,sizeof(spawner::search));
			ImGui::PopStyleVar(2);ImGui::PopStyleColor(3);ImGui::PopItemWidth();
			ImGui::Spacing();
			// Scrollable list with clipper
			ImGui::PushStyleColor(ImGuiCol_ChildBg,ImColor(22,22,22,255).Value);
			ImGui::NewLine();ImGui::SameLine(19.f);
			ImGui::BeginChild("##ILS",ImVec2(220.f,340.f),true);{
				ImGui::PushStyleColor(ImGuiCol_Header,ImColor(45,45,45,255).Value);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImColor(55,55,55,255).Value);
				ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImColor(40,40,40,255).Value);
				ImGuiListClipper clip;
				clip.Begin((int)spawner::view.size());
				while(clip.Step()){
					for(int r=clip.DisplayStart;r<clip.DisplayEnd;++r){
						auto&e=spawner::view[r];
						char lbl[192];_snprintf_s(lbl,sizeof(lbl),_TRUNCATE,"%s##%d",e.disp,r);
						bool sel=(spawner::selected==r);
						if(ImGui::Selectable(lbl,sel,ImGuiSelectableFlags_None,ImVec2(196.f,0)))
							spawner::selected=r;
						if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",e.full);
						if(sel)ImGui::SetItemDefaultFocus();
					}
				}
				clip.End();
				if(spawner::view.empty())ImGui::TextDisabled("no items match");
				ImGui::PopStyleColor(3);
			}ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			ImGui::TextDisabled("%d / %d items",static_cast<int>(spawner::view.size()),item_count);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Item Database Cover"),ENCL("Item Database"));
	}ImGui::NextColumn();{
		// Spawn controls
		InsertGroupBoxRight(ENCL("Spawn"),320.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);ImGui::Text("Selected:");
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);
			if(spawner::selected>=0&&spawner::selected<(int)spawner::view.size())
				ImGui::TextWrapped("%s",spawner::view[spawner::selected].full);
			else ImGui::TextDisabled("(none)");
			// Condition
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);ImGui::Text("Condition");
			ImGui::SameLine(90.f);ImGui::PushItemWidth(149.f);
			ImGui::InputInt("##cnd",&spawner::s_cond);
			if(spawner::s_cond<0)spawner::s_cond=0;if(spawner::s_cond>100)spawner::s_cond=100;
			ImGui::PopItemWidth();
			// Ammo
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);ImGui::Text("Ammo");
			ImGui::SameLine(90.f);ImGui::PushItemWidth(149.f);
			ImGui::InputInt("##amo",&spawner::s_ammo);ImGui::PopItemWidth();
			ImGui::NewLine();ImGui::SameLine(10.f);ImGui::TextDisabled("-1 = default");
			ImGui::Spacing();
			const bool ok=spawner::selected>=0&&spawner::selected<(int)spawner::view.size()&&g_game_ready;
            // Spawn on the player's current ground square.
            ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);
            if(!ok)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
            if(ImGui::Button(ENCL("Spawn on Ground"),ImVec2(230.f,26.f))&&ok)
                pz::spawn_item(spawner::view[spawner::selected].full);
            if(!ok)ImGui::PopStyleVar();
            // Custom ground spawn (condition + ammo)
            ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);
            if(!ok)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
            if(ImGui::Button(ENCL("Spawn Custom on Ground"),ImVec2(230.f,26.f))&&ok)
                pz::spawn_item_custom(spawner::view[spawner::selected].full,spawner::s_cond,spawner::s_ammo);
			if(!ok)ImGui::PopStyleVar();
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Spawn Cover"),ENCL("Spawn"));
		InsertSpacer("S-I Spacer");
		InsertGroupBoxRight(ENCL("Info"),168.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
            ImGui::TextDisabled("Spawn = ground, default stats");
            ImGui::TextDisabled("Custom = ground + condition/ammo");
			ImGui::Spacing();ImGui::Text("Game: %s",g_game_ready?"ready":"waiting...");
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Info Cover"),ENCL("Info"));
	}ImGui::Columns(1);
}

// ====================== Config IO (Elden Ring / cs2-6s pattern) ========
namespace config_io {
	struct config_data {
		bool full_bright, night_vision, zombie_ignore, god_mode;
		bool anti_hunger, unlimited_carry, anti_thirst;
		bool auto_heal, infinite_ammo;
		bool ze_en; float ze_dist; bool ze_box, ze_name, ze_hp, ze_sd; float ze_col[4];
		bool pe_en; float pe_dist; bool pe_box, pe_name, pe_hp, pe_sd; float pe_col[4];
		bool ve_en; float ve_dist; bool ve_box, ve_name, ve_sd; float ve_col[4];
		bool ae_en; float ae_dist; bool ae_box, ae_name, ae_hp, ae_sd; float ae_col[4];
		bool ie_en; float ie_dist; bool ie_name, ie_sd; float ie_col[4];
		bool esp_render; float menu_col[4]; int menu_key;
		bool unlimited_endurance, instant_actions, aim_assist;
		float aim_assist_max_dist;
		bool perfect_accuracy, one_hit;
		bool anti_fatigue, all_needs, invisible, noclip;
		bool anti_overload;
		bool no_reload;
	};
	static constexpr std::size_t legacy_size=offsetof(config_data,unlimited_endurance);
	static std::string sanitize(const std::string& in) {
		std::string o; o.reserve(in.size());
		for(char c:in){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_')o.push_back(c);else if(c==' ')o.push_back('_');}
		return o;
	}
	static std::wstring base_dir() {
		wchar_t ap[MAX_PATH]{}; ::SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,ap);
		std::wstring d=std::wstring(ap)+L"\\pzint"; ::CreateDirectoryW(d.c_str(),nullptr); return d;
	}
	static std::wstring configs_dir() { auto d=base_dir()+L"\\configs"; ::CreateDirectoryW(d.c_str(),nullptr); return d; }
	static std::wstring path_for(const std::string& name) {
		auto c=sanitize(name); if(c.empty())c="default";
		std::wstring w; for(char ch:c)w.push_back((wchar_t)ch); return configs_dir()+L"\\"+w+L".ini";
	}
	static std::wstring default_marker() { return configs_dir()+L"\\default.txt"; }
	static void pack(config_data& d) {
		d.full_bright=menu_state::full_bright; d.night_vision=menu_state::night_vision;
		d.zombie_ignore=menu_state::zombie_ignore; d.god_mode=menu_state::god_mode;
		d.anti_hunger=menu_state::anti_hunger; d.unlimited_carry=menu_state::unlimited_carry; d.anti_thirst=menu_state::anti_thirst;
		d.auto_heal=menu_state::auto_heal; d.infinite_ammo=menu_state::infinite_ammo;
		d.unlimited_endurance=menu_state::unlimited_endurance; d.instant_actions=menu_state::instant_actions;
		d.aim_assist=menu_state::aim_assist; d.aim_assist_max_dist=menu_state::aim_assist_max_dist;
		d.perfect_accuracy=menu_state::perfect_accuracy; d.one_hit=menu_state::one_hit;
		d.anti_fatigue=menu_state::anti_fatigue; d.all_needs=menu_state::all_needs;
		d.invisible=menu_state::invisible; d.noclip=menu_state::noclip;
		d.anti_overload=menu_state::anti_overload;
		d.no_reload=menu_state::no_reload;
		d.ze_en=menu_state::zombie_esp_enabled; d.ze_dist=menu_state::zombie_esp_max_dist;
		d.ze_box=menu_state::zombie_esp_box; d.ze_name=menu_state::zombie_esp_name;
		d.ze_hp=menu_state::zombie_esp_health; d.ze_sd=menu_state::zombie_esp_show_dist;
		memcpy(d.ze_col,menu_state::zombie_esp_color,16);
		d.pe_en=menu_state::player_esp_enabled; d.pe_dist=menu_state::player_esp_max_dist;
		d.pe_box=menu_state::player_esp_box; d.pe_name=menu_state::player_esp_name;
		d.pe_hp=menu_state::player_esp_health; d.pe_sd=menu_state::player_esp_show_dist;
		memcpy(d.pe_col,menu_state::player_esp_color,16);
		d.ve_en=menu_state::vehicle_esp_enabled; d.ve_dist=menu_state::vehicle_esp_max_dist;
		d.ve_box=menu_state::vehicle_esp_box; d.ve_name=menu_state::vehicle_esp_name;
		d.ve_sd=menu_state::vehicle_esp_show_dist;
		memcpy(d.ve_col,menu_state::vehicle_esp_color,16);
		d.ae_en=menu_state::animal_esp_enabled; d.ae_dist=menu_state::animal_esp_max_dist;
		d.ae_box=menu_state::animal_esp_box; d.ae_name=menu_state::animal_esp_name;
		d.ae_hp=menu_state::animal_esp_health; d.ae_sd=menu_state::animal_esp_show_dist;
		memcpy(d.ae_col,menu_state::animal_esp_color,16);
		d.ie_en=menu_state::item_esp_enabled; d.ie_dist=menu_state::item_esp_max_dist;
		d.ie_name=menu_state::item_esp_name; d.ie_sd=menu_state::item_esp_show_dist;
		memcpy(d.ie_col,menu_state::item_esp_color,16);
		d.esp_render=menu_state::esp_render_enabled;
		memcpy(d.menu_col,menu_state::menu_color,16); d.menu_key=menu_state::menu_key;
	}
	static void unpack(const config_data& d) {
		menu_state::full_bright=d.full_bright; menu_state::night_vision=d.night_vision;
		menu_state::zombie_ignore=d.zombie_ignore; menu_state::god_mode=d.god_mode;
		menu_state::anti_hunger=d.anti_hunger; menu_state::unlimited_carry=d.unlimited_carry; menu_state::anti_thirst=d.anti_thirst;
		menu_state::auto_heal=d.auto_heal; menu_state::infinite_ammo=d.infinite_ammo;
		menu_state::unlimited_endurance=d.unlimited_endurance; menu_state::instant_actions=d.instant_actions;
		menu_state::aim_assist=d.aim_assist; menu_state::aim_assist_max_dist=d.aim_assist_max_dist>0.f?d.aim_assist_max_dist:20.f;
		menu_state::perfect_accuracy=d.perfect_accuracy; menu_state::one_hit=d.one_hit;
		menu_state::anti_fatigue=d.anti_fatigue; menu_state::all_needs=d.all_needs;
		menu_state::invisible=d.invisible; menu_state::noclip=d.noclip;
		menu_state::anti_overload=d.anti_overload;
		menu_state::no_reload=d.no_reload;
		menu_state::zombie_esp_enabled=d.ze_en; menu_state::zombie_esp_max_dist=d.ze_dist;
		menu_state::zombie_esp_box=d.ze_box; menu_state::zombie_esp_name=d.ze_name;
		menu_state::zombie_esp_health=d.ze_hp; menu_state::zombie_esp_show_dist=d.ze_sd;
		memcpy(menu_state::zombie_esp_color,d.ze_col,16);
		menu_state::player_esp_enabled=d.pe_en; menu_state::player_esp_max_dist=d.pe_dist;
		menu_state::player_esp_box=d.pe_box; menu_state::player_esp_name=d.pe_name;
		menu_state::player_esp_health=d.pe_hp; menu_state::player_esp_show_dist=d.pe_sd;
		memcpy(menu_state::player_esp_color,d.pe_col,16);
		menu_state::vehicle_esp_enabled=d.ve_en; menu_state::vehicle_esp_max_dist=d.ve_dist;
		menu_state::vehicle_esp_box=d.ve_box; menu_state::vehicle_esp_name=d.ve_name;
		menu_state::vehicle_esp_show_dist=d.ve_sd;
		memcpy(menu_state::vehicle_esp_color,d.ve_col,16);
		menu_state::animal_esp_enabled=d.ae_en; menu_state::animal_esp_max_dist=d.ae_dist;
		menu_state::animal_esp_box=d.ae_box; menu_state::animal_esp_name=d.ae_name;
		menu_state::animal_esp_health=d.ae_hp; menu_state::animal_esp_show_dist=d.ae_sd;
		memcpy(menu_state::animal_esp_color,d.ae_col,16);
		menu_state::item_esp_enabled=d.ie_en; menu_state::item_esp_max_dist=d.ie_dist;
		menu_state::item_esp_name=d.ie_name; menu_state::item_esp_show_dist=d.ie_sd;
		memcpy(menu_state::item_esp_color,d.ie_col,16);
		menu_state::esp_render_enabled=d.esp_render;
		memcpy(menu_state::menu_color,d.menu_col,16); menu_state::menu_key=d.menu_key;
		if(menu_state::menu_color[3]<=0.f)menu_state::menu_color[3]=1.f;
	}
	static void save(const char* name) {
		config_data d{}; pack(d);
		auto p=path_for(name?name:"default");
		FILE*f=nullptr;::_wfopen_s(&f,p.c_str(),L"wb"); if(f){fwrite(&d,sizeof(d),1,f);fclose(f);}
	}
	static void load(const char* name) {
		auto p=path_for(name?name:"default");
		FILE*f=nullptr;::_wfopen_s(&f,p.c_str(),L"rb"); if(!f)return;
		config_data d{};d.aim_assist_max_dist=20.f;
		const auto r=fread(&d,1,sizeof(d),f);fclose(f);
		if(r<legacy_size)return;
		unpack(d);
	}
	static std::vector<std::string> list() {
		std::vector<std::string> out; auto pat=configs_dir()+L"\\*.ini";
		WIN32_FIND_DATAW fd{}; auto h=::FindFirstFileW(pat.c_str(),&fd);
		if(h==INVALID_HANDLE_VALUE)return out;
		do{ if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
			std::wstring n=fd.cFileName; if(n.size()>4)n.resize(n.size()-4);
			std::string a; for(wchar_t wc:n)a.push_back((char)(wc&0x7F)); out.push_back(std::move(a));
		}while(::FindNextFileW(h,&fd)); ::FindClose(h); return out;
	}
	static bool exists(const std::string& n){return ::GetFileAttributesW(path_for(n).c_str())!=INVALID_FILE_ATTRIBUTES;}
	static bool remove(const std::string& n){return ::DeleteFileW(path_for(n).c_str())!=0;}
	static std::string read_default() {
		std::ifstream f(default_marker()); if(!f)return {};
		std::string l; std::getline(f,l);
		while(!l.empty()&&(l.back()=='\r'||l.back()=='\n'||l.back()==' '))l.pop_back(); return l;
	}
	static void write_default(const std::string& n){std::ofstream f(default_marker(),std::ios::trunc);if(f)f<<n;}
	static void load_default() { auto d=read_default(); if(!d.empty()&&exists(d))load(d.c_str()); }
}

// ====================== Tab 3: Settings (cs2-6s / Elden Ring pattern) ==
extern std::atomic<bool> g_pzint_unload;

void Menu::Settings() {
	{ static bool once=false; if(!once){config_io::load_default();once=true;} }

	ImGuiStyle* style=&ImGui::GetStyle(); InsertSpacer("Top Spacer");
	ImGui::Columns(2,NULL,false);{
		InsertGroupBoxLeft(ENCL("Menu"),506.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertKeypicker("Menu key",menu_state::menu_key);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Menu color");
			InsertColorPicker("##mcol",menu_state::menu_color,true);
			InsertCheckbox("ESP render",menu_state::esp_render_enabled);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Menu Cover"),ENCL("Menu"));
	}ImGui::NextColumn();{
		// ---- Presets (cs2-6s / Elden Ring pattern) ----
		InsertGroupBoxRight(ENCL("Presets"),320.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			static char cfg_name[64]=""; static int sel_load=-1;
			auto cl=config_io::list();
			std::vector<const char*>cv; for(auto&s:cl)cv.push_back(s.c_str());
			auto def_name=config_io::read_default();
			if(sel_load>=(int)cl.size())sel_load=-1;
			// Name input
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);ImGui::PushItemWidth(174.f);
			ImGui::PushStyleColor(ImGuiCol_FrameBg,ImColor(30,30,30,255).Value);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImColor(40,40,40,255).Value);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImColor(22,22,22,255).Value);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0.f);
			ImGui::InputTextWithHint("##cfgname","config name...",cfg_name,IM_ARRAYSIZE(cfg_name));
			ImGui::PopStyleVar(2);ImGui::PopStyleColor(3);ImGui::PopItemWidth();
			// Load dropdown
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);ImGui::PushItemWidth(174.f);
			if(!cv.empty()){if(ImGui::Combo("##cfglist",&sel_load,cv.data(),(int)cv.size()))
				if(sel_load>=0&&sel_load<(int)cl.size())strncpy_s(cfg_name,cl[sel_load].c_str(),_TRUNCATE);
			}else{int dm=0;const char*em[]={"(no configs)"};ImGui::PushStyleColor(ImGuiCol_Text,ImColor(120,120,120,255).Value);ImGui::Combo("##cfglist_e",&dm,em,1);ImGui::PopStyleColor();}
			ImGui::PopItemWidth();
			// Buttons
			ImGui::PushStyleColor(ImGuiCol_Button,ImColor(37,37,37,255).Value);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(47,47,47,255).Value);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImColor(27,27,27,255).Value);
			ImGui::PushStyleColor(ImGuiCol_Border,ImColor(60,60,60,255).Value);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0.f);
			std::string tgt=cfg_name[0]?std::string(cfg_name):"default";
			bool tex=config_io::exists(tgt); bool isd=!def_name.empty()&&def_name==tgt;
			// Row 1: Create | Overwrite
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);
			if(tex)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
			if(ImGui::Button("Create",ImVec2(82.f,22.f))&&!tex)config_io::save(tgt.c_str());
			if(tex)ImGui::PopStyleVar();
			ImGui::SameLine();
			if(!tex)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
			if(ImGui::Button("Overwrite",ImVec2(82.f,22.f))&&tex)config_io::save(tgt.c_str());
			if(!tex)ImGui::PopStyleVar();
			// Row 2: Load | Delete
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);
			if(!tex)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
			if(ImGui::Button("Load",ImVec2(82.f,22.f))&&tex)config_io::load(tgt.c_str());
			ImGui::SameLine();
			if(ImGui::Button("Delete",ImVec2(82.f,22.f))&&tex){if(config_io::remove(tgt)){if(isd)config_io::write_default("");sel_load=-1;cfg_name[0]='\0';}}
			if(!tex)ImGui::PopStyleVar();
			// Row 3: Set as Default
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);
			if(isd)ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(menu_state::menu_color[0],menu_state::menu_color[1],menu_state::menu_color[2],1.f));
			std::string dlbl=isd?"Default: "+tgt:(def_name.empty()?"Set as Default":"Default (cur: "+def_name+")");
			if(ImGui::Button(dlbl.c_str(),ImVec2(174.f,22.f))&&tex)config_io::write_default(tgt);
			if(isd)ImGui::PopStyleColor();
			// Row 4: Reset
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);
			if(ImGui::Button("Reset settings",ImVec2(174.f,22.f))){
				menu_state::full_bright=menu_state::night_vision=menu_state::zombie_ignore=menu_state::god_mode=false;
				menu_state::anti_hunger=menu_state::unlimited_carry=menu_state::anti_thirst=false;
				menu_state::anti_overload=false;
				menu_state::auto_heal=menu_state::infinite_ammo=false;
				menu_state::unlimited_endurance=menu_state::instant_actions=menu_state::aim_assist=false;
				menu_state::perfect_accuracy=menu_state::one_hit=false;
				menu_state::no_reload=false;
				menu_state::anti_fatigue=menu_state::all_needs=false;
				menu_state::invisible=menu_state::noclip=false;
				menu_state::aim_assist_max_dist=20.f;
				menu_state::zombie_esp_enabled=menu_state::player_esp_enabled=true;
				menu_state::zombie_esp_max_dist=menu_state::player_esp_max_dist=50.f;
				menu_state::esp_render_enabled=true; menu_state::menu_key=VK_INSERT;
				float dc[]={0.47f,0.68f,0.86f,1.f}; memcpy(menu_state::menu_color,dc,16);
				sel_load=-1;cfg_name[0]='\0';
			}
			ImGui::PopStyleVar(2);ImGui::PopStyleColor(4);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Presets Cover"),ENCL("Presets"));
		InsertSpacer("P-O Spacer");
		// ---- Others (Unload) ----
		InsertGroupBoxRight(ENCL("Others"),168.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(42.f);
			ImGui::PushStyleColor(ImGuiCol_Button,ImColor(64,30,30,255).Value);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(84,35,35,255).Value);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImColor(48,24,24,255).Value);
			ImGui::PushStyleColor(ImGuiCol_Border,ImColor(120,60,60,255).Value);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0.f);
			if(ImGui::Button("Unload DLL",ImVec2(174.f,26.f)))g_pzint_unload.store(true,std::memory_order_release);
			ImGui::PopStyleVar(2);ImGui::PopStyleColor(4);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			ImGui::TextDisabled("INSERT toggles menu");
			ImGui::TextDisabled("END emergency unload");
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Others Cover"),ENCL("Others"));
	}ImGui::Columns(1);
}

// ====================== ESP Overlay ===============================
void DrawOverlay(){
	// Use a fullscreen transparent window for ESP draws. GetForegroundDrawList()
	// and GetBackgroundDrawList() are NOT rendered by this project's old custom
	// ImGui backend. Only window draw lists get processed by RenderDrawData().
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
	ImGui::Begin("##espoverlay",nullptr,
		ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
		ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|
		ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoInputs|
		ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoFocusOnAppearing|
		ImGuiWindowFlags_NoNavFocus|ImGuiWindowFlags_NoNav);
	ImDrawList*dl=ImGui::GetWindowDrawList();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	const float zoom=pz::projection_zoom();
	if(menu_state::esp_render_enabled&&g_game_ready&&zoom>0.0f){
		for(auto&e:g_entities){
			if(e.is_local||!e.on_screen)continue;
			if(e.sx!=e.sx||e.sy!=e.sy||e.dist!=e.dist)continue;
			const bool z=(e.type==pz::entity_type::zombie);
			const bool p=(e.type==pz::entity_type::player);
			const bool v=(e.type==pz::entity_type::vehicle);
			const bool a=(e.type==pz::entity_type::animal);
			const bool it=(e.type==pz::entity_type::item);
			bool enabled=false; float max_d=0; bool show_box=false,show_name=false,show_hp=false,show_dist=false;
			const float*cc=nullptr;
			if(z){enabled=menu_state::zombie_esp_enabled;max_d=menu_state::zombie_esp_max_dist;show_box=menu_state::zombie_esp_box;show_name=menu_state::zombie_esp_name;show_hp=menu_state::zombie_esp_health;show_dist=menu_state::zombie_esp_show_dist;cc=menu_state::zombie_esp_color;}
			else if(p){enabled=menu_state::player_esp_enabled;max_d=menu_state::player_esp_max_dist;show_box=menu_state::player_esp_box;show_name=menu_state::player_esp_name;show_hp=menu_state::player_esp_health;show_dist=menu_state::player_esp_show_dist;cc=menu_state::player_esp_color;}
			else if(v){enabled=menu_state::vehicle_esp_enabled;max_d=menu_state::vehicle_esp_max_dist;show_box=menu_state::vehicle_esp_box;show_name=menu_state::vehicle_esp_name;show_hp=true;show_dist=menu_state::vehicle_esp_show_dist;cc=menu_state::vehicle_esp_color;}
			else if(a){enabled=menu_state::animal_esp_enabled;max_d=menu_state::animal_esp_max_dist;show_box=menu_state::animal_esp_box;show_name=menu_state::animal_esp_name;show_hp=menu_state::animal_esp_health;show_dist=menu_state::animal_esp_show_dist;cc=menu_state::animal_esp_color;}
			else if(it){enabled=menu_state::item_esp_enabled;max_d=menu_state::item_esp_max_dist;show_box=false;show_name=menu_state::item_esp_name;show_hp=false;show_dist=menu_state::item_esp_show_dist;cc=menu_state::item_esp_color;}
			if(!enabled||e.dist>max_d||!cc)continue;
			ImU32 col=IM_COL32((int)(cc[0]*255),(int)(cc[1]*255),(int)(cc[2]*255),(int)(cc[3]*255));
			ImU32 shadow=IM_COL32(0,0,0,200);
			const auto kind=(z||p)?pz::projection::esp_kind::humanoid:
				v?pz::projection::esp_kind::vehicle:a?pz::projection::esp_kind::animal:pz::projection::esp_kind::item;
			const auto pose=(z||p)?e.pose:pz::projection::character_pose::standing;
			const auto box=pz::projection::esp_box_for(kind,pose,e.sx,e.sy,zoom);
			const float bh=box.bottom-box.top;
			if(show_box&&bh>0){
				const float x1=box.left,y1=box.top,x2=box.right,y2=box.bottom;
				dl->AddRect(ImVec2(x1-1,y1-1),ImVec2(x2+1,y2+1),shadow,0,0,2.0f);
				dl->AddRect(ImVec2(x1,y1),ImVec2(x2,y2),col,0,0,1.25f);
				if(show_hp&&e.health>0){
					const float max_health=z?2.1f:100.0f;
					const float fill=pz::projection::health_fraction(e.health,max_health);
					const float bx=x1-6.0f;
					dl->AddRectFilled(ImVec2(bx-2.0f,y1),ImVec2(bx+1.0f,y2),IM_COL32(0,0,0,160));
					const ImU32 bc=IM_COL32((int)((1.f-fill)*255),(int)(fill*255),0,255);
					dl->AddRectFilled(ImVec2(bx-1.0f,y1+bh*(1-fill)),ImVec2(bx,y2),bc);
				}
			}
			float name_y=box.top;
			if(show_name){
				ImVec2 ts=ImGui::CalcTextSize(e.name);
				name_y=pz::projection::label_above(box.top,ts.y);
				float tx=e.sx-ts.x*0.5f;
				dl->AddText(ImVec2(tx+1,name_y+1),shadow,e.name);
				dl->AddText(ImVec2(tx,name_y),IM_COL32(255,255,255,230),e.name);
			}
			if(show_dist){
				char d[32];_snprintf_s(d,sizeof(d),_TRUNCATE,"%.0fm",e.dist);
				ImVec2 ds=ImGui::CalcTextSize(d);
				float dx=e.sx-ds.x*0.5f,dy=pz::projection::label_below(box.bottom);
				dl->AddText(ImVec2(dx+1,dy+1),shadow,d);
				dl->AddText(ImVec2(dx,dy),IM_COL32(200,200,200,200),d);
			}
		}
	}
	// ESP status indicator (top-right, always visible when game ready)
	if(g_game_ready&&menu_state::esp_render_enabled){
		char info[64]; int eon=0;
		for(auto&e:g_entities) if(e.on_screen&&e.dist==e.dist) ++eon;
		_snprintf_s(info,sizeof(info),_TRUNCATE,"ESP: %d/%d visible",eon,(int)g_entities.size());
		dl->AddRectFilled(ImVec2(1550,8),ImVec2(1910,28),IM_COL32(0,0,0,180));
		dl->AddText(ImVec2(1555,10),IM_COL32(0,255,0,255),info);
	}
	// Watermark
	{
		const char*t="pz-int";ImVec2 ts=ImGui::CalcTextSize(t);
		float w=ts.x+16,h=ts.y+16;ImVec2 p(20,20);
		dl->AddRectFilledMultiColor(p,ImVec2(p.x+w/2,p.y+2),ImColor(55,177,218),ImColor(201,84,192),ImColor(201,84,192),ImColor(55,177,218));
		dl->AddRectFilledMultiColor(ImVec2(p.x+w/2,p.y),ImVec2(p.x+w,p.y+2),ImColor(201,84,192),ImColor(204,227,54),ImColor(204,227,54),ImColor(201,84,192));
		dl->AddRectFilled(ImVec2(p.x,p.y+2),ImVec2(p.x+w,p.y+h),ImColor(17,17,17,255));
		dl->AddRect(p,ImVec2(p.x+w,p.y+h),ImColor(60,60,60,255),0,0,1);
		dl->AddText(ImVec2(p.x+9,p.y+9),ImColor(0,0,0,180),t);
		dl->AddText(ImVec2(p.x+8,p.y+8),ImColor(255,255,255,255),t);
	}
	ImGui::End();
}

// ====================== Tab 4: Debug ======================================
void Menu::Debug() {
	ImGui::Spacing();ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_ChildBg,ImColor(22,22,22,255).Value);
	ImGui::BeginChild("##dbg",ImVec2(556.f,520.f),true);{
		const auto d = pz::get_debug_info();

		// Entity counts from live data
		int zc=0,pc=0,vc=0,ac=0,ic=0;
		for(auto&e:g_entities){
			switch(e.type){
			case pz::entity_type::zombie: ++zc; break;
			case pz::entity_type::player: ++pc; break;
			case pz::entity_type::vehicle: ++vc; break;
			case pz::entity_type::animal: ++ac; break;
			case pz::entity_type::item: ++ic; break;
			}
		}

		auto status=[](const char*label,bool ok){
			ImGui::Text("%s:",label);ImGui::SameLine(200.f);
			if(ok)ImGui::TextColored(ImVec4(0.3f,1.f,0.3f,1.f),"OK");
			else ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f),"FAIL");
		};

		ImGui::TextColored(ImVec4(0.47f,0.68f,0.86f,1.f),"=== JNI Bridge ===");
		status("JNI Environment", d.jni_env_valid);
		status("Class Loader", d.class_loader_valid);
		status("Methods Resolved", d.resolved);
		status("IsoUtils Projection", d.isoutils_available);
		status("Climate Method API", d.climate_method_api);
		status("Climate Fields", d.climate_fields_ok);
        status("World Spawn API", d.world_spawn_api_available);
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.47f,0.68f,0.86f,1.f),"=== Frame Context ===");
		status("Frame Valid", d.frame_ctx_valid);
		ImGui::Text("Screen: %dx%d", d.screen_w, d.screen_h);
		ImGui::Text("Tile Scale: %d", d.tile_scale);
		ImGui::Text("Zoom: %.3f", d.zoom);
		ImGui::Text("Camera Offset: %.1f, %.1f", d.cam_off_x, d.cam_off_y);
		ImGui::Text("Player Index: %d", d.player_idx);
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.47f,0.68f,0.86f,1.f),"=== Entity Collection ===");
		ImGui::Text("Total: %d", (int)g_entities.size());
		ImGui::Text("  Zombies: %d", zc);
		ImGui::Text("  Players: %d", pc);
		ImGui::Text("  Vehicles: %d", vc);
		ImGui::Text("  Animals: %d", ac);
		ImGui::Text("  Items: %d", ic);
		// Show first 3 entities with screen coords for projection debug
		int shown=0;
		for(auto&e:g_entities){
			if(shown>=5)break;
			ImGui::Text("  [%d] %s pos=(%.1f,%.1f,%.1f) scr=(%.0f,%.0f) on=%d dist=%.0f",
				shown, e.name, e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen?1:0, e.dist);
			++shown;
		}
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.47f,0.68f,0.86f,1.f),"=== Spawner ===");
		ImGui::Text("Last: %s", d.last_spawn_result);
		ImGui::Text("Item DB: %d items loaded", pz::item_db_count());
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.47f,0.68f,0.86f,1.f),"=== Feature State ===");
		ImGui::Text("God mode: %s", menu_state::god_mode?"ON":"off");
		ImGui::Text("Full bright: %s", menu_state::full_bright?"ON":"off");
		ImGui::Text("Night vision: %s", menu_state::night_vision?"ON":"off");
		ImGui::Text("Auto heal: %s", menu_state::auto_heal?"ON":"off");
		ImGui::Text("Zombie ignore: %s", menu_state::zombie_ignore?"ON":"off");
		ImGui::Text("Unlimited carry: %s", menu_state::unlimited_carry?"ON":"off");
		ImGui::Text("Infinite ammo: %s", menu_state::infinite_ammo?"ON":"off");
		ImGui::Text("ESP render: %s", menu_state::esp_render_enabled?"ON":"off");
        ImGui::Text("ESP types: Z=%s P=%s V=%s A=%s I=%s",
            menu_state::zombie_esp_enabled?"ON":"off",
            menu_state::player_esp_enabled?"ON":"off",
            menu_state::vehicle_esp_enabled?"ON":"off",
            menu_state::animal_esp_enabled?"ON":"off",
            menu_state::item_esp_enabled?"ON":"off");
		ImGui::Spacing();ImGui::Spacing();

		// Big red DUMP button
		ImGui::PushStyleColor(ImGuiCol_Button,ImColor(120,30,30,255).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(160,40,40,255).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImColor(80,20,20,255).Value);
		if(ImGui::Button("DUMP DEEP DEBUG TO FILE",ImVec2(520.f,30.f))){
			pz::dump_deep_debug(g_entities);
		}
		ImGui::PopStyleColor(3);
		ImGui::TextDisabled("Writes to %%TEMP%%\\pzint_dump.txt");
	}ImGui::EndChild();
	ImGui::PopStyleColor();
}
