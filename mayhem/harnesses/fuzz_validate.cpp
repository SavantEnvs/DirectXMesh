// DirectX::Validate + DirectX::Clean on raw index/adjacency/attribute buffers (16- and 32-bit
// indices). These are the functions the library documents as the gatekeepers for untrusted meshes,
// so they get the buffers unfiltered (out-of-range and -1 sentinel indices included).
#include "rawmesh.h"

template<class index_t> static void run(FuzzedDataProvider& fdp)
{
    auto flags = static_cast<VALIDATE_FLAGS>(fdp.ConsumeIntegral<uint8_t>() & 0x1f);
    bool breakBowties = fdp.ConsumeBool();
    auto m = dxm_raw_mesh<index_t>(fdp);
    const uint32_t* adj = m.hasAdj ? m.adj.data() : nullptr;
    std::wstring msgs;
    Validate(m.indices.data(), m.nFaces, m.nVerts, adj, flags, &msgs);
    Validate(m.indices.data(), m.nFaces, m.nVerts, adj, flags, nullptr);
    std::vector<uint32_t> dups;
    Clean(m.indices.data(), m.nFaces, m.nVerts, m.hasAdj ? m.adj.data() : nullptr, m.attr.data(), dups, breakBowties);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (fdp.ConsumeBool()) run<uint32_t>(fdp); else run<uint16_t>(fdp);
    return 0;
}
