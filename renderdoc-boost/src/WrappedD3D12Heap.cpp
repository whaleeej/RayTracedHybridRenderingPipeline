#include "WrappedD3D12Heap.h"
#include "WrappedD3D12Resource.h"

RDCBOOST_NAMESPACE_BEGIN



WrappedD3D12Heap::WrappedD3D12Heap(ID3D12Heap* pReal, WrappedD3D12Device* pWrappedDevice)
	: WrappedD3D12DeviceChild(pReal, pWrappedDevice)
{
	
}

WrappedD3D12Heap::~WrappedD3D12Heap() {

}

COMPtr<ID3D12DeviceChild> WrappedD3D12Heap::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12Heap>pvHeap=0;
	D3D12_HEAP_DESC& heapDesc = GetReal()->GetDesc();
	pNewDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&pvHeap));
	return pvHeap;
}

WrappedD3D12DescriptorHeap::WrappedD3D12DescriptorHeap(ID3D12DescriptorHeap* pReal, WrappedD3D12Device* pWrappedDevice)
	:WrappedD3D12DeviceChild(pReal, pWrappedDevice)
{
	auto& desc = GetReal()->GetDesc();
	m_slotDesc.resize(desc.NumDescriptors);
}

WrappedD3D12DescriptorHeap::~WrappedD3D12DescriptorHeap() {

}

struct TryExistImpl { //这里是一个非常sneaky的实现，最好是res在释放的时候主动从DescriptorHeap里去掉，但是太耗费性能了
	bool IsWrappedD3D12ResourceExisted(WrappedD3D12Resource* pWrappedD3D12Res) {
		if (!pWrappedD3D12Res)
			return false;
		try {
			pWrappedD3D12Res->GetReal();
		}
		catch (...) {
			return false;
		}
		return true;
	}
};

COMPtr<ID3D12DeviceChild> WrappedD3D12DescriptorHeap::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12DescriptorHeap> pvNewDescriptorHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = pvNewDescriptorHeap->GetDesc();
	pNewDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&pvNewDescriptorHeap));
	Assert(pvNewDescriptorHeap.Get());

	//TODO: 这里要确认slotRes指向的Res是否已经被delete掉了，这里暂时使用Exception机制
	for (size_t i = 0; i < m_slotDesc.size(); i++) {
		
		//if (m_slotDesc[i].viewDescType!= ViewDesc_SamplerV
		//	&&!TryExistImpl().IsWrappedD3D12ResourceExisted(m_slotDesc[i].pWrappedD3D12Resource)) continue;
		auto targetHandle = pvNewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		targetHandle.ptr = targetHandle.ptr +(size_t)pNewDevice->GetDescriptorHandleIncrementSize(descHeapDesc.Type)*i;

		auto pWrappedRes = m_slotDesc[i].pWrappedD3D12Resource;

		switch (m_slotDesc[i].viewDescType) {
		case ViewDesc_CBV:
			Assert(pWrappedRes);
			m_slotDesc[i].concreteViewDesc.cbv.BufferLocation = pWrappedRes->GetReal()->GetGPUVirtualAddress();
			pNewDevice->CreateConstantBufferView(&m_slotDesc[i].concreteViewDesc.cbv, targetHandle);
			break;
		case ViewDesc_SRV:
			Assert(!(pWrappedRes==NULL && m_slotDesc[i].isViewDescNull));
			pNewDevice->CreateShaderResourceView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_slotDesc[i].isViewDescNull? &m_slotDesc[i].concreteViewDesc.srv : NULL,
				targetHandle);
			break;
		case ViewDesc_UAV:
			pNewDevice->CreateUnorderedAccessView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				m_slotDesc[i].pWrappedD3D12CounterResource?m_slotDesc[i].pWrappedD3D12CounterResource->GetReal().Get():NULL,
				!m_slotDesc[i].isViewDescNull ? &m_slotDesc[i].concreteViewDesc.uav : NULL,
				targetHandle);
			break;
		case ViewDesc_RTV:
			pNewDevice->CreateRenderTargetView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_slotDesc[i].isViewDescNull ? &m_slotDesc[i].concreteViewDesc.rtv:NULL,
				targetHandle);
			break;
		case ViewDesc_DSV:
			pNewDevice->CreateDepthStencilView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_slotDesc[i].isViewDescNull ? &m_slotDesc[i].concreteViewDesc.dsv:NULL,
				targetHandle);
			break;
		case ViewDesc_SamplerV:
			pNewDevice->CreateSampler(&m_slotDesc[i].concreteViewDesc.sampler, targetHandle);
			break;
		case ViewDesc_Unknown:
			LogError("Code will not be here");
			break;
		}
	}

	return pvNewDescriptorHeap;

}

/************************************************************************/
/*                         override                                                                     */
/************************************************************************/

D3D12_HEAP_DESC STDMETHODCALLTYPE WrappedD3D12Heap::GetDesc(void) {
	return GetReal()->GetDesc();
}

D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE WrappedD3D12DescriptorHeap::GetDesc(void) {
	return GetReal()->GetDesc();
}

D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE WrappedD3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart(void)
{
	return GetReal()->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE WrappedD3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart(void) {
	return GetReal()->GetGPUDescriptorHandleForHeapStart();
}


RDCBOOST_NAMESPACE_END