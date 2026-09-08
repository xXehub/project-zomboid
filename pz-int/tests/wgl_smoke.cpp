#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include "../src/projection.h"

#pragma comment(lib, "opengl32.lib")
namespace gl_ext {
constexpr GLenum framebuffer = 0x8D40;
constexpr GLenum framebuffer_binding = 0x8CA6;
constexpr GLenum color_attachment0 = 0x8CE0;
using gen_framebuffers_fn = void(APIENTRY*)(GLsizei, GLuint*);
using bind_framebuffer_fn = void(APIENTRY*)(GLenum, GLuint);
using framebuffer_texture_2d_fn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);

gen_framebuffers_fn gen_framebuffers{};
bind_framebuffer_fn bind_framebuffer{};
framebuffer_texture_2d_fn framebuffer_texture_2d{};

bool load()
{
    gen_framebuffers = reinterpret_cast<gen_framebuffers_fn>(::wglGetProcAddress("glGenFramebuffers"));
    bind_framebuffer = reinterpret_cast<bind_framebuffer_fn>(::wglGetProcAddress("glBindFramebuffer"));
    framebuffer_texture_2d = reinterpret_cast<framebuffer_texture_2d_fn>(
        ::wglGetProcAddress("glFramebufferTexture2D"));
    return gen_framebuffers && bind_framebuffer && framebuffer_texture_2d;
}
} // namespace gl_ext
namespace wgl_ext {
constexpr int context_major_version = 0x2091;
constexpr int context_minor_version = 0x2092;
constexpr int context_profile_mask = 0x9126;
constexpr int context_core_profile_bit = 0x00000001;
using create_context_attribs_fn = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
} // namespace wgl_ext

namespace {

std::string read_log_for_process(const std::filesystem::path& path, DWORD process_id)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    const std::string contents{
        std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    const std::string marker = "pz-int init start pid=" + std::to_string(process_id);
    const std::size_t marker_position = contents.rfind(marker);
    return marker_position == std::string::npos ? std::string{} : contents.substr(marker_position);
}

bool contains(const std::string& text, const char* needle)
{
    return text.find(needle) != std::string::npos;
}

bool projection_contract_holds()
{
    const auto identity = pz::projection::from_exact(526.5f, 594.0f, 1.0f);
    const auto zoomed = pz::projection::from_exact(1948.75f, 848.5f, 2.0f);
    const auto invalid = pz::projection::from_exact(10.0f, 20.0f, 0.0f);
    const auto player_box = pz::projection::esp_box_for(
        pz::projection::esp_kind::humanoid, 320.0f, 240.0f, 2.0f);
    const auto vehicle_box = pz::projection::esp_box_for(
        pz::projection::esp_kind::vehicle, 320.0f, 240.0f, 0.5f);
    const auto text_y = pz::projection::label_above(player_box.top, 12.0f);
    return identity.x == 526.5f && identity.y == 594.0f &&
        zoomed.x == 974.375f && zoomed.y == 424.25f &&
        invalid.x == 10.0f && invalid.y == 20.0f &&
        player_box.left == 313.0f && player_box.right == 327.0f &&
        player_box.top == 212.0f && player_box.bottom == 240.0f &&
        player_box.label_y == 204.0f && text_y == 192.0f &&
        vehicle_box.left == 292.0f && vehicle_box.right == 348.0f &&
        vehicle_box.top == 160.0f && vehicle_box.bottom == 240.0f &&
        vehicle_box.label_y == 152.0f;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (!projection_contract_holds()) {
        std::cerr << "projection zoom contract failed\n";
        return 1;
    }

    const std::filesystem::path dll_path = argc > 1
        ? std::filesystem::absolute(argv[1])
        : std::filesystem::absolute(L"release\\pz-int.dll");

    wchar_t temp[MAX_PATH]{};
    const DWORD temp_length = ::GetTempPathW(MAX_PATH, temp);
    if (temp_length == 0 || temp_length >= MAX_PATH) {
        std::cerr << "GetTempPathW failed\n";
        return 2;
    }
    const std::filesystem::path log_path = std::filesystem::path{temp} / L"pzint.log";
    const DWORD process_id = ::GetCurrentProcessId();

    const wchar_t* class_name = L"pzint-wgl-smoke";
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = ::DefWindowProcW;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = class_name;
    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::cerr << "RegisterClassW failed\n";
        return 2;
    }

    HWND window = ::CreateWindowExW(0, class_name, L"pzint WGL smoke",
        WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, nullptr, nullptr, wc.hInstance, nullptr);
    if (!window) {
        std::cerr << "CreateWindowExW failed\n";
        return 2;
    }
    ::ShowWindow(window, SW_SHOW);

    HDC dc = ::GetDC(window);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    const int format = ::ChoosePixelFormat(dc, &pfd);
    if (format == 0 || !::SetPixelFormat(dc, format, &pfd)) {
        std::cerr << "OpenGL pixel format setup failed\n";
        return 2;
    }

    HGLRC bootstrap_context = ::wglCreateContext(dc);
    if (!bootstrap_context || !::wglMakeCurrent(dc, bootstrap_context)) {
        std::cerr << "bootstrap WGL context setup failed\n";
        return 2;
    }

