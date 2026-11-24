// VBO binary mesh reader (Utilities/WaveFrontReader.h WaveFrontReader::LoadVBO) for both index
// widths, then the DirectXMesh pipeline on the loaded mesh.
#include "wfr_shim.h"
#include "pipeline.h"

template<class index_t> static void run(const std::wstring& wp)
{
    WaveFrontReader<index_t> wfr;
    if (FAILED(wfr.LoadVBO(wp.c_str()))) return;
    std::vector<XMFLOAT3> pos;
    std::vector<XMFLOAT2> uv;
    for (auto& v : wfr.vertices) { pos.push_back(v.position); uv.push_back(v.textureCoordinate); }
    dxm_run_pipeline<index_t>(wfr.indices, pos, uv, {});
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string p = dxm_write_temp(data, size, ".vbo");
    std::wstring wp = dxm_widen(p);
    run<uint16_t>(wp);
    run<uint32_t>(wp);
    unlink(p.c_str());
    return 0;
}
