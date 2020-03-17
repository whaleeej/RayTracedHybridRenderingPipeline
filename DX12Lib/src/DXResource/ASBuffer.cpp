#include "DX12LibPCH.h"

#include <ASBuffer.h>

#include <cassert>

ASBuffer::ASBuffer(const std::wstring& name)
    : Buffer(name)
{}

ASBuffer::~ASBuffer()
{}

void ASBuffer::CreateViews(size_t numElements, size_t elementSize)
{

}

D3D12_CPU_DESCRIPTOR_HANDLE ASBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    throw std::exception("IndexBuffer::GetShaderResourceView should not be called.");
}

D3D12_CPU_DESCRIPTOR_HANDLE ASBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    throw std::exception("IndexBuffer::GetUnorderedAccessView should not be called.");
}
