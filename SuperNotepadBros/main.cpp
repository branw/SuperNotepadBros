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

HWND hwndNP = nullptr, hwndEdit = nullptr;

bool keydown[0x30] = { false };

DWORD WINAPI work(LPVOID) {
    SetWindowTextW(hwndNP, L"SuperNotepadBros");
    
    // Create a new buffer for our own rendering; we need to leave the existing
    // buffer alive to prevent LoadFile from crashing
    auto new_buffer = LocalAlloc(LMEM_MOVEABLE, 16384);
    if (new_buffer == nullptr) {
        OutputDebugStringA("Failed to LocalReAlloc");
        return 0;
    }

    Game game;

    double time = 0;
    double time_step = 1 / 10.0;

    for (auto i = 0;; i++) {
        // Handle keyboard inputs
        int dx = 0, dy = 0;
        if (keydown[VK_LEFT]) dx = -1;
        else if (keydown[VK_RIGHT]) dx = 1;
        if (keydown[VK_UP]) dy = 1;
        else if (keydown[VK_DOWN]) dy = -1;

        game.update(dx, dy);

        // Copy the field to the buffer
        auto buffer = reinterpret_cast<wchar_t*>(LocalLock(new_buffer));
        if (buffer == nullptr) {
            OutputDebugStringA("Failed to LocalLock");
            return 0;
        }

        auto field = game.field();
        std::copy(field.begin(), field.end(), buffer);

        LocalUnlock(buffer);

        // Notify the control that the buffer changed
        SendMessageW(hwndEdit, EM_SETHANDLE, reinterpret_cast<WPARAM>(buffer), NULL);

        time += time_step;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(time_step * 1000)));
    }
    
    return 0;
}

// Capture and block keyboard and mouse inputs
bool hook_GetMessageW(HMODULE module) {
    static decltype(&GetMessageW) original = nullptr;
    return hook_import<decltype(original)>(module,
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
            SendMessageA(hwndEdit, WM_KILLFOCUS, 0, 0);

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

// Capture opened files
bool hook_SendMessageW(HMODULE module) {
    static decltype(&SendMessageW) original = nullptr;
    return hook_import<decltype(original)>(module,
        "user32.dll", "SendMessageW", original,
        [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (hWnd == hwndEdit && Msg == EM_SETHANDLE) {
                auto mem = reinterpret_cast<HLOCAL>(wParam);
                auto buffer = reinterpret_cast<wchar_t*>(LocalLock(mem));

                //TODO handle loaded file

                LocalUnlock(mem);

                return NULL;
            }

            return original(hWnd, Msg, wParam, lParam);
        });
}

// Capture window handles
bool hook_CreateWindowExW(HMODULE module) {
    static decltype(&CreateWindowExW) original = nullptr;
    return hook_import<decltype(original)>(module,
        "user32.dll", "CreateWindowExW", original,
        [](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
            int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
            HINSTANCE hInstance, LPVOID lpParam) -> HWND {
                auto result = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
                    X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
                
                if (!lstrcmpW(lpClassName, L"Notepad")) {
                    hwndNP = result;
                }
                else if (!lstrcmpW(lpClassName, L"Edit")) {
                    hwndEdit = result;
                }

                return result;
        });
}

// Block window title updates
bool hook_SetWindowTextW(HMODULE module) {
    static decltype(&SetWindowTextW) original = nullptr;
    return hook_import<decltype(original)>(module,
        "user32.dll", "SetWindowTextW", original,
        [](HWND hWnd, LPCWSTR lpString) -> BOOL {
            return FALSE;
        });
}

void install_hooks() {
    auto module = GetModuleHandle(NULL);

    hook_CreateWindowExW(module);
    hook_SendMessageW(module);
    hook_GetMessageW(module);
    hook_SetWindowTextW(module);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Ignore thread notifications
        DisableThreadLibraryCalls(hinstDLL);

        install_hooks();
    }

    return TRUE;
}
