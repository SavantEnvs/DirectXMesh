// DirectX::ComputeMeshlets (plain and per-subset) + ComputeCullData on raw meshes, degenerate
// triangles included.
#include "rawmesh.h"

template<class index_t> static void run(FuzzedDataProvider& fdp)
{
    size_t maxVerts = fdp.ConsumeIntegralInRange<size_t>(0, 300);
    size_t maxPrims = fdp.ConsumeIntegralInRange<size_t>(0, 300);
    bool useSubsets = fdp.ConsumeBool();
    bool wcw = fdp.ConsumeBool();
    auto m = dxm_raw_mesh<index_t>(fdp);
    const uint32_t* adj = m.hasAdj ? m.adj.data() : nullptr;
    // Contract (DirectXMesh.h / wiki): meshlet generation expects a mesh that passes Validate()
    // (indices < nVerts or -1, adjacency in range).
    if (FAILED(Validate(m.indices.data(), m.nFaces, m.nVerts, adj, VALIDATE_DEFAULT))) return;

    std::vector<Meshlet> meshlets;
    std::vector<uint8_t> uniqueVB;
    std::vector<MeshletTriangle> prims;
    HRESULT hr;
    if (useSubsets) {
        auto subsets = ComputeSubsets(m.attr.data(), m.nFaces);
        std::vector<std::pair<size_t, size_t>> msub(subsets.size());
        hr = ComputeMeshlets(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, subsets.data(), subsets.size(),
                             adj, meshlets, uniqueVB, prims, msub.data(), maxVerts, maxPrims);
    } else {
        hr = ComputeMeshlets(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, adj, meshlets, uniqueVB, prims,
                             maxVerts, maxPrims);
    }
    if (FAILED(hr)) return;
    std::vector<CullData> cull(meshlets.size());
    ComputeCullData(m.pos.data(), m.nVerts, meshlets.data(), meshlets.size(),
                    reinterpret_cast<const index_t*>(uniqueVB.data()), uniqueVB.size() / sizeof(index_t),
                    prims.data(), prims.size(), cull.data(), wcw ? MESHLET_WIND_CW : MESHLET_DEFAULT);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (fdp.ConsumeBool()) run<uint32_t>(fdp); else run<uint16_t>(fdp);
    return 0;
}
