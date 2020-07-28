#pragma once
#include <vector>
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Heap : public WrappedD3D12DeviceChild<ID3D12Heap> {
public:
	WrappedD3D12Heap(ID3D12Heap* pReal, WrappedD3D12Device* pWrappedDevice);
	~WrappedD3D12Heap();

public: //override
	virtual D3D12_HEAP_DESC STDMETHODCALLTYPE GetDesc(void);

public: //framework:
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);
};

class WrappedD3D12Resource;

class WrappedD3D12DescriptorHeap : public WrappedD3D12DeviceChild<ID3D12DescriptorHeap> {
public:
	enum ViewDescType {
		ViewDesc_SRV=0,
		ViewDesc_CBV,
		ViewDesc_UAV,
		ViewDesc_RTV,
		ViewDesc_DSV,
		ViewDesc_SamplerV,
		ViewDesc_Unknown
	};
	struct DescriptorHeapSlotDesc{
		WrappedD3D12Resource* pWrappedD3D12Resource = NULL;
		ViewDescType viewDescType = ViewDesc_Unknown;
		union ConcreteViewDesc{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv;
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
			D3D12_SAMPLER_DESC sampler;
		}concreteViewDesc;
	};

public:
	WrappedD3D12DescriptorHeap(ID3D12DescriptorHeap* pReal, WrappedD3D12Device* pWrappedDevice);
	~WrappedD3D12DescriptorHeap();

public: //override
	virtual D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc(void);

	virtual D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetCPUDescriptorHandleForHeapStart(void);

	virtual D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetGPUDescriptorHandleForHeapStart(void);

public: // function
	bool handleInDescriptorHeapRange(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		auto desc = GetReal()->GetDesc();
		UINT perSize = m_pRealDevice->GetDescriptorHandleIncrementSize(desc.Type);
		return handle.ptr >= GetReal()->GetCPUDescriptorHandleForHeapStart().ptr && handle.ptr < (GetReal()->GetCPUDescriptorHandleForHeapStart().ptr + desc.NumDescriptors *perSize);
	}

	void cacheDescriptorCreateParam(DescriptorHeapSlotDesc slotDesc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		auto desc = GetReal()->GetDesc();
		auto diff = handle.ptr - GetReal()->GetCPUDescriptorHandleForHeapStart().ptr;
		SIZE_T num = (SIZE_T)(diff / m_pRealDevice->GetDescriptorHandleIncrementSize(desc.Type));
		Assert(num >= 0 && num < desc.NumDescriptors);
		m_slotDesc[num] = slotDesc;
	}

public: //framework:
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);

protected:
	std::vector<DescriptorHeapSlotDesc> m_slotDesc;
};

RDCBOOST_NAMESPACE_END