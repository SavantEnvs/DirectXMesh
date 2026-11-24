// Runs the DirectXMesh processing pipeline (as meshconvert does) over a parsed mesh.
#pragma once
#include "common.h"

template<class index_t>
static void dxm_run_pipeline(std::vector<index_t> indices, std::vector<XMFLOAT3> pos,
                             std::vector<XMFLOAT2> uv, std::vector<uint32_t> attr)
{
    if (indices.empty() || indices.size() % 3 || pos.empty()) return;
    const size_t nFaces = indices.size() / 3;
    size_t nVerts = pos.size();
    if (uv.size() != nVerts) uv.assign(nVerts, XMFLOAT2(0, 0));
    if (attr.size() != nFaces) attr.assign(nFaces, 0);

    std::vector<uint32_t> adj(nFaces * 3), preps(nVerts);
    if (FAILED(GenerateAdjacencyAndPointReps(indices.data(), nFaces, pos.data(), nVerts, 0.f, preps.data(), adj.data())))
        return;
    std::wstring msgs;
    Validate(indices.data(), nFaces, nVerts, adj.data(),
             static_cast<VALIDATE_FLAGS>(VALIDATE_BACKFACING | VALIDATE_BOWTIES | VALIDATE_DEGENERATE | VALIDATE_UNUSED | VALIDATE_ASYMMETRIC_ADJ), &msgs);
    // DirectXMesh's documented contract: the remaining operations expect a mesh that passes Validate().
    if (FAILED(Validate(indices.data(), nFaces, nVerts, adj.data(), VALIDATE_DEFAULT)))
        return;
    std::vector<uint32_t> dups;
    if (FAILED(Clean(indices.data(), nFaces, nVerts, adj.data(), attr.data(), dups, true)))
        return;
    for (uint32_t d : dups) { pos.push_back(pos[d]); uv.push_back(uv[d]); }
    nVerts = pos.size();
    if (nVerts >= UINT32_MAX) return;

    std::vector<XMFLOAT3> normals(nVerts), tangents(nVerts), bitangents(nVerts);
    ComputeNormals(indices.data(), nFaces, pos.data(), nVerts, CNORM_DEFAULT, normals.data());
    ComputeTangentFrame(indices.data(), nFaces, pos.data(), normals.data(), uv.data(), nVerts, tangents.data(), bitangents.data());

    std::vector<uint32_t> faceRemap(nFaces);
    if (SUCCEEDED(AttributeSort(nFaces, attr.data(), faceRemap.data())))
        ReorderIBAndAdjacency(indices.data(), nFaces, adj.data(), faceRemap.data());
    if (SUCCEEDED(OptimizeFacesEx(indices.data(), nFaces, nVerts, adj.data(), attr.data(), faceRemap.data())))
        ReorderIBAndAdjacency(indices.data(), nFaces, adj.data(), faceRemap.data());
    std::vector<uint32_t> vertRemap(nVerts);
    if (SUCCEEDED(OptimizeVertices(indices.data(), nFaces, nVerts, vertRemap.data()))) {
        FinalizeIB(indices.data(), nFaces, vertRemap.data(), nVerts);
        FinalizeVB(pos.data(), sizeof(XMFLOAT3), nVerts, vertRemap.data());
    }

    std::vector<Meshlet> meshlets;
    std::vector<uint8_t> uniqueVB;
    std::vector<MeshletTriangle> prims;
    if (SUCCEEDED(ComputeMeshlets(indices.data(), nFaces, pos.data(), nVerts, nullptr, meshlets, uniqueVB, prims))) {
        std::vector<CullData> cull(meshlets.size());
        const size_t nVI = uniqueVB.size() / sizeof(index_t);
        ComputeCullData(pos.data(), nVerts, meshlets.data(), meshlets.size(),
                        reinterpret_cast<const index_t*>(uniqueVB.data()), nVI,
                        prims.data(), prims.size(), cull.data());
    }
}
