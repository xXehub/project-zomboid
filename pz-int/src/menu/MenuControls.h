#pragma once
// ============================================================================
// MenuControls.h - pz-int menu control macros.
//
// Ported from cs2-6s main/menu/MenuControls.h. The Insert* macro family and
// the key picker are kept 1:1 so the tab code reads like the original skeet
// menu. What was dropped: the cs2-6s settings.hpp / zdraw shim / Config.h
// dependencies - pz-int has no such stack, so:
//   * zdraw::rgba is defined locally (simple 8-bit RGBA byte struct, the
//     exact fields ImRgbaPicker in Menu.cpp reads).
//   * The keybind_mode enum is defined locally instead of being shared
//     with core/settings.hpp.
// ============================================================================

#include "Menu.h"
#include "imgui.h"
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <unordered_map>

// Shared keybind mode (local equivalent of cs2-6s settings::keybind_mode).
namespace settings {
    enum class keybind_mode : std::uint8_t {
        hold    = 0,
        toggle  = 1,
        always  = 2,
    };
}

// Local zdraw::rgba stand-in (cs2-6s pulls this from the zdraw shim).
namespace zdraw {
    struct rgba {
        std::uint8_t r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 };
    };
}

// File-scope style pointer used by the Insert* macros.
// Must NOT call ImGui::GetStyle() at static-init time (DLL load) because
// ImGui::CreateContext() hasn't run yet. Resolved lazily on first use.
inline ImGuiStyle* style = nullptr;
inline ImGuiStyle* GetStyle() { if (!style) style = &ImGui::GetStyle(); return style; }

// =====================================
// - Custom controls
// =====================================

#define InsertSpacer(x1) style->Colors[ImGuiCol_ChildBg] = ImColor(0, 0, 0, 0); ImGui::BeginChild(x1, ImVec2(210.f, 18.f), false); {} ImGui::EndChild(); style->Colors[ImGuiCol_ChildBg] = ImColor(49, 49, 49, 255);
#define InsertGroupboxSpacer(x1) style->Colors[ImGuiCol_ChildBg] = ImColor(0, 0, 0, 0); ImGui::BeginChild(x1, ImVec2(210.f, 9.f), false); {} ImGui::EndChild(); style->Colors[ImGuiCol_ChildBg] = ImColor(49, 49, 49, 255);
#define InsertGroupboxTitle(x1) ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(11.f); ImGui::GroupBoxTitle(x1);

#define InsertGroupBoxLeft(x1,x2) ImGui::NewLine(); ImGui::SameLine(19.f); ImGui::BeginGroupBox(x1, ImVec2(258.f, x2), true);
#define InsertGroupBoxRight(x1,x2) ImGui::NewLine(); ImGui::SameLine(10.f); ImGui::BeginGroupBox(x1, ImVec2(258.f, x2), true);
#define InsertEndGroupBoxLeft(x1,x2) ImGui::EndGroupBox(); ImGui::SameLine(19.f); ImGui::BeginGroupBoxScroll(x1, x2, ImVec2(258.f, 11.f), true); ImGui::EndGroupBoxScroll();
#define InsertEndGroupBoxRight(x1,x2) ImGui::EndGroupBox(); ImGui::SameLine(10.f); ImGui::BeginGroupBoxScroll(x1, x2, ImVec2(258.f, 11.f), true); ImGui::EndGroupBoxScroll();

#define InsertGroupBoxTop(x1,x2) ImGui::NewLine(); ImGui::SameLine(19.f); ImGui::BeginGroupBox(x1, x2, true);
#define InsertEndGroupBoxTop(x1,x2,x3) ImGui::EndGroupBox(); ImGui::SameLine(19.f); ImGui::BeginGroupBoxScroll(x1, x2, x3, true); ImGui::EndGroupBoxScroll();

// =====================================
// - Default controls
// =====================================

#define InsertCheckbox(x1,x2) ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(19.f); ImGui::Checkbox(x1, &x2);
#define InsertSlider(x1,x2,x3,x4,x5) ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(159.f); ImGui::SliderFloat(x1, &x2, x3, x4, x5); ImGui::PopItemWidth();
#define InsertSliderWithoutText(x1,x2,x3,x4,x5) ImGui::Spacing(); ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(159.f); ImGui::SliderFloat(x1, &x2, x3, x4, x5); ImGui::PopItemWidth();
#define InsertCombo(x1,x2,x3) ImGui::Spacing(); ImGui::NewLine(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(158.f); ImGui::Combo(x1, &x2, x3, IM_ARRAYSIZE(x3)); ImGui::PopItemWidth(); ImGui::CustomSpacing(1.f);
#define InsertComboWithoutText(x1,x2,x3) ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(158.f); ImGui::Combo(x1, &x2, x3, IM_ARRAYSIZE(x3)); ImGui::PopItemWidth(); ImGui::CustomSpacing(1.f);
#define InsertMultiCombo(x1,x2,x3,x4) ImGui::Spacing(); ImGui::NewLine(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(158.f); ImGui::MultiCombo(x1, x2, x3, x4); ImGui::PopItemWidth(); ImGui::CustomSpacing(1.f);
#define InsertMultiComboWithoutText(x1,x2,x3,x4) ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(CHECKBOX_LABEL_X); ImGui::PushItemWidth(158.f); ImGui::MultiCombo(x1, x2, x3, x4); ImGui::PopItemWidth(); ImGui::CustomSpacing(1.f);

