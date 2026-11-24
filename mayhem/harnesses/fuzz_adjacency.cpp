// Adjacency / point reps / GS adjacency / normals / tangent frames on raw meshes.
#include "rawmesh.h"

template<class index_t> static void run(FuzzedDataProvider& fdp)
{
    float eps = static_cast<float>(fdp.ConsumeIntegralInRange<int>(-2, 8)) * 0.25f;
    auto cflags = static_cast<CNORM_FLAGS>(fdp.ConsumeIntegral<uint8_t>() & 0x7);
    auto m = dxm_raw_mesh<index_t>(fdp);
    std::vector<XMFLOAT2> uv(m.nVerts);
    for (auto& t : uv) { t.x = fdp.ConsumeIntegral<int8_t>() / 16.f; t.y = fdp.ConsumeIntegral<int8_t>() / 16.f; }
    // Contract: these functions expect Validate()-clean index buffers.
    if (FAILED(Validate(m.indices.data(), m.nFaces, m.nVerts, nullptr, VALIDATE_DEFAULT))) return;

    std::vector<uint32_t> preps(m.nVerts), adj(m.nFaces * 3), adj2(m.nFaces * 3);
    if (FAILED(GenerateAdjacencyAndPointReps(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, eps, preps.data(), adj.data())))
        return;
    ConvertPointRepsToAdjacency(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, preps.data(), adj2.data());
    ConvertPointRepsToAdjacency(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, nullptr, adj2.data());
    std::vector<index_t> gs(m.nFaces * 6);
    GenerateGSAdjacency(m.indices.data(), m.nFaces, preps.data(), adj.data(), m.nVerts, gs.data());

    std::vector<XMFLOAT3> n(m.nVerts), t(m.nVerts), b(m.nVerts);
    std::vector<XMFLOAT4> t4(m.nVerts);
    ComputeNormals(m.indices.data(), m.nFaces, m.pos.data(), m.nVerts, cflags, n.data());
    ComputeTangentFrame(m.indices.data(), m.nFaces, m.pos.data(), n.data(), uv.data(), m.nVerts, t.data(), b.data());
    ComputeTangentFrame(m.indices.data(), m.nFaces, m.pos.data(), n.data(), uv.data(), m.nVerts, t4.data());
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (fdp.ConsumeBool()) run<uint32_t>(fdp); else run<uint16_t>(fdp);
    return 0;
}
