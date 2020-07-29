#include "WrappedD3D12DXGISwapChain.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12Heap.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12Resource::WrappedD3D12Resource(
	ID3D12Resource* pReal, WrappedD3D12Device* pDevice,
	WrappedD3D12Heap* pHeap,
	WrappedD3D12ResourceType type,
	D3D12_RESOURCE_STATES state ,
	const D3D12_CLEAR_VALUE *pClearValue,
	UINT64 heapOffset)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_pWrappedHeap(pHeap),
	m_Type(type),
	m_State(state),
	m_pRealSwapChain(NULL),
	m_heapOffset(heapOffset)
{
	if (pClearValue) {
		m_ClearValue = *pClearValue;
	}
}
WrappedD3D12Resource::~WrappedD3D12Resource() {
}


ID3D12DeviceChild* WrappedD3D12Resource::CopyToDevice(ID3D12Device* pNewDevice) {
	D3D12_RESOURCE_DESC resDesc = GetReal()->GetDesc();
	ID3D12Resource* pvResource;
	switch (m_Type)
	{
	//TODO Copy to new device
	case CommittedWrappedD3D12Resource:
		D3D12_HEAP_PROPERTIES heapProperties;
		D3D12_HEAP_FLAGS heapFlags;
		GetReal()->GetHeapProperties(&heapProperties, &heapFlags);
		pNewDevice->CreateCommittedResource(
			&heapProperties, heapFlags, &resDesc, m_State, &m_ClearValue, IID_PPV_ARGS(&pvResource));
		break;
	case PlacedWrappedD3D12Resource:
		Assert(m_pWrappedHeap&&m_pWrappedHeap->GetReal());
		pNewDevice->CreatePlacedResource(
			m_pWrappedHeap->GetReal(), m_heapOffset, &resDesc, m_State, &m_ClearValue, IID_PPV_ARGS(&pvResource));
		break;
	case ReservedWrappedD3D12Resource:
		pNewDevice->CreateReservedResource(
			&resDesc, m_State, &m_ClearValue, IID_PPV_ARGS(&pvResource));
		break;
	case BackBufferWrappedD3D12Resource:
		//do nothing
		break;
	}

	return pvResource;
}
void WrappedD3D12Resource::InitSwapChain(IDXGISwapChain1* pRealSwapChain) {
	m_pRealSwapChain = pRealSwapChain;
}

void WrappedD3D12Resource::SwitchToSwapChain(IDXGISwapChain1* pNewSwapChain, ID3D12Resource* pNewResource) {
	if (pNewSwapChain == m_pRealSwapChain)
		return;
	Assert(pNewResource != NULL);
	//TODO: copy content in the swapchain buffer //no need by now
	m_PrivateData.CopyPrivateData(pNewResource);
	m_pReal->Release();
	m_pReal = pNewResource;
	m_pReal->AddRef();
	m_pRealSwapChain = pNewSwapChain;
}

void WrappedD3D12Resource::changeToState(D3D12_RESOURCE_STATES state) {
	m_State = state;
}

/************************************************************************/
/*                        wrapped                                                                     */
/************************************************************************/
HRESULT STDMETHODCALLTYPE WrappedD3D12Resource::Map(
	UINT Subresource,
	_In_opt_  const D3D12_RANGE *pReadRange,
	_Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData) {
	return GetReal()->Map(Subresource, pReadRange, ppData);
}

void STDMETHODCALLTYPE WrappedD3D12Resource::Unmap(
	UINT Subresource,
	_In_opt_  const D3D12_RANGE *pWrittenRange) {
	return GetReal()->Unmap(Subresource, pWrittenRange);
}

D3D12_RESOURCE_DESC STDMETHODCALLTYPE WrappedD3D12Resource::GetDesc(void) {
	return GetReal()->GetDesc();
}

D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE WrappedD3D12Resource::GetGPUVirtualAddress(void) {
	return GetReal()->GetGPUVirtualAddress();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Resource::WriteToSubresource(
	UINT DstSubresource,
	_In_opt_  const D3D12_BOX *pDstBox,
	_In_  const void *pSrcData,
	UINT SrcRowPitch,
	UINT SrcDepthPitch) {
	return GetReal()->WriteToSubresource(DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Resource::ReadFromSubresource(
	_Out_  void *pDstData,
	UINT DstRowPitch,
	UINT DstDepthPitch,
	UINT SrcSubresource,
	_In_opt_  const D3D12_BOX *pSrcBox) {
	return GetReal()->ReadFromSubresource(pDstData, DstRowPitch, DstDepthPitch, SrcSubresource, pSrcBox);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Resource::GetHeapProperties(
	_Out_opt_  D3D12_HEAP_PROPERTIES *pHeapProperties,
	_Out_opt_  D3D12_HEAP_FLAGS *pHeapFlags) {
	return GetReal()->GetHeapProperties(pHeapProperties, pHeapFlags);
}

RDCBOOST_NAMESPACE_END