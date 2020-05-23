#include <Windows.h>
#include <sstream>
#include <iostream>
#include <tuple>
#include <optional>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#include "iat_patcher.hpp"
#include "game.hpp"

HWND* hwndEdit = nullptr;

bool keydown[0x30] = { false };

DWORD WINAPI work(LPVOID) {
    Game game;

    double time = 0;
    double time_step = 1 / 10.0;

    for (;;) {
        int dx = 0, dy = 0;
        if (keydown[VK_LEFT]) dx = -1;
        else if (keydown[VK_RIGHT]) dx = 1;
        if (keydown[VK_UP]) dy = 1;
        else if (keydown[VK_DOWN]) dy = -1;

        game.update(dx, dy);

        auto field = game.field();
        SendMessageW(*hwndEdit, WM_SETTEXT, field.length(), field);

        time += time_step;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(time_step * 1000)));
    }

    return 0;
}

bool hook_GetMessageW(HMODULE module) {
    static decltype(&GetMessageW) original = nullptr;
    return hook_import<decltype(&GetMessageW)>(module,
        "user32.dll", "GetMessageW", original,
        [](LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) -> BOOL {
            // Launch a worker thread once everything is initialized
            static auto initialized = 0l;
            if (InterlockedCompareExchange(&initialized, 1, 0) == 0) {
                auto thread = CreateThread(NULL, 0, work, NULL, 0, NULL);
                if (thread) {
                    CloseHandle(thread);
                }
            }

            // Constantly try to kill the edit control's focus
            //TODO figure out what re-triggers the focus
            SendMessageA(*hwndEdit, WM_KILLFOCUS, 0, 0);

            auto result = original(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

            // Handle keyboard messages
            if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) {
                if (lpMsg->wParam < 0x30) {
                    keydown[lpMsg->wParam] = lpMsg->message == WM_KEYDOWN;
                }
            }

            // Block keyboard and mouse messages
            if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP ||
                lpMsg->message == WM_MOUSEMOVE ||
                lpMsg->message == WM_LBUTTONDOWN || lpMsg->message == WM_LBUTTONUP ||
                lpMsg->message == WM_RBUTTONDOWN || lpMsg->message == WM_RBUTTONUP ||
                lpMsg->message == WM_LBUTTONDBLCLK || lpMsg->message == WM_RBUTTONDBLCLK) {
                lpMsg->message = WM_NULL;
            }

            return result;
        });
}

void initialize_values() {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));

    //TODO load from PDB
    hwndEdit = reinterpret_cast<HWND*>(base_addr + 0x2'd368);
}

void install_hooks() {
    auto module = GetModuleHandle(NULL);

    hook_GetMessageW(module);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Ignore thread notifications
        DisableThreadLibraryCalls(hinstDLL);

        initialize_values();
        install_hooks();
    }

    return TRUE;
}
