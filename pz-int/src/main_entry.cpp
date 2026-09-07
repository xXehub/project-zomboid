#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// ============================================================================
// main_entry.cpp - pz-int internal renderer.
//
// TRUE INTERNAL: hooks SwapBuffers in gdi32.dll (which PZ's LWJGL GL context
// goes through) and renders the ImGui menu DIRECTLY into the game's
// framebuffer. No overlay window. Works in exclusive fullscreen, borderless,
// windowed - everywhere, because we draw inside the game's own present.
//
// Flow:
//   DllMain -> bootstrap thread:
//     1. install an instruction-aware MinHook detour immediately
//     2. wait until SwapBuffers runs on a valid WGL render surface
//     3. initialize ImGui inside that render context
//     4. render the menu/ESP before the game's real SwapBuffers call
// ============================================================================

#include <Windows.h>
#include <GL/gl.h>

#include <atomic>
#include <string>
#include <cstdio>
#include <cstring>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/Icons.h"
#include "../imgui/IconFont.h"
#include "../imgui/xorstr.hpp"
#include "menu/Menu.h"
#include "pz_game.h"
#include <MinHook.h>

#pragma comment(lib, "opengl32.lib")

// ---- pzint logging ----------------------------------------------------------

namespace pzlog {

	inline HANDLE g_file = INVALID_HANDLE_VALUE;

