#pragma once

#include <Windows.h>

// Gets the start of the import table of a module
PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE module);

template<typename FuncTy>
FuncTy get_imported_function(char const* module_name, char const* func_name) {
    auto module = GetModuleHandleA(module_name);
    if (module == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<FuncTy>(GetProcAddress(module, func_name));
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