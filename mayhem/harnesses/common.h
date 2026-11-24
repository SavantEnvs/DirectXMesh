// Shared helpers for the DirectXMesh harnesses (savantenvs integration).
#pragma once
// sal.h macros (e.g. __valid) clash with libstdc++ internals, so every std header used anywhere is
// included BEFORE DirectXMesh.h / WaveFrontReader.h pull sal.h in.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unistd.h>
#include <fuzzer/FuzzedDataProvider.h>

// On Linux DirectXMesh exposes the D3D12 input-layout overloads (VBReader/VBWriter, IsValid,
// ComputeInputLayout) when d3d12.h is included first, as the library itself does.
#include <wsl/winadapter.h>
#include <directx/d3d12.h>
#include "DirectXMesh.h"

using namespace DirectX;

// Writes the input to a fresh file in $TMPDIR (fallback /tmp) with the given suffix and returns its
// path; the caller unlinks it. The WaveFrontReader API reads files only (no memory API).
static inline std::string dxm_write_temp(const uint8_t* data, size_t size, const char* suffix)
{
    const char* dir = getenv("TMPDIR");
    if (!dir || !*dir) dir = "/tmp";
    std::string tmpl = std::string(dir) + "/dxmfuzzXXXXXX" + suffix;
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), static_cast<int>(strlen(suffix)));
    if (fd < 0) abort();
    size_t off = 0;
    while (off < size) {
        ssize_t w = write(fd, data + off, size - off);
        if (w <= 0) abort();
        off += static_cast<size_t>(w);
    }
    close(fd);
    return std::string(buf.data());
}
