#include <WrappedD3D12Device.h>

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12Device::WrappedD3D12Device(ID3D12Device * pRealDevice, const SDeviceCreateParams& param)
	: WrappedD3D12Object(pRealDevice)
	, m_DeviceCreateParams(param)
	, m_bRenderDocDevice(false)
{
}

WrappedD3D12Device::~WrappedD3D12Device() {
	//release handled in WrappedD3D12ObjectBase
}

void WrappedD3D12Device::OnDeviceChildReleased(ID3D12DeviceChild* pReal) {
	if (m_BackRefs.erase(pReal) !=0) return;

	//TODO not in backRefs, then it will be in backbuffer for swapchain, try erase in it

	//TODO unknown wrapped devicechild released error
	LogError("Unknown device child released.");
	Assert(false);
}

void WrappedD3D12Device::SwitchToDevice(ID3D12Device* pNewDevice) {
	//0. create new device(done)
	Assert(pNewDevice != NULL);

	//1. copy private data of device
	m_PrivateData.CopyPrivateData(pNewDevice);

	//2. create new swapchain

	//3. swapchain buffer switchToDevice

	//4. backRefs switchToDevice
	if (!m_BackRefs.empty()) {
		printf("Transferring DeviceChild resources to new device without modifying the content of WrappedDeviceChild\n");
		printf("--------------------------------------------------\n");
		std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> newBackRefs;
		int progress; int idx;
		for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); it++) {
			it->second->SwitchToDevice(pNewDevice);//key of the framework
			newBackRefs[static_cast<ID3D12DeviceChild*>(it->second->GetRealObject())] = it->second;
			// sequencing
			++idx;
			while (progress < (int)(idx * 50 / m_BackRefs.size()))
			{
				printf(">");
				++progress;
			}
		}
		printf("\n");
		m_BackRefs.swap(newBackRefs);
	}

	//5.pReal substitute
	ULONG refs = m_pReal->Release();
	if (refs != 0)
		LogError("Previous real device ref count: %d", static_cast<int>(refs));
	m_pReal = pNewDevice;
	m_pReal->AddRef();

}



