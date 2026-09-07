// ============================================================================
// dllmain.cpp - pz-int entry point.
//
// Injected into ProjectZomboid64.exe. DllMain only spawns the init thread
// (loader-lock safe); everything else happens on that thread.
// ============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <atomic>

// Defined in main_entry.cpp.
DWORD WINAPI PzintInitThread(LPVOID module);

// Emergency-unload flag; also written by the hook's END hotkey.
extern std::atomic<bool> g_pzint_unload;

extern "C" __declspec(dllexport)
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH: {
        ::DisableThreadLibraryCalls(hInstance);
        const HANDLE thread = ::CreateThread(
            nullptr, 0, &PzintInitThread, hInstance, 0, nullptr);
        if (!thread)
            return FALSE;
        ::CloseHandle(thread);
        break;
    }

    case DLL_PROCESS_DETACH:
        // FreeLibraryAndExitThread is issued by the bootstrap thread only
        // after the detour is removed. Process termination needs no teardown.
        if (!lpReserved)
            g_pzint_unload.store(true, std::memory_order_release);
        break;
    }
    return TRUE;
}
