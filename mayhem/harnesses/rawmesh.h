// Decodes a raw mesh (the buffers DirectXMesh's public API takes) from fuzz input.
#pragma once
#include "common.h"

template<class index_t> struct RawMesh {
    size_t nVerts = 0, nFaces = 0;
    std::vector<index_t> indices;
    std::vector<XMFLOAT3> pos;
    std::vector<uint32_t> adj, attr;
    bool hasAdj = false;
};

// Layout: u16 nVerts, u16 nFaces, u8 flags, then raw indices, raw adjacency (if flag bit 0),
// attributes (if bit 1), then positions; short reads are zero-filled by FuzzedDataProvider.
template<class index_t>
static RawMesh<index_t> dxm_raw_mesh(FuzzedDataProvider& fdp)
{
    RawMesh<index_t> m;
    m.nVerts = fdp.ConsumeIntegralInRange<size_t>(1, 2048);
    m.nFaces = fdp.ConsumeIntegralInRange<size_t>(1, 2048);
    uint8_t f = fdp.ConsumeIntegral<uint8_t>();
    m.indices.resize(m.nFaces * 3);
    for (auto& i : m.indices) {
        // small values index real vertices; the high bit asks for a raw (possibly out-of-range or
        // sentinel -1) value, the cases Validate() bounds-checks.
        uint32_t r = fdp.ConsumeIntegral<uint16_t>();
        i = (r & 0x8000) ? static_cast<index_t>(fdp.ConsumeIntegral<uint32_t>()) : static_cast<index_t>(r % (m.nVerts + 1));
    }
    m.hasAdj = f & 1;
    if (m.hasAdj) {
        m.adj.resize(m.nFaces * 3);
        for (auto& a : m.adj) {
            uint32_t r = fdp.ConsumeIntegral<uint16_t>();
            a = (r & 0x8000) ? fdp.ConsumeIntegral<uint32_t>() : (r == 0x7fff ? UINT32_MAX : r % (m.nFaces + 1));
        }
    }
    m.attr.resize(m.nFaces);
    if (f & 2) for (auto& a : m.attr) a = fdp.ConsumeIntegralInRange<uint32_t>(0, 7);
    m.pos.resize(m.nVerts);
    for (auto& p : m.pos) {
        p.x = static_cast<float>(fdp.ConsumeIntegral<int8_t>());
        p.y = static_cast<float>(fdp.ConsumeIntegral<int8_t>());
        p.z = static_cast<float>(fdp.ConsumeIntegral<int8_t>());
    }
    return m;
}