#define InsertColorPicker(x1,x2,x3) ImGui::SameLine(219.f); Menu::ColorPicker(x1, x2, x3);

// zdraw::rgba color picker bridge (defined in Menu.cpp).
void ImRgbaPicker(const char* name, zdraw::rgba& c, bool alpha);

// Key binding mode live-state evaluator (hold / toggle / always). Ported
// unchanged from cs2-6s.
inline bool keybind_active(int vk, settings::keybind_mode mode, const void* owner)
{
    using M = settings::keybind_mode;
    if (mode == M::always) return true;
    if (vk == 0) return false;

    const bool down = (::GetAsyncKeyState(vk) & 0x8000) != 0;

    if (mode == M::hold) {
        return down;
    }

    // toggle
    struct slot { bool last_down = false; bool latched = false; };
    static std::unordered_map<const void*, slot> states;
    auto& st = states[owner];
    if (down && !st.last_down) {
        st.latched = !st.latched;
    }
    st.last_down = down;
    return st.latched;
}

inline const char* keybind_mode_name(settings::keybind_mode m)
{
    switch (m) {
    case settings::keybind_mode::hold:   return "H";
    case settings::keybind_mode::toggle: return "T";
    case settings::keybind_mode::always: return "A";
    }
    return "?";
}

// ============================================================
// Label alignment tuning knob (ported 1:1 from cs2-6s)
// ============================================================
inline constexpr float CHECKBOX_LABEL_X = 45.0f;

// Inline checkbox + color picker on the same row (ported from cs2-6s).
inline void InsertCheckboxColor(const char* label, bool& flag, zdraw::rgba& c, bool alpha, const char* unique_id)
{
    ImGui::Spacing();
    ImGui::NewLine();
    ImGui::SameLine(19.f);
    ImGui::Checkbox(label, &flag);
    ImRgbaPicker(unique_id, c, alpha);
}

// Sub-label + color picker row (ported from cs2-6s).
inline void InsertLabelColor(const char* label, zdraw::rgba& c, bool alpha, const char* unique_id)
{
    ImGui::Spacing();
    ImGui::NewLine();
    ImGui::SameLine(CHECKBOX_LABEL_X);
    ImGui::Text("%s", label);
    ImRgbaPicker(unique_id, c, alpha);
}

// =====================================
// - Key picker (ported 1:1 from cs2-6s)
// =====================================
inline const char* keypicker_vk_name(int vk)
{
    static char buf[16];
    switch (vk) {
    case 0:               return "[NONE]";
    case VK_LBUTTON:      return "[M1]";
    case VK_RBUTTON:      return "[M2]";
    case VK_MBUTTON:      return "[M3]";
    case VK_XBUTTON1:     return "[M4]";
    case VK_XBUTTON2:     return "[M5]";
    case VK_LSHIFT: case VK_SHIFT:       return "[SHIFT]";
    case VK_RSHIFT:                       return "[RSHIFT]";
    case VK_LCONTROL: case VK_CONTROL:   return "[CTRL]";
    case VK_RCONTROL:                     return "[RCTRL]";
    case VK_LMENU: case VK_MENU:         return "[ALT]";
    case VK_RMENU:                        return "[RALT]";
    case VK_TAB:          return "[TAB]";
    case VK_SPACE:        return "[SPACE]";
    case VK_BACK:         return "[BACK]";
    case VK_RETURN:       return "[ENTER]";
    case VK_CAPITAL:      return "[CAPS]";
    case VK_INSERT:       return "[INS]";
    case VK_DELETE:       return "[DEL]";
    case VK_HOME:         return "[HOME]";
    case VK_END:          return "[END]";
    case VK_PRIOR:        return "[PGUP]";
    case VK_NEXT:         return "[PGDN]";
    case VK_LEFT:         return "[LEFT]";
    case VK_RIGHT:        return "[RIGHT]";
    case VK_UP:           return "[UP]";
    case VK_DOWN:         return "[DOWN]";
    case VK_OEM_MINUS:    return "[-]";
    case VK_OEM_PLUS:     return "[+]";
    case VK_OEM_COMMA:    return "[,]";
    case VK_OEM_PERIOD:   return "[.]";
    }
    if (vk >= VK_F1 && vk <= VK_F24) { sprintf_s(buf, "[F%d]", vk - VK_F1 + 1); return buf; }
    if (vk >= 0x30 && vk <= 0x39)    { sprintf_s(buf, "[%c]", (char)vk); return buf; }
    if (vk >= 0x41 && vk <= 0x5A)    { sprintf_s(buf, "[%c]", (char)vk); return buf; }
    sprintf_s(buf, "[0x%02X]", vk);
    return buf;
}