	inline void open()
	{
		if (g_file != INVALID_HANDLE_VALUE)
			return;
		wchar_t temp[MAX_PATH]{};
		const auto length = ::GetTempPathW(MAX_PATH, temp);
		const std::wstring base = (length > 0 && length < MAX_PATH)
			? std::wstring(temp) : std::wstring(L".\\");
		g_file = ::CreateFileW((base + L"pzint.log").c_str(), FILE_APPEND_DATA,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	}

	inline void log(const char* fmt, ...)
	{
		char buf[1024];
		va_list ap;
		va_start(ap, fmt);
		_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
		va_end(ap);

		SYSTEMTIME st{};
		::GetLocalTime(&st);

		char line[1280];
		_snprintf_s(line, sizeof(line), _TRUNCATE,
			"[%02u:%02u:%02u.%03u] %s\n",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buf);

		::OutputDebugStringA(line);
	open();
	if (g_file != INVALID_HANDLE_VALUE) {
		DWORD written{};
		::WriteFile(g_file, line, static_cast<DWORD>(std::strlen(line)), &written, nullptr);
		::FlushFileBuffers(g_file);
	}
	}

	inline void close()
	{
		if (g_file != INVALID_HANDLE_VALUE) {
			::CloseHandle(g_file);
			g_file = INVALID_HANDLE_VALUE;
		}
	}

} // namespace pzlog

// ---- globals -----------------------------------------------------------------

ImFont* menuFont   = nullptr;
ImFont* tabFont    = nullptr;
ImFont* tabFont2   = nullptr;
ImFont* tabFont3   = nullptr;
ImFont* controlFont = nullptr;
ImFont* boldMenuFont = nullptr;

extern std::atomic<bool> g_pzint_unload;
std::atomic<bool> g_pzint_unload{ false };

// ---- hook state ---------------------------------------------------------------

using swap_buffers_fn = BOOL(WINAPI*)(HDC);
static swap_buffers_fn g_real_swap_buffers = nullptr;
static void* g_swap_buffers_target = nullptr;
static std::atomic_uint32_t g_hook_calls{ 0 };
static std::atomic_uint32_t g_active_hook_calls{ 0 };
static std::atomic_bool g_imgui_ready{ false };
static std::atomic_bool g_renderer_shutdown{ false };
static HGLRC g_render_context = nullptr;
static HDC g_render_dc = nullptr;
static HWND g_game_hwnd = nullptr;
static bool g_first_frame_logged = false;

// SEH helpers (keep crashes inside the hook from killing the game).
namespace seh {
	static bool safe_newframe()   { __try { ImGui::NewFrame(); return true; } __except (EXCEPTION_EXECUTE_HANDLER) { return false; } }
	static bool safe_overlay()     { __try { DrawOverlay(); return true; } __except (EXCEPTION_EXECUTE_HANDLER) { return false; } }
	static void safe_menu()         { __try { if (Menu::Get().isOpen) Menu::Get().Render(); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
	static void safe_toggles()      { __try {
		const pz::survival_features features{
			.full_bright = menu_state::full_bright,
			.zombie_ignore = menu_state::zombie_ignore,
			.god_mode = menu_state::god_mode,
			.anti_hunger = menu_state::anti_hunger,
			.anti_encumbrance = menu_state::anti_encumbrance,
			.anti_thirst = menu_state::anti_thirst,
			.auto_heal = menu_state::auto_heal
		};
		pz::apply_survival_features(features);
	} __except (EXCEPTION_EXECUTE_HANDLER) {} }
	static bool safe_shutdown()     { __try { return pz::shutdown(); } __except (EXCEPTION_EXECUTE_HANDLER) { return false; } }
	static void safe_collect()      { __try { pz::collect_entities(g_entities); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
	static bool safe_render()       { __try { ImGui::Render(); return true; } __except (EXCEPTION_EXECUTE_HANDLER) { return false; } }
	static bool safe_gldraw()       { __try { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); return true; } __except (EXCEPTION_EXECUTE_HANDLER) { return false; } }
}

// Lazy one-time ImGui init, called from inside the hook once we have a
// current WGL context on this thread (the game's render thread).
static bool imgui_init_once(HDC hdc, HGLRC context, HWND window, int width, int height)
{
	if (g_imgui_ready.load(std::memory_order_acquire))
		return true;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	ImGui::StyleColorsDark();

	if (!ImGui_ImplOpenGL3_Init()) {
		ImGui::DestroyContext();
		return false;
	}

	menuFont = io.Fonts->AddFontDefault();
	{
		ImFontConfig icon_config;
		icon_config.OversampleH = 2;
		icon_config.OversampleV = 2;
		icon_config.PixelSnapH = true;
		icon_config.GlyphMinAdvanceX = 24.0f;
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		void* const copy = IM_ALLOC(fa_solid_900_ttf_size);
		std::memcpy(copy, fa_solid_900_ttf_data, fa_solid_900_ttf_size);
		tabFont = io.Fonts->AddFontFromMemoryTTF(
			copy, fa_solid_900_ttf_size, 24.0f, &icon_config, icon_ranges);
		if (!tabFont)
			tabFont = menuFont;
	}
	tabFont2 = tabFont3 = controlFont = boldMenuFont = menuFont;

	if (!ImGui_ImplOpenGL3_CreateDeviceObjects()) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	g_render_dc = hdc;
	g_render_context = context;
	g_game_hwnd = window;
	g_imgui_ready.store(true, std::memory_order_release);
	pzlog::log("imgui initialized context=%p gl=%s", context,
		reinterpret_cast<const char*>(::glGetString(GL_VERSION)));
	return true;
}

// Manual input state fed from the game's window messages (see wgl hook
// section below for where the game HWND is found). We poll with
// GetAsyncKeyState + cursor pos every frame instead of hooking WndProc,
// which is simpler and process-local:
static void feed_input(int width, int height)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

	POINT mouse{};
	::GetCursorPos(&mouse);
	if (g_game_hwnd && ::ScreenToClient(g_game_hwnd, &mouse)) {
		RECT client{};
		if (::GetClientRect(g_game_hwnd, &client)) {
			const int client_width = client.right - client.left;
			const int client_height = client.bottom - client.top;
			if (client_width > 0 && client_height > 0) {
				mouse.x = ::MulDiv(mouse.x, width, client_width);
				mouse.y = ::MulDiv(mouse.y, height, client_height);
			}
		}
	}
	io.MousePos = ImVec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
	io.MouseDown[0] = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	io.MouseDown[1] = (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	io.MouseDown[2] = (::GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
	io.MouseWheel = 0.0f;

	io.KeyCtrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

	static bool previous_keys[256]{};
	for (int virtual_key = 0; virtual_key < 256; ++virtual_key) {
		const bool down = (::GetAsyncKeyState(virtual_key) & 0x8000) != 0;
		io.KeysDown[virtual_key] = down;
		if (down && !previous_keys[virtual_key] && virtual_key >= ' ' &&
			virtual_key <= '~' && !io.KeyCtrl) {
			io.AddInputCharacter(static_cast<ImWchar>(virtual_key));
		}
		previous_keys[virtual_key] = down;
	}

	static bool key_map_initialized = false;
	if (!key_map_initialized) {
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';
		key_map_initialized = true;
	}
}

// ---- the hook ----------------------------------------------------------------

namespace {

struct render_surface {
	HGLRC context{};
	HWND window{};
	int width{};
	int height{};
	int viewport_width{};
	int viewport_height{};
};

bool query_render_surface(HDC hdc, render_surface& surface)
{
	surface.context = ::wglGetCurrentContext();
	if (!surface.context || !hdc || ::wglGetCurrentDC() != hdc)
		return false;
	GLint viewport[4]{};
	::glGetIntegerv(GL_VIEWPORT, viewport);
	surface.viewport_width = viewport[2];
	surface.viewport_height = viewport[3];

	surface.window = ::WindowFromDC(hdc);
	if (!surface.window || !::IsWindow(surface.window) || !::IsWindowVisible(surface.window))
		return false;

	RECT client{};
	if (!::GetClientRect(surface.window, &client))
		return false;
	surface.width = client.right - client.left;
	surface.height = client.bottom - client.top;
	if (surface.width < 320 || surface.height < 200)
		return false;

	return true;
}

void shutdown_renderer_on_render_thread()
{
	if (g_renderer_shutdown.load(std::memory_order_acquire))
		return;

	// Disable every menu feature before restoring reversible game state.
	Menu::Get().Shutdown();

	// Retry JNI restoration up to 3 times; each attempt re-enters a JFrame.
	bool jni_ok = false;
	for (int attempt = 0; attempt < 3 && !jni_ok; ++attempt)
		jni_ok = seh::safe_shutdown();

	if (!jni_ok)
		pzlog::log("WARNING: JNI feature restoration incomplete after retries");

	if (!g_imgui_ready.exchange(false, std::memory_order_acq_rel)) {
		g_renderer_shutdown.store(true, std::memory_order_release);
		return;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();
	g_render_context = nullptr;
	g_render_dc = nullptr;
	g_game_hwnd = nullptr;
	g_renderer_shutdown.store(true, std::memory_order_release);
	pzlog::log("renderer and JNI bridge shut down (restored=%s)",
		jni_ok ? "yes" : "PARTIAL");
}

} // namespace

static BOOL call_original_and_leave(HDC hdc)
{
	const swap_buffers_fn original = g_real_swap_buffers;
	const BOOL result = original ? original(hdc) : FALSE;
	g_active_hook_calls.fetch_sub(1, std::memory_order_release);
	return result;
}

static BOOL WINAPI hooked_swapbuffers(HDC hdc)
{
	g_hook_calls.fetch_add(1, std::memory_order_relaxed);
	g_active_hook_calls.fetch_add(1, std::memory_order_acquire);

	__try {
		render_surface surface{};
		if (!query_render_surface(hdc, surface))
			return call_original_and_leave(hdc);
		if (g_pzint_unload.load(std::memory_order_acquire)) {
			if (g_imgui_ready.load(std::memory_order_acquire) &&
				surface.context == g_render_context && hdc == g_render_dc) {
				shutdown_renderer_on_render_thread();
			}
			return call_original_and_leave(hdc);
		}


		if (!g_imgui_ready.load(std::memory_order_acquire)) {
			g_game_hwnd = surface.window;
			if (!g_first_frame_logged) {
				pzlog::log("render surface selected hdc=%p hwnd=%p viewport=%dx%d client=%dx%d",
					hdc, surface.window, surface.viewport_width, surface.viewport_height,
					surface.width, surface.height);
				g_first_frame_logged = true;
			}
			if (!imgui_init_once(hdc, surface.context, surface.window,
				surface.width, surface.height)) {
				pzlog::log("ERROR: imgui initialization failed");
				return call_original_and_leave(hdc);
			}
		}

		// Ignore bootstrap/secondary drawables after choosing the main context.
		if (surface.context != g_render_context || hdc != g_render_dc)
			return call_original_and_leave(hdc);


		feed_input(surface.width, surface.height);

		static bool last_toggle = false;
		static int last_virtual_key = 0;
		static bool last_end = false;
		const auto edge = [](bool& previous, int virtual_key) {
			const bool down = (::GetAsyncKeyState(virtual_key) & 0x8000) != 0;
			const bool fired = down && !previous;
			previous = down;
			return fired;
		};

		if (edge(last_end, VK_END)) {
			pzlog::log("emergency unload requested");
			g_pzint_unload.store(true, std::memory_order_release);
			shutdown_renderer_on_render_thread();
			return call_original_and_leave(hdc);
		}

		const int virtual_key = menu_state::menu_key ? menu_state::menu_key : VK_INSERT;
		if (virtual_key != last_virtual_key) {
			last_toggle = false;
			last_virtual_key = virtual_key;
		}
		if (edge(last_toggle, virtual_key)) {
			Menu::Get().isOpen = !Menu::Get().isOpen;
			pzlog::log("menu toggle vk=0x%02X open=%d", virtual_key,
				Menu::Get().isOpen ? 1 : 0);
		}

		g_game_ready = pz::game_ready();
		if (g_game_ready) {
			seh::safe_collect();
			seh::safe_toggles();
		} else {
			g_entities.clear();
		}

		ImGui_ImplOpenGL3_NewFrame();
		if (seh::safe_newframe()) {
			seh::safe_overlay();
			seh::safe_menu();
			const bool frame_rendered = seh::safe_render();
			if (g_pzint_unload.load(std::memory_order_acquire)) {
				shutdown_renderer_on_render_thread();
				return call_original_and_leave(hdc);
			}
			if (frame_rendered)
				seh::safe_gldraw();
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		pzlog::log("ERROR: exception in SwapBuffers hook code=0x%08X",
			::GetExceptionCode());
	}

	return call_original_and_leave(hdc);
}

static bool hook_swapbuffers()
{
	const MH_STATUS init_status = ::MH_Initialize();
	if (init_status != MH_OK && init_status != MH_ERROR_ALREADY_INITIALIZED) {
		pzlog::log("ERROR: MH_Initialize: %s", ::MH_StatusToString(init_status));
		return false;
	}

	const MH_STATUS create_status = ::MH_CreateHookApiEx(
		L"gdi32.dll", "SwapBuffers", reinterpret_cast<void*>(&hooked_swapbuffers),
		reinterpret_cast<void**>(&g_real_swap_buffers), &g_swap_buffers_target);
	if (create_status != MH_OK) {
		pzlog::log("ERROR: MH_CreateHookApiEx: %s", ::MH_StatusToString(create_status));
		::MH_Uninitialize();
		return false;
	}

	const MH_STATUS enable_status = ::MH_EnableHook(g_swap_buffers_target);
	if (enable_status != MH_OK) {
		pzlog::log("ERROR: MH_EnableHook: %s", ::MH_StatusToString(enable_status));
		::MH_RemoveHook(g_swap_buffers_target);
		::MH_Uninitialize();
		g_swap_buffers_target = nullptr;
		g_real_swap_buffers = nullptr;
		return false;
	}

	pzlog::log("SwapBuffers hook installed target=%p trampoline=%p",
		g_swap_buffers_target, reinterpret_cast<void*>(g_real_swap_buffers));
	return true;
}

static bool unhook_swapbuffers()
{
	if (g_swap_buffers_target) {
		const MH_STATUS disable_status = ::MH_DisableHook(g_swap_buffers_target);
		if (disable_status != MH_OK && disable_status != MH_ERROR_DISABLED) {
			pzlog::log("ERROR: MH_DisableHook: %s", ::MH_StatusToString(disable_status));
			return false;
		}
	}

	// No new detour can enter after MH_DisableHook. Keep the trampoline and
	// this DLL alive until every call that already entered has returned.
	for (int attempt = 0; attempt < 5000 &&
		g_active_hook_calls.load(std::memory_order_acquire) != 0; ++attempt) {
		::Sleep(1);
	}
	if (g_active_hook_calls.load(std::memory_order_acquire) != 0) {
		pzlog::log("ERROR: detour did not drain; keeping module loaded safely");
		return false;
	}

	if (g_swap_buffers_target) {
		const MH_STATUS remove_status = ::MH_RemoveHook(g_swap_buffers_target);
		if (remove_status != MH_OK) {
			pzlog::log("ERROR: MH_RemoveHook: %s", ::MH_StatusToString(remove_status));
			return false;
		}
	}
	const MH_STATUS uninit_status = ::MH_Uninitialize();
	if (uninit_status != MH_OK && uninit_status != MH_ERROR_NOT_INITIALIZED) {
		pzlog::log("ERROR: MH_Uninitialize: %s", ::MH_StatusToString(uninit_status));
		return false;
	}

	g_swap_buffers_target = nullptr;
	g_real_swap_buffers = nullptr;
	pzlog::log("SwapBuffers hook fully removed calls=%u", g_hook_calls.load());
	return true;
}

DWORD WINAPI PzintInitThread(LPVOID parameter)
{
	const auto module = static_cast<HMODULE>(parameter);
	pzlog::log("pz-int init start pid=%u tid=%u", ::GetCurrentProcessId(),
		::GetCurrentThreadId());

	if (!hook_swapbuffers()) {
		pzlog::log("FATAL: SwapBuffers hook installation failed");
		::FreeLibraryAndExitThread(module, 1);
	}

	while (!g_pzint_unload.load(std::memory_order_acquire))
		::Sleep(100);

	// Cleanup must complete on the WGL/JNI owner thread before the detour or
	// module can be released. Button and END requests do this in the same frame.
	if (g_imgui_ready.load(std::memory_order_acquire)) {
		for (int attempt = 0; attempt < 200 &&
			!g_renderer_shutdown.load(std::memory_order_acquire); ++attempt) {
			::Sleep(25);
		}
		if (!g_renderer_shutdown.load(std::memory_order_acquire)) {
			pzlog::log("ERROR: render-thread cleanup did not complete; module retained safely");
			::ExitThread(1);
		}
	}

	if (unhook_swapbuffers()) {
		pzlog::log("pz-int unloaded");
		pzlog::close();
		::FreeLibraryAndExitThread(module, 0);
	}

	pzlog::log("pz-int hook disabled; module retained after drain timeout");
	::ExitThread(1);
}
