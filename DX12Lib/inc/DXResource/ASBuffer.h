#pragma once

#include "Buffer.h"

class ASBuffer : public Buffer
{
public:
	ASBuffer( const std::wstring& name = L"");
    virtual ~ASBuffer();

    // Inherited from Buffer
    virtual void CreateViews(size_t numElements, size_t elementSize) override;

    /**
    * Get the SRV for a resource.
    */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

    /**
    * Get the UAV for a (sub)resource.
    */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

protected:

private:

};