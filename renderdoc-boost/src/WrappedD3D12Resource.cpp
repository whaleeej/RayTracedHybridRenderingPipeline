#include "WrappedD3D12DXGISwapChain.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12Heap.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12Resource::~WrappedD3D12Resource() {
	if (m_ClearValue) {
		delete m_ClearValue;
	}
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		WrappedD3D12GPUVAddrMgr::Get().Free(m_Offset);
}

COMPtr<ID3D12DeviceChild> WrappedD3D12Resource::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12Resource> pvResource;
	switch (m_Type)
	{
	//创建D3D12_RESOURCE_STATE_COPY_DEST状态的资源，方便在device的switch中拷贝
	case CommittedWrappedD3D12Resource:
		D3D12_HEAP_PROPERTIES pHeapProperties;
		D3D12_HEAP_FLAGS pHeapFlags;
		GetReal()->GetHeapProperties(&pHeapProperties, &pHeapFlags);
		if (pHeapProperties.Type == D3D12_HEAP_TYPE_UPLOAD ||
			pHeapProperties.Type == D3D12_HEAP_TYPE_READBACK) m_bNeedCopy = false;
		pNewDevice->CreateCommittedResource(
			&m_HeapProperties, m_HeapFlags, &m_Desc, 
			m_bNeedCopy?D3D12_RESOURCE_STATE_COPY_DEST:m_State, m_ClearValue, IID_PPV_ARGS(&pvResource));
		break;
	case PlacedWrappedD3D12Resource:
		LogError("Placed D3D12Resource copy unsupportted now");
		Assert(m_pWrappedHeap.Get()&&m_pWrappedHeap->GetReal().Get());
		if (m_pWrappedHeap.Get()->GetDesc().Properties.Type == D3D12_HEAP_TYPE_UPLOAD ||
			m_pWrappedHeap.Get()->GetDesc().Properties.Type == D3D12_HEAP_TYPE_READBACK) m_bNeedCopy = false;
		pNewDevice->CreatePlacedResource(
			m_pWrappedHeap->GetReal().Get(), m_heapOffset, &m_Desc, 
			m_bNeedCopy ? D3D12_RESOURCE_STATE_COPY_DEST : m_State, m_ClearValue, IID_PPV_ARGS(&pvResource));
		break;
	case ReservedWrappedD3D12Resource:
		LogError("Reserved D3D12Resource copy unsupportted now");
		m_bNeedCopy = false;
		pNewDevice->CreateReservedResource(
			&m_Desc, 
			m_State, m_ClearValue, IID_PPV_ARGS(&pvResource));
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
	//TODO: copy content in the swapchain buffers
	m_Desc = pNewResource->GetDesc();
	m_PrivateData.CopyPrivateData(pNewResource);
	m_pReal = pNewResource;
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
	//return GetReal()->GetGPUVirtualAddress();
	//return reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(this);
	return m_Offset;
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