/***********************************override***********************************/
UINT STDMETHODCALLTYPE WrappedD3D12Device::GetNodeCount(void){
	return GetReal()->GetNodeCount();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandQueue(
	_In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppCommandQueue){
	if (ppCommandQueue == NULL)
		return GetReal()->CreateCommandQueue(pDesc, riid, NULL);

	ID3D12CommandQueue* pCommandQueue = NULL;
	HRESULT ret = GetReal()->CreateCommandQueue(pDesc, IID_PPV_ARGS(&pCommandQueue));
	if (pCommandQueue) {
		WrappedD3D12DeviceChild<ID3D12CommandQueue>* wrapped = new WrappedD3D12DeviceChild<ID3D12CommandQueue>(pCommandQueue, this);
		*ppCommandQueue = wrapped;
		m_BackRefs[pCommandQueue] = wrapped;
		pCommandQueue->Release();
	}
	else {
		*ppCommandQueue = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandAllocator(
	_In_  D3D12_COMMAND_LIST_TYPE type,
	REFIID riid,
	_COM_Outptr_  void **ppCommandAllocator){
	if (ppCommandAllocator == NULL)
		return GetReal()->CreateCommandAllocator(type, riid, NULL);

	ID3D12CommandAllocator* pCommandAllocator = NULL;
	HRESULT ret = GetReal()->CreateCommandAllocator(type, IID_PPV_ARGS(&pCommandAllocator));
	if (pCommandAllocator) {
		WrappedD3D12DeviceChild<ID3D12CommandAllocator>* wrapped = new WrappedD3D12DeviceChild<ID3D12CommandAllocator>(pCommandAllocator, this);
		*ppCommandAllocator = wrapped;
		m_BackRefs[pCommandAllocator] = wrapped;
		pCommandAllocator->Release();
	}
	else {
		*ppCommandAllocator = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateGraphicsPipelineState(
	_In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppPipelineState){
	if (ppPipelineState == NULL)
		return GetReal()->CreateGraphicsPipelineState(pDesc, riid, NULL);

	ID3D12PipelineState* pPipelineState = NULL;
	HRESULT ret = GetReal()->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(&pPipelineState));
	if (pPipelineState) {
		WrappedD3D12DeviceChild<ID3D12PipelineState>* wrapped = new WrappedD3D12DeviceChild<ID3D12PipelineState>(pPipelineState, this);
		*ppPipelineState = wrapped;
		m_BackRefs[pPipelineState] = wrapped;
		pPipelineState->Release();
	}
	else {
		*ppPipelineState = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateComputePipelineState(
	_In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppPipelineState){
	if (ppPipelineState == NULL)
		return GetReal()->CreateComputePipelineState(pDesc, riid, NULL);

	ID3D12PipelineState* pPipelineState = NULL;
	HRESULT ret = GetReal()->CreateComputePipelineState(pDesc, IID_PPV_ARGS(&pPipelineState));
	if (pPipelineState) {
		WrappedD3D12DeviceChild<ID3D12PipelineState>* wrapped = new WrappedD3D12DeviceChild<ID3D12PipelineState>(pPipelineState, this);
		*ppPipelineState = wrapped;
		m_BackRefs[pPipelineState] = wrapped;
		pPipelineState->Release();
	}
	else {
		*ppPipelineState = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandList(
	_In_  UINT nodeMask,
	_In_  D3D12_COMMAND_LIST_TYPE type,
	_In_  ID3D12CommandAllocator *pCommandAllocator,
	_In_opt_  ID3D12PipelineState *pInitialState,
	REFIID riid,
	_COM_Outptr_  void **ppCommandList){
	if (ppCommandList == NULL)
		return GetReal()->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, NULL);

	ID3D12CommandList* pCommandList = NULL;
	HRESULT ret = GetReal()->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, IID_PPV_ARGS(&pCommandList));
	if (pCommandList) {
		WrappedD3D12DeviceChild<ID3D12CommandList>* wrapped = new WrappedD3D12DeviceChild<ID3D12CommandList>(pCommandList, this);
		*ppCommandList = wrapped;
		m_BackRefs[pCommandList] = wrapped;
		pCommandList->Release();
	}
	else {
		*ppCommandList = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CheckFeatureSupport(
	D3D12_FEATURE Feature,
	_Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
	UINT FeatureSupportDataSize){
	return GetReal()->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateDescriptorHeap(
	_In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
	REFIID riid,
	_COM_Outptr_  void **ppvHeap){
	if (ppvHeap == NULL)
		return GetReal()->CreateDescriptorHeap(pDescriptorHeapDesc, riid, NULL);

	ID3D12DescriptorHeap* pvHeap = NULL;
	HRESULT ret = GetReal()->CreateDescriptorHeap(pDescriptorHeapDesc, IID_PPV_ARGS(&pvHeap));
	if (pvHeap) {
		WrappedD3D12DeviceChild<ID3D12DescriptorHeap>* wrapped = new WrappedD3D12DeviceChild<ID3D12DescriptorHeap>(pvHeap, this);
		*ppvHeap = wrapped;
		m_BackRefs[pvHeap] = wrapped;
		pvHeap->Release();
	}
	else {
		*ppvHeap = NULL;
	}
	return ret;
}

UINT STDMETHODCALLTYPE WrappedD3D12Device::GetDescriptorHandleIncrementSize(
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateRootSignature(
	_In_  UINT nodeMask,
	_In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
	_In_  SIZE_T blobLengthInBytes,
	REFIID riid,
	_COM_Outptr_  void **ppvRootSignature){
	if (ppvRootSignature == NULL)
		return GetReal()->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, NULL);

	ID3D12RootSignature* pvRootSignature = NULL;
	HRESULT ret = GetReal()->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(&pvRootSignature));
	if (pvRootSignature) {
		WrappedD3D12DeviceChild<ID3D12RootSignature>* wrapped = new WrappedD3D12DeviceChild<ID3D12RootSignature>(pvRootSignature, this);
		*ppvRootSignature = wrapped;
		m_BackRefs[pvRootSignature] = wrapped;
		pvRootSignature->Release();
	}
	else {
		*ppvRootSignature = NULL;
	}
	return ret;
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateConstantBufferView(
	_In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateShaderResourceView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateUnorderedAccessView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  ID3D12Resource *pCounterResource,
	_In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateRenderTargetView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateDepthStencilView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateSampler(
	_In_  const D3D12_SAMPLER_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor){}

void STDMETHODCALLTYPE WrappedD3D12Device::CopyDescriptors(
	_In_  UINT NumDestDescriptorRanges,
	_In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
	_In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
	_In_  UINT NumSrcDescriptorRanges,
	_In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
	_In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType){}

void STDMETHODCALLTYPE WrappedD3D12Device::CopyDescriptorsSimple(
	_In_  UINT NumDescriptors,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType){}

D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE WrappedD3D12Device::GetResourceAllocationInfo(
	_In_  UINT visibleMask,
	_In_  UINT numResourceDescs,
	_In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs){}

D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE WrappedD3D12Device::GetCustomHeapProperties(
	_In_  UINT nodeMask,
	D3D12_HEAP_TYPE heapType){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommittedResource(
	_In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
	D3D12_HEAP_FLAGS HeapFlags,
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialResourceState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riidResource,
	_COM_Outptr_opt_  void **ppvResource){
	if (ppvResource == NULL)
		return GetReal()->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, riidResource, NULL);

	ID3D12Resource* pvResource = NULL;
	HRESULT ret = GetReal()->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (pvResource) {
		WrappedD3D12DeviceChild<ID3D12Resource>* wrapped = new WrappedD3D12DeviceChild<ID3D12Resource>(pvResource, this);
		*ppvResource = wrapped;
		m_BackRefs[pvResource] = wrapped;
		pvResource->Release();
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateHeap(
	_In_  const D3D12_HEAP_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvHeap){
	if (ppvHeap == NULL)
		return GetReal()->CreateHeap(pDesc, riid, NULL);

	ID3D12Heap* pvHeap = NULL;
	HRESULT ret = GetReal()->CreateHeap(pDesc, IID_PPV_ARGS(&pvHeap));
	if (pvHeap) {
		WrappedD3D12DeviceChild<ID3D12Heap>* wrapped = new WrappedD3D12DeviceChild<ID3D12Heap>(pvHeap, this);
		*ppvHeap = wrapped;
		m_BackRefs[pvHeap] = wrapped;
		pvHeap->Release();
	}
	else {
		*ppvHeap = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreatePlacedResource(
	_In_  ID3D12Heap *pHeap,
	UINT64 HeapOffset,
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvResource){
	if (ppvResource == NULL)
		return GetReal()->CreatePlacedResource(pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, NULL);

	ID3D12Resource* pvResource = NULL;
	HRESULT ret = GetReal()->CreatePlacedResource(pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (pvResource) {
		WrappedD3D12DeviceChild<ID3D12Resource>* wrapped = new WrappedD3D12DeviceChild<ID3D12Resource>(pvResource, this);
		*ppvResource = wrapped;
		m_BackRefs[pvResource] = wrapped;
		pvResource->Release();
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateReservedResource(
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvResource){
	if (ppvResource == NULL)
		return GetReal()->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, riid, NULL);

	ID3D12Resource* pvResource = NULL;
	HRESULT ret = GetReal()->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (pvResource) {
		WrappedD3D12DeviceChild<ID3D12Resource>* wrapped = new WrappedD3D12DeviceChild<ID3D12Resource>(pvResource, this);
		*ppvResource = wrapped;
		m_BackRefs[pvResource] = wrapped;
		pvResource->Release();
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateSharedHandle(
	_In_  ID3D12DeviceChild *pObject,
	_In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
	DWORD Access,
	_In_opt_  LPCWSTR Name,
	_Out_  HANDLE *pHandle){
	LogError("CreateSharedHandle not supported yet");
	return GetReal()->CreateSharedHandle(pObject, pAttributes, Access, Name, pHandle);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::OpenSharedHandle(
	_In_  HANDLE NTHandle,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvObj) {
	LogError("OpenSharedHandle not supported yet");
	return GetReal()->OpenSharedHandle(NTHandle, riid, ppvObj);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::OpenSharedHandleByName(
	_In_  LPCWSTR Name,
	DWORD Access,
	/* [annotation][out] */
	_Out_  HANDLE *pNTHandle) {
	LogError("OpenSharedHandleByName not supported yet");
	return GetReal()->OpenSharedHandleByName(Name, Access, pNTHandle);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::MakeResident(
	UINT NumObjects,
	_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::Evict(
	UINT NumObjects,
	_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateFence(
	UINT64 InitialValue,
	D3D12_FENCE_FLAGS Flags,
	REFIID riid,
	_COM_Outptr_  void **ppFence) {
	if (ppFence == NULL)
		return GetReal()->CreateFence(InitialValue, Flags, riid, NULL);

	ID3D12Fence* pFence = NULL;
	HRESULT ret = GetReal()->CreateFence(InitialValue, Flags, IID_PPV_ARGS(&pFence));
	if (pFence) {
		WrappedD3D12DeviceChild<ID3D12Fence>* wrapped = new WrappedD3D12DeviceChild<ID3D12Fence>(pFence, this);
		*ppFence = wrapped;
		m_BackRefs[pFence] = wrapped;
		pFence->Release();
	}
	else {
		*ppFence = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::GetDeviceRemovedReason(void){}

void STDMETHODCALLTYPE WrappedD3D12Device::GetCopyableFootprints(
	_In_  const D3D12_RESOURCE_DESC *pResourceDesc,
	_In_range_(0, D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
	_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource)  UINT NumSubresources,
	UINT64 BaseOffset,
	_Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
	_Out_writes_opt_(NumSubresources)  UINT *pNumRows,
	_Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
	_Out_opt_  UINT64 *pTotalBytes){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateQueryHeap(
	_In_  const D3D12_QUERY_HEAP_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvHeap) {
	if (ppvHeap == NULL)
		return GetReal()->CreateQueryHeap(pDesc, riid, NULL);

	ID3D12QueryHeap* pvHeap = NULL;
	HRESULT ret = GetReal()->CreateQueryHeap(pDesc, IID_PPV_ARGS(&pvHeap));
	if (pvHeap) {
		WrappedD3D12DeviceChild<ID3D12QueryHeap>* wrapped = new WrappedD3D12DeviceChild<ID3D12QueryHeap>(pvHeap, this);
		*ppvHeap = wrapped;
		m_BackRefs[pvHeap] = wrapped;
		pvHeap->Release();
	}
	else {
		*ppvHeap = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::SetStablePowerState(
	BOOL Enable){}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandSignature(
	_In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
	_In_opt_  ID3D12RootSignature *pRootSignature,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvCommandSignature) {
	if (ppvCommandSignature == NULL)
		return GetReal()->CreateCommandSignature(pDesc, pRootSignature, riid, NULL);

	ID3D12CommandSignature* pCommandSignature = NULL;
	HRESULT ret = GetReal()->CreateCommandSignature(pDesc, pRootSignature,IID_PPV_ARGS(&pCommandSignature));
	if (pCommandSignature) {
		WrappedD3D12DeviceChild<ID3D12CommandSignature>* wrapped = new WrappedD3D12DeviceChild<ID3D12CommandSignature>(pCommandSignature, this);
		*ppvCommandSignature = wrapped;
		m_BackRefs[pCommandSignature] = wrapped;
		pCommandSignature->Release();
	}
	else {
		*ppvCommandSignature = NULL;
	}
	return ret;
}

void STDMETHODCALLTYPE WrappedD3D12Device::GetResourceTiling(
	_In_  ID3D12Resource *pTiledResource,
	_Out_opt_  UINT *pNumTilesForEntireResource,
	_Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
	_Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
	_Inout_opt_  UINT *pNumSubresourceTilings,
	_In_  UINT FirstSubresourceTilingToGet,
	_Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips){}

LUID STDMETHODCALLTYPE WrappedD3D12Device::GetAdapterLuid(void){}

RDCBOOST_NAMESPACE_END

