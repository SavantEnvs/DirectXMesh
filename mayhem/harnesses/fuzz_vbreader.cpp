// DirectX::VBReader / VBWriter: decode and re-encode raw vertex buffers described by a fuzzed
// D3D12 input-element declaration (DirectXMeshVBReader.cpp, DirectXMeshVBWriter.cpp, layout code in
// DirectXMeshUtil.cpp). The vertex bytes are taken unfiltered from the input.
#include "common.h"

static const char* const kSemantics[] = {"POSITION", "NORMAL", "TEXCOORD", "COLOR", "TANGENT",
                                         "BINORMAL", "BLENDINDICES", "BLENDWEIGHT", "PSIZE", "SV_Position"};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const size_t nDecl = fdp.ConsumeIntegralInRange<size_t>(1, 8);
    std::vector<D3D12_INPUT_ELEMENT_DESC> decl(nDecl);
    for (auto& d : decl) {
        d.SemanticName = kSemantics[fdp.ConsumeIntegralInRange<size_t>(0, std::size(kSemantics) - 1)];
        d.SemanticIndex = fdp.ConsumeIntegralInRange<unsigned>(0, 3);
        d.Format = static_cast<DXGI_FORMAT>(fdp.ConsumeIntegralInRange<unsigned>(0, 191));
        d.InputSlot = fdp.ConsumeIntegralInRange<unsigned>(0, 2);
        d.AlignedByteOffset = fdp.ConsumeBool() ? D3D12_APPEND_ALIGNED_ELEMENT : fdp.ConsumeIntegralInRange<unsigned>(0, 96);
        d.InputSlotClass = fdp.ConsumeBool() ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        d.InstanceDataStepRate = 0;
    }
    const size_t nVerts = fdp.ConsumeIntegralInRange<size_t>(1, 64);
    const bool x2bias = fdp.ConsumeBool();

    const D3D12_INPUT_LAYOUT_DESC layout = {decl.data(), static_cast<UINT>(nDecl)};
    IsValid(layout);
    uint32_t offsets[8] = {};
    uint32_t strides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    ComputeInputLayout(layout, offsets, strides);

    auto reader = std::make_unique<VBReader>();
    if (FAILED(reader->Initialize(layout))) return 0;

    std::vector<std::vector<uint8_t>> streams(3);
    for (unsigned slot = 0; slot < 3; ++slot) {
        if (!strides[slot]) continue;
        streams[slot] = fdp.ConsumeBytes<uint8_t>(nVerts * strides[slot]);
        streams[slot].resize(nVerts * strides[slot]);
        if (FAILED(reader->AddStream(streams[slot].data(), nVerts, slot, strides[slot]))) return 0;
    }

    std::vector<XMVECTOR> buf(nVerts);
    std::vector<XMFLOAT4> f4(nVerts);
    std::vector<float> f1(nVerts);
    auto writer = std::make_unique<VBWriter>();
    const bool canWrite = SUCCEEDED(writer->Initialize(layout));
    std::vector<std::vector<uint8_t>> out(3);
    if (canWrite)
        for (unsigned slot = 0; slot < 3; ++slot)
            if (strides[slot]) {
                out[slot].resize(nVerts * strides[slot]);
                writer->AddStream(out[slot].data(), nVerts, slot, strides[slot]);
            }
    for (const auto& d : decl) {
        if (SUCCEEDED(reader->Read(buf.data(), d.SemanticName, d.SemanticIndex, nVerts, x2bias)) && canWrite)
            writer->Write(buf.data(), d.SemanticName, d.SemanticIndex, nVerts, x2bias);
        reader->Read(f4.data(), d.SemanticName, d.SemanticIndex, nVerts, x2bias);
        reader->Read(f1.data(), d.SemanticName, d.SemanticIndex, nVerts, x2bias);
        if (canWrite) writer->Write(f4.data(), d.SemanticName, d.SemanticIndex, nVerts, x2bias);
    }
    return 0;
}
