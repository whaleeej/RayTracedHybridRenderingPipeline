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
	m_Slots.resize(desc.NumDescriptors);
	m_cpuStart = GetReal()->GetCPUDescriptorHandleForHeapStart();
	m_gpuStart = GetReal()->GetGPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_Slots.size(); i++) {
		m_Slots[i].idx = i;
		m_Slots[i].m_DescriptorHeap = this;
	}
}

WrappedD3D12DescriptorHeap::~WrappedD3D12DescriptorHeap() {
	auto& desc = GetReal()->GetDesc();
}

COMPtr<ID3D12DeviceChild> WrappedD3D12DescriptorHeap::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12DescriptorHeap> pvNewDescriptorHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = this->GetDesc();
	pNewDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&pvNewDescriptorHeap));
	Assert(pvNewDescriptorHeap.Get());
	m_cpuStart = pvNewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_gpuStart = pvNewDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	//if ((descHeapDesc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=0) {
	//	return pvNewDescriptorHeap;
	//}

	for (UINT i = 0; i < m_Slots.size(); i++) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
			pvNewDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
			i, 
			pNewDevice->GetDescriptorHandleIncrementSize(descHeapDesc.Type));

		auto pWrappedRes = m_Slots[i].pWrappedD3D12Resource;
		switch (m_Slots[i].viewDescType) {
		case ViewDesc_CBV: {
			if (!pWrappedRes || !GetWrappedDevice()->isResourceExist(pWrappedRes, m_Slots[i].pRealD3D12Object)) break;
			auto cbv = m_Slots[i].concreteViewDesc.cbv;
			DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(cbv.BufferLocation);
			cbv.BufferLocation = realVAddr;
			pNewDevice->CreateConstantBufferView(&cbv, handle);
			break;
		}
		case ViewDesc_SRV:
		{
			Assert(!(pWrappedRes == NULL && m_Slots[i].isViewDescNull));
			if (pWrappedRes && !GetWrappedDevice()->isResourceExist(pWrappedRes, m_Slots[i].pRealD3D12Object)) break;
			pNewDevice->CreateShaderResourceView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_Slots[i].isViewDescNull ? &m_Slots[i].concreteViewDesc.srv : NULL,
				handle);
			break;
		}
		case ViewDesc_UAV:
		{
			Assert(!(pWrappedRes == NULL && m_Slots[i].isViewDescNull));
			if (pWrappedRes && !GetWrappedDevice()->isResourceExist(pWrappedRes, m_Slots[i].pRealD3D12Object)) break;
			pNewDevice->CreateUnorderedAccessView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				m_Slots[i].pWrappedD3D12CounterResource ? m_Slots[i].pWrappedD3D12CounterResource->GetReal().Get() : NULL,
				!m_Slots[i].isViewDescNull ? &m_Slots[i].concreteViewDesc.uav : NULL,
				handle);
			break;
		}
		case ViewDesc_RTV:
		{
			Assert(!(pWrappedRes == NULL && m_Slots[i].isViewDescNull));
			if (pWrappedRes && !GetWrappedDevice()->isResourceExist(pWrappedRes, m_Slots[i].pRealD3D12Object)) break;
			pNewDevice->CreateRenderTargetView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_Slots[i].isViewDescNull ? &m_Slots[i].concreteViewDesc.rtv : NULL,
				handle);
			break;
		}
		case ViewDesc_DSV:
		{
			Assert(!(pWrappedRes == NULL && m_Slots[i].isViewDescNull));
			if (pWrappedRes && !GetWrappedDevice()->isResourceExist(pWrappedRes, m_Slots[i].pRealD3D12Object)) break;
			pNewDevice->CreateDepthStencilView(
				pWrappedRes ? pWrappedRes->GetReal().Get() : NULL,
				!m_Slots[i].isViewDescNull ? &m_Slots[i].concreteViewDesc.dsv : NULL,
				handle);
			break;
		}
		case ViewDesc_SamplerV:
		{
			pNewDevice->CreateSampler(&m_Slots[i].concreteViewDesc.sampler, handle);
			break;
		}
		case ViewDesc_Unknown:
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
	//return GetReal()->GetCPUDescriptorHandleForHeapStart();//hooked with slots
	return { (SIZE_T)static_cast<void*>(m_Slots.data()) };
}

D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE WrappedD3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart(void) {
	//return GetReal()->GetGPUDescriptorHandleForHeapStart();//hooked with slots
	return { (SIZE_T)static_cast<void*>(m_Slots.data()) };
}


RDCBOOST_NAMESPACE_END