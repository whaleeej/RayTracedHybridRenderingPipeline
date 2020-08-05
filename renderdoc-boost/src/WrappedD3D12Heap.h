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
	struct DescriptorHeapSlot{ // TODO 把WrappedD3D12Resource的生命拥有起来，改成ComPtr
		ViewDescType viewDescType = ViewDesc_Unknown;
		WrappedD3D12Resource* pWrappedD3D12Resource = NULL;
		WrappedD3D12Resource* pWrappedD3D12CounterResource = NULL;
		ID3D12Resource* pRealD3D12Object = NULL;
		bool isViewDescNull = true;
		union ConcreteViewDesc{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv;
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
			D3D12_SAMPLER_DESC sampler;
		}concreteViewDesc;

		UINT  idx;
		WrappedD3D12DescriptorHeap* m_DescriptorHeap;

		D3D12_RESOURCE_DESC res_desc = CD3DX12_RESOURCE_DESC
		(D3D12_RESOURCE_DIMENSION_UNKNOWN,0,0,0,0,0, DXGI_FORMAT_UNKNOWN,0,0, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE);

		D3D12_CPU_DESCRIPTOR_HANDLE getRealCPUHandle() {
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(
				m_DescriptorHeap->m_cpuStart,
				m_DescriptorHeap->m_pRealDevice->GetDescriptorHandleIncrementSize(m_DescriptorHeap->GetDesc().Type),
				idx
			);
		};

		D3D12_GPU_DESCRIPTOR_HANDLE getRealGPUHandle() {
			return CD3DX12_GPU_DESCRIPTOR_HANDLE(
				m_DescriptorHeap->m_gpuStart,
				m_DescriptorHeap->m_pRealDevice->GetDescriptorHandleIncrementSize(m_DescriptorHeap->GetDesc().Type),
				idx
			);

		};

		DescriptorHeapSlot& operator=(const DescriptorHeapSlot& cls) {
			viewDescType = cls.viewDescType;
			pWrappedD3D12Resource = cls.pWrappedD3D12Resource;
			pWrappedD3D12CounterResource = cls.pWrappedD3D12CounterResource;
			isViewDescNull = cls.isViewDescNull;
			concreteViewDesc = cls.concreteViewDesc;
			pRealD3D12Object = cls.pRealD3D12Object;
			res_desc = cls.res_desc;
			return *this;
		}
	};

public:
	WrappedD3D12DescriptorHeap(ID3D12DescriptorHeap* pReal, WrappedD3D12Device* pWrappedDevice);
	~WrappedD3D12DescriptorHeap();

public: //override
	virtual D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc(void);

	virtual D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetCPUDescriptorHandleForHeapStart(void);

	virtual D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetGPUDescriptorHandleForHeapStart(void);

public: // function

public: //framework:
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	friend DescriptorHeapSlot;
	std::vector<DescriptorHeapSlot> m_Slots;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart;
};

RDCBOOST_NAMESPACE_END