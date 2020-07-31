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
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);
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
	struct DescriptorHeapSlotDesc{ // TODO 把WrappedD3D12Resource的生命拥有起来，改成ComPtr
		WrappedD3D12Resource* pWrappedD3D12Resource = NULL;
		WrappedD3D12Resource* pWrappedD3D12CounterResource = NULL;
		ViewDescType viewDescType = ViewDesc_Unknown;
		bool isViewDescNull = false;
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
		auto& desc = GetReal()->GetDesc();
		UINT perSize = m_pWrappedDevice->GetDescriptorHandleIncrementSize(desc.Type);
		return handle.ptr >= GetReal()->GetCPUDescriptorHandleForHeapStart().ptr && handle.ptr < (GetReal()->GetCPUDescriptorHandleForHeapStart().ptr + desc.NumDescriptors *perSize);
	}

	DescriptorHeapSlotDesc& getDescriptorCreateParamCache(size_t i) {
		Assert(i >= 0 && i < m_slotDesc.size());
		return m_slotDesc[i];
	}

	void cacheDescriptorCreateParamByIndex(DescriptorHeapSlotDesc& slotDesc, size_t index) {
		Assert(index >= 0 && index < m_slotDesc.size());
		m_slotDesc[index] = slotDesc;
	}

	void cacheDescriptorCreateParam(DescriptorHeapSlotDesc& slotDesc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		auto& desc = GetReal()->GetDesc();
		auto diff = handle.ptr - GetReal()->GetCPUDescriptorHandleForHeapStart().ptr;
		SIZE_T num = (SIZE_T)(diff / m_pWrappedDevice->GetDescriptorHandleIncrementSize(desc.Type));
		Assert(num >= 0 && num < desc.NumDescriptors);
		//m_slotDesc[num] = slotDesc;
		cacheDescriptorCreateParamByIndex(slotDesc, num);
	}

public: //framework:
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	std::vector<DescriptorHeapSlotDesc> m_slotDesc;
};

RDCBOOST_NAMESPACE_END