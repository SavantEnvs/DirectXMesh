// Portability shim so Utilities/WaveFrontReader.h (header-only) compiles against libstdc++ without
// editing it. Upstream only builds the library on Linux; the header assumes MSVC's STL, where
// std::filesystem::path::c_str() is const wchar_t* and file streams take wchar_t* names. The shim
// keeps those MSVC semantics by renaming three std names to thin wrappers for the header only.
#pragma once
#include "common.h"

namespace dxm_shim {
struct path {
    std::filesystem::path p;
    mutable std::wstring w;
    path(const wchar_t* s) : p(s) {}
    path(std::filesystem::path q) : p(std::move(q)) {}
    path filename() const { return path(p.filename()); }
    path extension() const { return path(p.extension()); }
    path& replace_filename(const path& o) { p.replace_filename(o.p); return *this; }
    path& replace_extension(const path& o) { p.replace_extension(o.p); return *this; }
    const wchar_t* c_str() const { w = p.wstring(); return w.c_str(); }
};
struct wifstream : std::basic_ifstream<wchar_t> {
    explicit wifstream(const wchar_t* s) : std::basic_ifstream<wchar_t>(std::filesystem::path(s)) {}
};
struct ifstream : std::basic_ifstream<char> {
    ifstream(const wchar_t* s, std::ios_base::openmode m) : std::basic_ifstream<char>(std::filesystem::path(s), m) {}
    static constexpr std::ios_base::openmode in = std::ios_base::in;
    static constexpr std::ios_base::openmode binary = std::ios_base::binary;
};
} // namespace dxm_shim

namespace std {
namespace dxm_fs { using path = ::dxm_shim::path; }
using dxm_wifstream = ::dxm_shim::wifstream;
using dxm_ifstream = ::dxm_shim::ifstream;
} // namespace std

// MSVC CRT secure-string functions used by the header.
// Like the MSVC CRT, a destination that is too small is a fatal invalid-parameter error (abort),
// not a silent truncation.
inline int wcscpy_s(wchar_t* d, size_t n, const wchar_t* s)
{
    if (!d || !s || n == 0 || wcslen(s) >= n) abort();
    wcscpy(d, s);
    return 0;
}
template<size_t N> inline int wcscpy_s(wchar_t (&d)[N], const wchar_t* s) { return wcscpy_s(d, N, s); }
#ifndef OutputDebugStringW
inline void OutputDebugStringW(const wchar_t*) {}
#endif

#define filesystem dxm_fs
#define wifstream dxm_wifstream
#define ifstream dxm_ifstream
#include "WaveFrontReader.h"
#undef filesystem
#undef wifstream
#undef ifstream

using DX::WaveFrontReader;

static inline std::wstring dxm_widen(const std::string& s)
{
    return std::filesystem::path(s).wstring();
}
