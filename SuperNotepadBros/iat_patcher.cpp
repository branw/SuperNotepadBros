#include "iat_patcher.hpp"

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
