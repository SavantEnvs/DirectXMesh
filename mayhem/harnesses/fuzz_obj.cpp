// Wavefront OBJ parser (Utilities/WaveFrontReader.h WaveFrontReader::Load) followed by the
// DirectXMesh processing pipeline on the parsed mesh, as meshconvert does with an .obj input.
#include "wfr_shim.h"
#include "pipeline.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string p = dxm_write_temp(data, size, ".obj");
    std::wstring wp = dxm_widen(p);
    WaveFrontReader<uint32_t> wfr;
    // loadmtl=false: an mtllib line would otherwise open a second, input-named file from disk;
    // the MTL parser has its own target (fuzz_mtl).
    HRESULT hr = wfr.Load(wp.c_str(), (size & 1) != 0, false);
    unlink(p.c_str());
    if (FAILED(hr)) return 0;

    std::vector<XMFLOAT3> pos;
    std::vector<XMFLOAT2> uv;
    for (auto& v : wfr.vertices) { pos.push_back(v.position); uv.push_back(v.textureCoordinate); }
    dxm_run_pipeline<uint32_t>(wfr.indices, pos, uv, wfr.attributes);
    return 0;
}