// Key picker with optional mode menu (ported 1:1 from cs2-6s, minus the
// g_Config dependency: the popup's accent color comes from the local
// menu color state instead).
inline void InsertKeypicker(const char* label, int& out_vk, settings::keybind_mode* out_mode = nullptr)
{
    ImGui::Spacing();
    ImGui::NewLine();

    ImGui::SameLine(CHECKBOX_LABEL_X);
    ImGui::Text("%s", label);
    ImGui::SameLine(219.f);

    static const int* s_waiting_ptr = nullptr;
    static bool       s_wait_release = false;

    const bool is_waiting = (s_waiting_ptr == &out_vk);

    char disp[32];
    if (is_waiting) {
        _snprintf_s(disp, sizeof(disp), _TRUNCATE, "[...]");
    } else if (out_mode && *out_mode != settings::keybind_mode::hold) {
        _snprintf_s(disp, sizeof(disp), _TRUNCATE, "%s %s",
            keypicker_vk_name(out_vk),
            keybind_mode_name(*out_mode));
    } else {
        _snprintf_s(disp, sizeof(disp), _TRUNCATE, "%s", keypicker_vk_name(out_vk));
    }

    ImFont* font = ImGui::GetFont();
    const float font_size = font->FontSize * 0.75f;
    const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, disp);
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    char btn_id[64];
    _snprintf_s(btn_id, sizeof(btn_id), _TRUNCATE,
        "##kp_%s_%p", label, static_cast<void*>(&out_vk));

    ImGui::InvisibleButton(btn_id, ImVec2(text_size.x + 4.f, text_size.y));

    const bool hovered   = ImGui::IsItemHovered();
    const bool l_clicked = ImGui::IsItemClicked(0);
    const bool r_clicked = ImGui::IsItemClicked(1);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 col = hovered
        ? ImColor(140, 140, 140, 255)
        : ImColor(90,  90,  90, 255);

    dl->AddText(font, font_size, cursor, col, disp);

    if (l_clicked) {
        s_waiting_ptr  = &out_vk;
        s_wait_release = true;
    }

    char popup_id[64];
    _snprintf_s(popup_id, sizeof(popup_id), _TRUNCATE,
        "##kpmode_%s_%p", label, static_cast<void*>(&out_vk));

    if (r_clicked && out_mode) {
        ImGui::OpenPopup(popup_id);
    }

    if (out_mode && ImGui::BeginPopup(popup_id)) {
        using M = settings::keybind_mode;

        // Accent colour for the active mode row (pz-int: read from the
        // local menu color state - cs2-6s reads g_Config.Settings).
        extern float g_menu_accent[ 4 ];
        const ImVec4 accent_v4 = ImVec4(
            g_menu_accent[0],
            g_menu_accent[1],
            g_menu_accent[2],
            1.0f);

        auto mode_row = [&](const char* name, M target) {
            const bool active = (*out_mode == target);

            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Header,        accent_v4);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, accent_v4);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  accent_v4);
            }

            if (ImGui::Selectable(name, active, 0, ImVec2(110.0f, 0.0f))) {
                *out_mode = target;
            }

            if (active) {
                ImGui::PopStyleColor(3);
            }
        };

        mode_row("Hold",   M::hold);
        mode_row("Toggle", M::toggle);
        mode_row("Always", M::always);

        ImGui::EndPopup();
    }

    // Listen loop (two-phase, so M1..M5 are bindable).
    if (is_waiting) {
        if (s_wait_release) {
            if ((::GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
                s_wait_release = false;
            }
            return;
        }

        for (int vk = 0x01; vk <= 0xFE; ++vk) {
            if (vk == VK_CANCEL) continue;
            if ((::GetAsyncKeyState(vk) & 0x8000) == 0) continue;

            if (vk == VK_ESCAPE) {
                s_waiting_ptr = nullptr;
                break;
            }

            out_vk = vk;
            s_waiting_ptr = nullptr;
            ::Sleep(200);
            break;
        }
    }
}
