#include <Windows.h>
#include <sstream>
#include <iostream>
#include <tuple>
#include <optional>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>

// Frames per second
// Noticible flickering above 25 FPS
double const fps = 25;
// Number of columns
double const width = 80;
// Number of rows
double const height = 40;
// Field of view in degrees
auto const fov = 40;
// ASCII characters representing cells from dark to light
// From http://paulbourke.net/dataformats/asciiart/
char const shades[] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\" ^ `'. ";

struct Vector3 {
    double x, y, z;

    Vector3(double x = 0, double y = 0, double z = 0) : x{ x }, y{ y }, z{ z } {}

    double length() const {
        return sqrt(x * x + y * y + z * z);
    }

    void normalize() {
        auto len = length();
        x /= len;
        y /= len;
        z /= len;
    }

    double dot(Vector3 other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 cross(Vector3 other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    Vector3 operator+(Vector3 other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    Vector3 operator-(Vector3 other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    Vector3 operator*(double other) const {
        return { x * other, y * other, z * other };
    }
};

struct Ray {
    Vector3 position;
    Vector3 direction;

    Ray(Vector3 position, Vector3 direction) : position{ position }, direction{ direction } {
        this->direction.normalize();
    }

    static Ray from_segment(Vector3 from, Vector3 to) {
        return { from, to - from };
    }
};

struct Sphere {
    Vector3 position;
    double radius;

    std::optional<Ray> intersect(Ray ray) const {
        // Create a cross-section of the sphere co-planar to the ray
        auto v = position - ray.position;
        auto b = v.dot(ray.direction);
        auto d2 = b * b - v.dot(v) + radius * radius;

        // No intersection
        if (d2 < 0) return {};

        // Two possible solutions to the quadratic: pick closest
        auto d = sqrt(d2);
        auto t0 = b - d, t1 = b + d;
        auto t = (t0 > 0 ? t0 : t1);

        auto intersection_pos = ray.position + ray.direction * t;
        return Ray{ intersection_pos, intersection_pos - position };
    }
};

auto const pi = atan(1) * 4;
auto const fov_factor = tan((fov * (pi / 180)) / 2);
auto const aspect_ratio = (width / height) / 2;



// Gets the start of the import table of a module
PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE module) {
    auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
    auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(module) + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        return nullptr;
    }

    auto import_dir = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (import_dir.VirtualAddress == 0) {
        return nullptr;
    }

    return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<uintptr_t>(module) + import_dir.VirtualAddress);
}

template<typename FuncTy>
FuncTy get_imported_function(char const* module_name, char const* func_name) {
    return reinterpret_cast<FuncTy>(GetProcAddress(GetModuleHandleA(module_name), func_name));
}


template<typename FuncTy>
bool hook_import(HMODULE module, char const* module_name, char const* func_name, FuncTy& original, FuncTy hook) {
    // Store a pointer to the actual function; this function can be called
    // multiple times in the same address space so we only want to do this
    // the first time
    if (original == nullptr) {
        original = get_imported_function<FuncTy>(module_name, func_name);
        if (original == nullptr) {
            return false;
        }
    }

    auto import_descr = get_import_table(module);
    if (import_descr == nullptr) {
        return false;
    }

    // Walk the import descriptor table
    auto base_addr = reinterpret_cast<uintptr_t>(module);
    for (; import_descr->Name != 0; import_descr++) {
        // Skip other modules
        auto name = reinterpret_cast<LPCSTR>(base_addr + import_descr->Name);

        if (_stricmp(name, module_name)) {
            continue;
        }

        auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base_addr + import_descr->FirstThunk);
        auto original_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base_addr + import_descr->OriginalFirstThunk);
        for (; thunk->u1.Function != 0; thunk++, original_thunk++) {
            //auto import_by_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base_addr + original_thunk->u1.AddressOfData);
            //auto curr_func_name = reinterpret_cast<char *>(import_by_name->Name);

            // Skip other functions
            if (thunk->u1.Function != reinterpret_cast<uintptr_t>(original)) {
                continue;
            }

            // Patch the function pointer
            DWORD old_protection;
            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &old_protection);
            thunk->u1.Function = reinterpret_cast<uintptr_t>(hook);
            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), old_protection, &old_protection);
        }
    }

    return true;
}

HWND* hwndEdit = nullptr;

bool keydown[0x30] = { false };

DWORD WINAPI work(LPVOID) {
    auto time = 0.f;
    auto const time_step = 1 / fps;

    // Buffer frames into a string to simplify text processing. Reserving a
    // fixed capacity and then clearing the contents every frame prevents
    // wasteful allocations
    std::string buffer;
    buffer.reserve(width * height + (height - 1));

    Vector3 const eye_pos{ 0, 0, 15 };

    // Calculate eye angles w.r.t. the origin
    auto eye_dir = Vector3{ 0, 0, 0 } -eye_pos;
    eye_dir.normalize();
    auto eye_right = eye_dir.cross(Vector3{ 0, -1, 0 });
    eye_right.normalize();
    auto eye_up = eye_right.cross(eye_dir);
    eye_up.normalize();

    Vector3 light_pos{ 10, 10, 5 };

    Sphere ball{ Vector3{ 0, 0, 0 }, 1 };

    for (;;) {
        // Pan ball with arrow keys
        int dx = 0, dy = 0;
        if (keydown[VK_LEFT]) dx = -1;
        else if (keydown[VK_RIGHT]) dx = 1;
        if (keydown[VK_UP]) dy = -1;
        else if (keydown[VK_DOWN]) dy = 1;

        ball.position.x -= dx * 0.2;
        ball.position.y -= dy * 0.2;

        // Zoom ball with space
        if (keydown[VK_SPACE]) ball.position.z += 0.5;
        else if (ball.position.z > 0) ball.position.z -= 0.5;

        // Revolve light around the ball
        light_pos.x = cos(time * pi * 2) * 10;
        light_pos.z = sin(time * pi * 2) * 10;

        // Ray trace
        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                auto const look_pos = eye_pos + eye_dir +
                    eye_right * fov_factor * aspect_ratio * (x / width - 0.5) +
                    eye_up * fov_factor * (y / height - 0.5);

                auto const test_ray = Ray::from_segment(eye_pos, look_pos);
                if (auto const intersection = ball.intersect(test_ray)) {
                    auto light_dir = light_pos - intersection->position;
                    light_dir.normalize();

                    double diffuse = intersection->direction.dot(light_dir);
                    if (diffuse <= 0) {
                        diffuse = 0;
                    }

                    int const shade_index = (diffuse * diffuse * diffuse) * (sizeof(shades) - 1);
                    buffer += shades[shade_index];
                }
                else {
                    buffer += ' ';
                }
            }

            buffer += '\n';
        }

        // Write the buffer to Notepad
        SendMessageA(*hwndEdit, WM_SETTEXT,
            buffer.length(), reinterpret_cast<LPARAM>(buffer.c_str()));

        buffer.clear();

        // Sleep until the next time step, neglecting the time elapsed in this
        // loop itself
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
