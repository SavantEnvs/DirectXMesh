// Face/vertex cache optimisers, remap/finalize/compact, weld, concat, attribute sort and subsets
// on raw meshes (DirectXMeshOptimize*.cpp, DirectXMeshRemap.cpp, DirectXMeshWeldVertices.cpp,
// DirectXMeshConcat.cpp, DirectXMeshUtil.cpp).
#include "rawmesh.h"

template<class index_t> static void run(FuzzedDataProvider& fdp)
{
    uint32_t vcache = fdp.ConsumeIntegralInRange<uint32_t>(0, 64);
    uint32_t restart = fdp.ConsumeIntegralInRange<uint32_t>(0, 64);
    uint32_t lru = fdp.ConsumeIntegralInRange<uint32_t>(0, 80);
    size_t cacheSize = fdp.ConsumeIntegralInRange<size_t>(0, 64);
    auto m = dxm_raw_mesh<index_t>(fdp);
    // Contract: the optimisers expect a mesh that passes Validate() (indices < nVerts or -1).
    if (FAILED(Validate(m.indices.data(), m.nFaces, m.nVerts, nullptr, VALIDATE_DEFAULT))) return;

    std::vector<uint32_t> preps(m.nVerts), adj(m.nFaces * 3);
    if (FAILED(GenerateAdjacencyAndPointReps(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, 0.f, preps.data(), adj.data())))
        return;

    float acmr = 0, atvr = 0;
    ComputeVertexCacheMissRate(m.indices.data(), m.nFaces, m.nVerts, cacheSize, acmr, atvr);

    std::vector<uint32_t> faceRemap(m.nFaces), attr = m.attr;
    if (SUCCEEDED(AttributeSort(m.nFaces, attr.data(), faceRemap.data()))) {
        std::vector<index_t> ib2(m.nFaces * 3);
        std::vector<uint32_t> adj2(m.nFaces * 3);
        ReorderIB(m.indices.data(), m.nFaces, faceRemap.data(), ib2.data());
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data(), ib2.data(), adj2.data());
        // keep indices/adjacency in the attribute-sorted face order from here on
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data());
    }
    auto subsets = ComputeSubsets(attr.data(), m.nFaces);
    (void)subsets;

    if (SUCCEEDED(OptimizeFaces(m.indices.data(), m.nFaces, m.nVerts, adj.data(), faceRemap.data(), vcache, restart)))
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data());
    if (SUCCEEDED(OptimizeFacesEx(m.indices.data(), m.nFaces, m.nVerts, adj.data(), attr.data(), faceRemap.data(), vcache, restart)))
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data());
    if (SUCCEEDED(OptimizeFacesLRU(m.indices.data(), m.nFaces, m.nVerts, faceRemap.data(), lru)))
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data());
    if (SUCCEEDED(OptimizeFacesLRUEx(m.indices.data(), m.nFaces, m.nVerts, attr.data(), faceRemap.data(), lru)))
        ReorderIBAndAdjacency(m.indices.data(), m.nFaces, adj.data(), faceRemap.data());

    std::vector<uint32_t> vertRemap(m.nVerts);
    size_t trailing = 0;
    if (SUCCEEDED(OptimizeVertices(m.indices.data(), m.nFaces, m.nVerts, vertRemap.data(), &trailing))) {
        std::vector<index_t> ib2(m.nFaces * 3);
        FinalizeIB(m.indices.data(), m.nFaces, vertRemap.data(), m.nVerts, ib2.data());
        std::vector<XMFLOAT3> vb2(m.nVerts);
        std::vector<uint32_t> preps2(m.nVerts);
        FinalizeVB(m.pos.data(), sizeof(XMFLOAT3), m.nVerts, nullptr, 0, vertRemap.data(), vb2.data());
        FinalizeVBAndPointReps(m.pos.data(), sizeof(XMFLOAT3), m.nVerts, preps.data(), nullptr, 0,
                               vertRemap.data(), vb2.data(), preps2.data());
        if (trailing < m.nVerts) {
            std::vector<XMFLOAT3> vb3(m.nVerts - trailing);
            CompactVB(vb2.data(), sizeof(XMFLOAT3), m.nVerts, trailing, nullptr, vb3.data());
        }
    }

    std::vector<uint32_t> weldRemap(m.nVerts);
    std::vector<index_t> ibw = m.indices;
    const auto& pos = m.pos;
    WeldVertices(ibw.data(), m.nFaces, m.nVerts, preps.data(), weldRemap.data(),
                 [&](uint32_t a, uint32_t b) { return pos[a].x == pos[b].x && pos[a].y == pos[b].y; });

    std::vector<uint32_t> fdm(m.nFaces), vdm(m.nVerts);
    size_t tf = fdp.ConsumeIntegral<uint16_t>(), tv = fdp.ConsumeIntegral<uint16_t>();
    ConcatenateMesh(m.nFaces, m.nVerts, fdm.data(), vdm.data(), tf, tv);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (fdp.ConsumeBool()) run<uint32_t>(fdp); else run<uint16_t>(fdp);
    return 0;
}
