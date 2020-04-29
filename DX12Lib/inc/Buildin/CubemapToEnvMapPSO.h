#pragma once

#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include <cstdint>

struct CubeMapToEnvCB
{
    uint32_t CubemapSize;
    uint32_t padding[3];
};

// I don't use scoped enums to avoid the explicit cast that is required to 
// treat these as root indices into the root signature.
namespace CubeMapToEnvMapRS
{
    enum
    {
        CubeMapToEnvMapCB,
        SkyboxCubaMap,
        DstMip,
        NumRootParameters
    };
}

class CubeMapToEnvMapPSO
{
public:
    CubeMapToEnvMapPSO();

    const RootSignature& GetRootSignature() const
    {
        return m_RootSignature;
    }

    Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState() const
    {
        return m_PipelineState;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
    {
        return m_DefaultUAV.GetDescriptorHandle();
    }

private:
    RootSignature m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    DescriptorAllocation m_DefaultUAV;
};