    const auto create_context_attribs = reinterpret_cast<wgl_ext::create_context_attribs_fn>(
        ::wglGetProcAddress("wglCreateContextAttribsARB"));
    if (!create_context_attribs) {
        std::cerr << "wglCreateContextAttribsARB unavailable\n";
        return 2;
    }
    const int context_attributes[] = {
        wgl_ext::context_major_version, 4,
        wgl_ext::context_minor_version, 6,
        wgl_ext::context_profile_mask, wgl_ext::context_core_profile_bit,
        0 };
    HGLRC context = create_context_attribs(dc, nullptr, context_attributes);
    if (!context) {
        std::cerr << "OpenGL 4.6 core context creation failed\n";
        return 2;
    }
    ::wglMakeCurrent(nullptr, nullptr);
    ::wglDeleteContext(bootstrap_context);
    if (!::wglMakeCurrent(dc, context)) {
        std::cerr << "OpenGL core context activation failed\n";
        return 2;
    }

    RECT client{};
    ::GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    ::glViewport(0, 0, width, height);
    ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    ::glClear(GL_COLOR_BUFFER_BIT);
    ::SwapBuffers(dc);
    if (!gl_ext::load()) {
        std::cerr << "OpenGL framebuffer functions unavailable\n";
        return 2;
    }

    constexpr int internal_width = 320;
    constexpr int internal_height = 240;
    GLuint internal_texture{};
    ::glGenTextures(1, &internal_texture);
    ::glBindTexture(GL_TEXTURE_2D, internal_texture);
    ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, internal_width, internal_height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLuint internal_framebuffer{};
    gl_ext::gen_framebuffers(1, &internal_framebuffer);
    gl_ext::bind_framebuffer(gl_ext::framebuffer, internal_framebuffer);
    gl_ext::framebuffer_texture_2d(gl_ext::framebuffer, gl_ext::color_attachment0,
        GL_TEXTURE_2D, internal_texture, 0);

	const auto prepare_pz_state = [&] {
		gl_ext::bind_framebuffer(gl_ext::framebuffer, 0);
		::glDrawBuffer(GL_BACK);
		::glViewport(0, 0, width, height);
		::glClear(GL_COLOR_BUFFER_BIT);
		gl_ext::bind_framebuffer(gl_ext::framebuffer, internal_framebuffer);
		::glDrawBuffer(gl_ext::color_attachment0);
		::glViewport(0, 0, internal_width, internal_height);
		::glClear(GL_COLOR_BUFFER_BIT);
	};
	const auto swap_target = reinterpret_cast<const unsigned char*>(
		::GetProcAddress(::GetModuleHandleW(L"gdi32.dll"), "SwapBuffers"));
	if (!swap_target) {
		std::cerr << "SwapBuffers export missing\n";
		return 2;
	}
	std::array<unsigned char, 16> original_bytes{};
	std::copy_n(swap_target, original_bytes.size(), original_bytes.begin());

    HMODULE dll = ::LoadLibraryW(dll_path.c_str());
    if (!dll) {
        std::cerr << "LoadLibraryW failed: " << ::GetLastError() << '\n';
        return 2;
    }

    // Give the bootstrap thread time to install its hook. The fixed build
    // must not wait for wglGetCurrentContext() on that unrelated thread.
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
	const bool hook_bytes_changed = !std::equal(
		original_bytes.begin(), original_bytes.end(), swap_target);

    // Open the menu while SwapBuffers executes so the framebuffer contains
    // real ImGui geometry, not merely an initialized but unused context.
    ::keybd_event(VK_INSERT, 0, 0, 0);
    for (int frame = 0; frame < 4; ++frame) {
        prepare_pz_state();
        ::SwapBuffers(dc);
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }
    ::keybd_event(VK_INSERT, 0, KEYEVENTF_KEYUP, 0);

    for (int frame = 0; frame < 8; ++frame) {
        prepare_pz_state();
        ::SwapBuffers(dc);
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    gl_ext::bind_framebuffer(gl_ext::framebuffer, 0);
    ::glReadBuffer(GL_FRONT);
    ::glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const std::size_t colored_pixels = static_cast<std::size_t>(std::count_if(
        pixels.begin(), pixels.end(), [](unsigned char channel) { return channel != 0 && channel != 255; }));

    const std::string log = read_log_for_process(log_path, process_id);
    const bool hook_seen = contains(log, "SwapBuffers hook installed");
    const bool surface_seen = contains(log, "render surface selected");
    const bool imgui_seen = contains(log, "imgui initialized");
    const bool rendered = colored_pixels > 1000;

    std::cout << "hook=" << hook_seen
              << " surface=" << surface_seen
              << " imgui=" << imgui_seen
              << " patched=" << hook_bytes_changed
              << " colored_channels=" << colored_pixels;

    // END requests renderer teardown on this WGL thread. Continue presenting
    // until the bootstrap thread disables MinHook and releases the DLL.
    ::keybd_event(VK_END, 0, 0, 0);
    prepare_pz_state();
    ::SwapBuffers(dc);
    ::keybd_event(VK_END, 0, KEYEVENTF_KEYUP, 0);
    bool unloaded = false;
    bool hook_bytes_restored = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        prepare_pz_state();
        ::SwapBuffers(dc);
        hook_bytes_restored = std::equal(
            original_bytes.begin(), original_bytes.end(), swap_target);
        unloaded = ::GetModuleHandleW(dll_path.filename().c_str()) == nullptr;
        if (unloaded && hook_bytes_restored) break;
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    std::cout << " restored=" << hook_bytes_restored
              << " unloaded=" << unloaded << '\n';

    ::wglMakeCurrent(nullptr, nullptr);
    ::wglDeleteContext(context);
    ::ReleaseDC(window, dc);
    ::DestroyWindow(window);

    if (!hook_seen || !surface_seen || !imgui_seen || !hook_bytes_changed ||
        !rendered || !hook_bytes_restored || !unloaded) {
        std::cerr << "WGL smoke failed\n" << log;
        return 1;
    }
    return 0;
}
