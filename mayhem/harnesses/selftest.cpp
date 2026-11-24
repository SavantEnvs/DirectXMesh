// Known-answer functional tests for DirectXMesh, run by mayhem/test.sh (upstream's own test suite
// lives in a separate repository, walbourn/directxmeshtest, and is Windows-only).
// Prints one "PASS <name>" / "FAIL <name>" line per test.
#include "wfr_shim.h"
#include <cmath>

static int g_fail = 0;
static void check(const char* name, bool ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}

static const XMFLOAT3 kCubePos[8] = {{0,0,0},{0,0,1},{0,1,0},{0,1,1},{1,0,0},{1,0,1},{1,1,0},{1,1,1}};
static const uint16_t kCubeIB[36] = {0,1,3, 0,3,2, 4,6,7, 4,7,5, 0,4,5, 0,5,1, 2,3,7, 2,7,6, 0,2,6, 0,6,4, 1,5,7, 1,7,3};

int main(int argc, char** argv)
{
    // 1. adjacency of a closed cube: every edge has a neighbour, point reps are identity
    std::vector<uint32_t> adj(36), preps(8);
    HRESULT hr = GenerateAdjacencyAndPointReps(kCubeIB, 12, kCubePos, 8, 0.f, preps.data(), adj.data());
    bool closed = SUCCEEDED(hr);
    for (auto a : adj) closed = closed && a < 12;
    for (uint32_t i = 0; i < 8; ++i) closed = closed && preps[i] == i;
    check("adjacency_closed_cube", closed);
    // face 0 (0,1,3) shares edge 0-1 with face 5 (0,5,1)
    check("adjacency_face0_edge0", adj[0] == 5);

    // 2. Validate accepts the cube and rejects an out-of-range index and a degenerate face
    check("validate_cube_ok", Validate(kCubeIB, 12, 8, adj.data(), VALIDATE_DEFAULT) == S_OK);
    uint16_t bad[3] = {0, 1, 9};
    check("validate_out_of_range", Validate(bad, 1, 8, nullptr, VALIDATE_DEFAULT) == E_FAIL);
    uint16_t degen[3] = {0, 0, 1};
    std::wstring msgs;
    check("validate_degenerate", FAILED(Validate(degen, 1, 8, nullptr, VALIDATE_DEGENERATE, &msgs)) && !msgs.empty());

    // 3. normals of a single triangle in the XY plane
    XMFLOAT3 tri[3] = {{0,0,0},{1,0,0},{0,1,0}};
    uint16_t tib[3] = {0, 1, 2};
    XMFLOAT3 n[3] = {};
    hr = ComputeNormals(tib, 1, tri, 3, CNORM_DEFAULT, n);
    check("normals_ccw", SUCCEEDED(hr) && n[0].x == 0.f && n[0].y == 0.f && n[0].z == 1.f && n[2].z == 1.f);
    hr = ComputeNormals(tib, 1, tri, 3, CNORM_WIND_CW, n);
    check("normals_cw", SUCCEEDED(hr) && n[1].z == -1.f);

    // 4. meshlets of the cube: one meshlet holding all 12 triangles and 8 vertices
    std::vector<Meshlet> ml;
    std::vector<uint8_t> uvb;
    std::vector<MeshletTriangle> prims;
    hr = ComputeMeshlets(kCubeIB, 12, kCubePos, 8, adj.data(), ml, uvb, prims);
    check("meshlets_cube", SUCCEEDED(hr) && ml.size() == 1 && ml[0].PrimCount == 12 && ml[0].VertCount == 8 && prims.size() == 12);

    // 5. vertex cache optimisation keeps a permutation
    std::vector<uint32_t> remap(8);
    hr = OptimizeVertices(kCubeIB, 12, 8, remap.data());
    std::vector<uint32_t> sorted = remap;
    std::sort(sorted.begin(), sorted.end());
    bool perm = SUCCEEDED(hr);
    for (uint32_t i = 0; i < 8; ++i) perm = perm && sorted[i] == i;
    check("optimize_vertices_permutation", perm && remap[0] == 0);

    // 6. VBReader decodes R8G8B8A8_UNORM colours
    D3D12_INPUT_ELEMENT_DESC decl[1] = {{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
    uint8_t vb[8] = {255, 0, 51, 255, 0, 255, 0, 0};
    VBReader rd;
    XMFLOAT4 c[2] = {};
    const D3D12_INPUT_LAYOUT_DESC layout = {decl, 1};
    hr = rd.Initialize(layout);
    if (SUCCEEDED(hr)) hr = rd.AddStream(vb, 2, 0, 4);
    if (SUCCEEDED(hr)) hr = rd.Read(c, "COLOR", 0, 2);
    check("vbreader_unorm", SUCCEEDED(hr) && c[0].x == 1.f && c[0].y == 0.f && std::fabs(c[0].z - 0.2f) < 1e-6f && c[1].y == 1.f && c[1].w == 0.f);

    // 7. OBJ loader on the cube seed
    if (argc > 1) {
        WaveFrontReader<uint32_t> wfr;
        std::wstring p = dxm_widen(argv[1]);
        hr = wfr.Load(p.c_str(), true, false);
        check("obj_load_cube", SUCCEEDED(hr) && wfr.indices.size() == 36 && wfr.vertices.size() == 24 && wfr.hasNormals);
    } else {
        check("obj_load_cube", false);
    }
    // 8. VBO loader on the generated cube seed
    if (argc > 2) {
        WaveFrontReader<uint16_t> wfr;
        std::wstring p = dxm_widen(argv[2]);
        hr = wfr.LoadVBO(p.c_str());
        check("vbo_load_cube", SUCCEEDED(hr) && wfr.vertices.size() == 8 && wfr.indices.size() == 36 && wfr.indices[2] == 3 &&
              wfr.vertices[7].position.x == 1.f);
    } else {
        check("vbo_load_cube", false);
    }
    return g_fail ? 1 : 0;
}
