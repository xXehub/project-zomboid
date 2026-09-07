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
#include <algorithm>
#include <string>
#include <cstring>
#include <cctype>
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
	bool anti_hunger=false, anti_encumbrance=false, anti_thirst=false;
	bool auto_heal=false;
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
	menu_state::anti_encumbrance=false;
	menu_state::anti_thirst=false;
	menu_state::auto_heal=false;
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
		InsertGroupBoxLeft(ENCL("Character"),300.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("God mode",menu_state::god_mode);
			InsertCheckbox("Auto heal",menu_state::auto_heal);
			InsertCheckbox("Anti hunger",menu_state::anti_hunger);
			InsertCheckbox("Anti encumbrance",menu_state::anti_encumbrance);
			InsertCheckbox("Anti thirst",menu_state::anti_thirst);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Character Cover"),ENCL("Character"));
		InsertSpacer("C-W Spacer");
		InsertGroupBoxLeft(ENCL("Weapon"),150.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(19.f);
			if(ImGui::Button(ENCL("Refill ammo"),ImVec2(220.f,26.f)))pz::refill_ammo();
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Weapon Cover"),ENCL("Weapon"));
	}ImGui::NextColumn();{
		InsertGroupBoxRight(ENCL("World"),210.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Full bright",menu_state::full_bright);
			InsertCheckbox("Night vision",menu_state::night_vision);
			InsertCheckbox("Zombies ignore",menu_state::zombie_ignore);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("World Cover"),ENCL("World"));
		InsertSpacer("W-S Spacer");
		InsertGroupBoxRight(ENCL("Status"),220.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			ImGui::Text("Game: %s",g_game_ready?"ready":"waiting...");ImGui::Spacing();
			int zc=0,pc=0,vc=0,ac=0,ic=0;
			for(auto&e:g_entities){
				switch(e.type){
				case pz::entity_type::zombie:++zc;break;
				case pz::entity_type::player:++pc;break;
				case pz::entity_type::vehicle:++vc;break;
				case pz::entity_type::animal:++ac;break;
				case pz::entity_type::item:++ic;break;
				}
			}
			ImGui::Text("Zombies: %d  Players: %d",zc,pc);
			ImGui::Text("Vehicles: %d  Animals: %d  Items: %d",vc,ac,ic);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Status Cover"),ENCL("Status"));
	}ImGui::Columns(1);
}

// ====================== Tab 1: Visual =============================
void Menu::Visual() {
	ImGuiStyle* style=&ImGui::GetStyle(); InsertSpacer("Top Spacer");
	ImGui::Columns(2,NULL,false);{
		// ---- Left column: Zombie ESP + Animal ESP ----
		InsertGroupBoxLeft(ENCL("Zombie ESP"),240.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Enabled",menu_state::zombie_esp_enabled);
			{float t=menu_state::zombie_esp_max_dist;InsertSlider("Max distance##zd",t,5.f,200.f,"%.0f");menu_state::zombie_esp_max_dist=t;}
			InsertCheckbox("Box##zb",menu_state::zombie_esp_box);
			InsertCheckbox("Name##zn",menu_state::zombie_esp_name);
			InsertCheckbox("Health##zh",menu_state::zombie_esp_health);
			InsertCheckbox("Distance##zsd",menu_state::zombie_esp_show_dist);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Color");
			InsertColorPicker("##zcol",menu_state::zombie_esp_color,true);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Zombie ESP Cover"),ENCL("Zombie ESP"));
		InsertSpacer("ZA Spacer");
		InsertGroupBoxLeft(ENCL("Animal ESP"),240.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Enabled##ae",menu_state::animal_esp_enabled);
			{float t=menu_state::animal_esp_max_dist;InsertSlider("Max distance##ad",t,5.f,200.f,"%.0f");menu_state::animal_esp_max_dist=t;}
			InsertCheckbox("Box##ab",menu_state::animal_esp_box);
			InsertCheckbox("Name##an",menu_state::animal_esp_name);
			InsertCheckbox("Health##ah",menu_state::animal_esp_health);
			InsertCheckbox("Distance##asd",menu_state::animal_esp_show_dist);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Color");
			InsertColorPicker("##acol",menu_state::animal_esp_color,true);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxLeft(ENCL("Animal ESP Cover"),ENCL("Animal ESP"));
	}ImGui::NextColumn();{
		// ---- Right column: Player ESP + Vehicle ESP ----
		InsertGroupBoxRight(ENCL("Player ESP"),240.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Enabled##pe",menu_state::player_esp_enabled);
			{float t=menu_state::player_esp_max_dist;InsertSlider("Max distance##pd",t,5.f,200.f,"%.0f");menu_state::player_esp_max_dist=t;}
			InsertCheckbox("Box##pb",menu_state::player_esp_box);
			InsertCheckbox("Name##pn",menu_state::player_esp_name);
			InsertCheckbox("Health##ph",menu_state::player_esp_health);
			InsertCheckbox("Distance##psd",menu_state::player_esp_show_dist);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Color");
			InsertColorPicker("##pcol",menu_state::player_esp_color,true);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Player ESP Cover"),ENCL("Player ESP"));
		InsertSpacer("PV Spacer");
		InsertGroupBoxRight(ENCL("Vehicle ESP"),240.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			InsertCheckbox("Enabled##ve",menu_state::vehicle_esp_enabled);
			{float t=menu_state::vehicle_esp_max_dist;InsertSlider("Max distance##vd",t,5.f,300.f,"%.0f");menu_state::vehicle_esp_max_dist=t;}
			InsertCheckbox("Box##vb",menu_state::vehicle_esp_box);
			InsertCheckbox("Name##vn",menu_state::vehicle_esp_name);
			InsertCheckbox("Distance##vsd",menu_state::vehicle_esp_show_dist);
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Color");
			InsertColorPicker("##vcol",menu_state::vehicle_esp_color,true);
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Vehicle ESP Cover"),ENCL("Vehicle ESP"));
	}ImGui::Columns(1);
	// ---- Item ESP (full width below) ----
	InsertSpacer("VI Spacer");
	InsertGroupBoxTop(ENCL("Item ESP"),ImVec2(530.f,160.f));{
		style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
		InsertCheckbox("Enabled##ie",menu_state::item_esp_enabled);
		{float t=menu_state::item_esp_max_dist;InsertSlider("Max distance##id",t,5.f,100.f,"%.0f");menu_state::item_esp_max_dist=t;}
		InsertCheckbox("Name##in",menu_state::item_esp_name);
		InsertCheckbox("Distance##isd",menu_state::item_esp_show_dist);
		ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(CHECKBOX_LABEL_X);ImGui::Text("Color");
		InsertColorPicker("##icol",menu_state::item_esp_color,true);
		style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
	}InsertEndGroupBoxTop(ENCL("Item ESP Cover"),ENCL("Item ESP"),ImVec2(530.f,11.f));
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
			// Add to Inventory (instant, default stats)
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);
			if(!ok)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
			if(ImGui::Button(ENCL("Add to Inventory"),ImVec2(230.f,26.f))&&ok)
				pz::spawn_item(spawner::view[spawner::selected].full);
			if(!ok)ImGui::PopStyleVar();
			// Spawn Custom (condition + ammo)
			ImGui::Spacing();ImGui::NewLine();ImGui::SameLine(10.f);
			if(!ok)ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.45f);
			if(ImGui::Button(ENCL("Spawn Custom"),ImVec2(230.f,26.f))&&ok)
				pz::spawn_item_custom(spawner::view[spawner::selected].full,spawner::s_cond,spawner::s_ammo);
			if(!ok)ImGui::PopStyleVar();
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Spawn Cover"),ENCL("Spawn"));
		InsertSpacer("S-I Spacer");
		InsertGroupBoxRight(ENCL("Info"),168.f);{
			style->ItemSpacing=ImVec2(4,2);style->WindowPadding=ImVec2(4,4);ImGui::CustomSpacing(9.f);
			ImGui::TextDisabled("Add = instant, default stats");
			ImGui::TextDisabled("Custom = set condition + ammo");
			ImGui::Spacing();ImGui::Text("Game: %s",g_game_ready?"ready":"waiting...");
			style->ItemSpacing=ImVec2(0,0);style->WindowPadding=ImVec2(6,6);
		}InsertEndGroupBoxRight(ENCL("Info Cover"),ENCL("Info"));
	}ImGui::Columns(1);
}

// ====================== Config IO (Elden Ring / cs2-6s pattern) ========
namespace config_io {
	struct config_data {
		bool full_bright, night_vision, zombie_ignore, god_mode;
		bool anti_hunger, anti_encumbrance, anti_thirst;
		bool auto_heal;
		bool ze_en; float ze_dist; bool ze_box, ze_name, ze_hp, ze_sd; float ze_col[4];
		bool pe_en; float pe_dist; bool pe_box, pe_name, pe_hp, pe_sd; float pe_col[4];
		bool ve_en; float ve_dist; bool ve_box, ve_name, ve_sd; float ve_col[4];
		bool ae_en; float ae_dist; bool ae_box, ae_name, ae_hp, ae_sd; float ae_col[4];
		bool ie_en; float ie_dist; bool ie_name, ie_sd; float ie_col[4];
		bool esp_render; float menu_col[4]; int menu_key;
	};
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
		d.anti_hunger=menu_state::anti_hunger; d.anti_encumbrance=menu_state::anti_encumbrance; d.anti_thirst=menu_state::anti_thirst;
		d.auto_heal=menu_state::auto_heal;
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
		menu_state::anti_hunger=d.anti_hunger; menu_state::anti_encumbrance=d.anti_encumbrance; menu_state::anti_thirst=d.anti_thirst;
		menu_state::auto_heal=d.auto_heal;
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
		config_data d{}; auto r=fread(&d,1,sizeof(d),f); fclose(f); if(r<sizeof(d))return;
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
				menu_state::full_bright=menu_state::zombie_ignore=menu_state::god_mode=false;
				menu_state::anti_hunger=menu_state::anti_encumbrance=menu_state::anti_thirst=false;
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
	ImDrawList*bg=ImGui::GetBackgroundDrawList();
	ImDrawList*fg=ImGui::GetForegroundDrawList();
	if(menu_state::esp_render_enabled&&g_game_ready){
		for(auto&e:g_entities){
			if(e.is_local||!e.on_screen)continue;
			const bool z=(e.type==pz::entity_type::zombie);
			const bool p=(e.type==pz::entity_type::player);
			const bool v=(e.type==pz::entity_type::vehicle);
			const bool a=(e.type==pz::entity_type::animal);
			const bool it=(e.type==pz::entity_type::item);

			bool enabled=false; float max_d=0; bool show_box=false,show_name=false,show_hp=false,show_dist=false;
			const float*cc=nullptr;
			if(z){enabled=menu_state::zombie_esp_enabled;max_d=menu_state::zombie_esp_max_dist;show_box=menu_state::zombie_esp_box;show_name=menu_state::zombie_esp_name;show_hp=menu_state::zombie_esp_health;show_dist=menu_state::zombie_esp_show_dist;cc=menu_state::zombie_esp_color;}
			else if(p){enabled=menu_state::player_esp_enabled;max_d=menu_state::player_esp_max_dist;show_box=menu_state::player_esp_box;show_name=menu_state::player_esp_name;show_hp=menu_state::player_esp_health;show_dist=menu_state::player_esp_show_dist;cc=menu_state::player_esp_color;}
			else if(v){enabled=menu_state::vehicle_esp_enabled;max_d=menu_state::vehicle_esp_max_dist;show_box=menu_state::vehicle_esp_box;show_name=menu_state::vehicle_esp_name;show_hp=false;show_dist=menu_state::vehicle_esp_show_dist;cc=menu_state::vehicle_esp_color;}
			else if(a){enabled=menu_state::animal_esp_enabled;max_d=menu_state::animal_esp_max_dist;show_box=menu_state::animal_esp_box;show_name=menu_state::animal_esp_name;show_hp=menu_state::animal_esp_health;show_dist=menu_state::animal_esp_show_dist;cc=menu_state::animal_esp_color;}
			else if(it){enabled=menu_state::item_esp_enabled;max_d=menu_state::item_esp_max_dist;show_box=false;show_name=menu_state::item_esp_name;show_hp=false;show_dist=menu_state::item_esp_show_dist;cc=menu_state::item_esp_color;}
			if(!enabled||e.dist>max_d||!cc)continue;

			ImU32 col=IM_COL32((int)(cc[0]*255),(int)(cc[1]*255),(int)(cc[2]*255),(int)(cc[3]*255));
			ImU32 shadow=IM_COL32(0,0,0,180);
			float bw=14.0f, bh=(z||p)?56.0f:v?40.0f:a?36.0f:0.0f;

			// Cornered box (CS2 style)
			if(show_box&&bh>0){
				float x1=e.sx-bw,y1=e.sy-bh,x2=e.sx+bw,y2=e.sy;
				float cl=std::min(10.0f,(x2-x1)*0.3f);
				bg->AddLine(ImVec2(x1,y1),ImVec2(x1+cl,y1),col,2.f);
				bg->AddLine(ImVec2(x1,y1),ImVec2(x1,y1+cl),col,2.f);
				bg->AddLine(ImVec2(x2,y1),ImVec2(x2-cl,y1),col,2.f);
				bg->AddLine(ImVec2(x2,y1),ImVec2(x2,y1+cl),col,2.f);
				bg->AddLine(ImVec2(x1,y2),ImVec2(x1+cl,y2),col,2.f);
				bg->AddLine(ImVec2(x1,y2),ImVec2(x1,y2-cl),col,2.f);
				bg->AddLine(ImVec2(x2,y2),ImVec2(x2-cl,y2),col,2.f);
				bg->AddLine(ImVec2(x2,y2),ImVec2(x2,y2-cl),col,2.f);
				// Health bar left side
				if(show_hp&&e.health>0){
					float fill=std::clamp(e.health/100.0f,0.0f,1.0f);
					float bx=x1-6;
					bg->AddRectFilled(ImVec2(bx-2,y1),ImVec2(bx,y2),IM_COL32(0,0,0,120));
					ImU32 bc=IM_COL32((int)((1.f-fill)*255),(int)(fill*255),0,255);
					bg->AddRectFilled(ImVec2(bx-2,y1+bh*(1-fill)),ImVec2(bx,y2),bc);
				}
			}
			// Name centered above
			float ty=e.sy-bh-4;
			if(show_name){
				ImVec2 ts=ImGui::CalcTextSize(e.name);
				float tx=e.sx-ts.x*0.5f;
				bg->AddText(ImVec2(tx+1,ty+1),shadow,e.name);
				bg->AddText(ImVec2(tx,ty),IM_COL32(255,255,255,230),e.name);
				ty-=14;
			}
			// Distance centered below
			if(show_dist){
				char d[32];_snprintf_s(d,sizeof(d),_TRUNCATE,"%.0fm",e.dist);
				ImVec2 ds=ImGui::CalcTextSize(d);
				float dx=e.sx-ds.x*0.5f;
				float dy=e.sy+4;
				bg->AddText(ImVec2(dx+1,dy+1),shadow,d);
				bg->AddText(ImVec2(dx,dy),IM_COL32(200,200,200,200),d);
			}
		}
	}
	{// Watermark
		const char*t="pz-int";ImVec2 ts=ImGui::CalcTextSize(t);
		float w=ts.x+16,h=ts.y+16;ImVec2 p(20,20);
		fg->AddRectFilledMultiColor(p,ImVec2(p.x+w/2,p.y+2),ImColor(55,177,218),ImColor(201,84,192),ImColor(201,84,192),ImColor(55,177,218));
		fg->AddRectFilledMultiColor(ImVec2(p.x+w/2,p.y),ImVec2(p.x+w,p.y+2),ImColor(201,84,192),ImColor(204,227,54),ImColor(204,227,54),ImColor(201,84,192));
		fg->AddRectFilled(ImVec2(p.x,p.y+2),ImVec2(p.x+w,p.y+h),ImColor(17,17,17,255));
		fg->AddRect(p,ImVec2(p.x+w,p.y+h),ImColor(60,60,60,255),0,0,1);
		fg->AddText(ImVec2(p.x+9,p.y+9),ImColor(0,0,0,180),t);
		fg->AddText(ImVec2(p.x+8,p.y+8),ImColor(255,255,255,255),t);
	}
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
		status("Container Sync (MP)", d.container_sync_ok);
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
		ImGui::Text("ESP render: %s", menu_state::esp_render_enabled?"ON":"off");
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
