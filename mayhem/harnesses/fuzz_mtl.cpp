// Wavefront MTL material parser (Utilities/WaveFrontReader.h WaveFrontReader::LoadMTL).
#include "wfr_shim.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string p = dxm_write_temp(data, size, ".mtl");
    std::wstring wp = dxm_widen(p);
    WaveFrontReader<uint32_t> wfr;
    wfr.LoadMTL(wp.c_str());
    unlink(p.c_str());
    return 0;
}
